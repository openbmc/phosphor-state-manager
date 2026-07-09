#include "config.h"

#include "chassis_state_manager_smp.hpp"

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/exception.hpp>
#include <xyz/openbmc_project/Common/error.hpp>
#include <xyz/openbmc_project/Inventory/Item/common.hpp>

#include <format>
#include <fstream>
#include <iostream>
#include <map>
#include <string>

namespace phosphor::state::manager
{

PHOSPHOR_LOG2_USING;

// When you see server:: or reboot:: you know we're referencing our base class
namespace server = sdbusplus::server::xyz::openbmc_project::state;

using namespace phosphor::logging;
using sdbusplus::xyz::openbmc_project::Common::Error::InternalFailure;
using InventoryItem = sdbusplus::common::xyz::openbmc_project::inventory::Item;

constexpr auto SYSTEMD_SERVICE = "org.freedesktop.systemd1";
constexpr auto SYSTEMD_OBJ_PATH = "/org/freedesktop/systemd1";
constexpr auto SYSTEMD_INTERFACE = "org.freedesktop.systemd1.Manager";

constexpr auto PROPERTY_INTERFACE = "org.freedesktop.DBus.Properties";

constexpr auto CHASSIS_POWEROFF_TARGET = "obmc-chassis-poweroff@0.target";
constexpr auto CHASSIS_POWERON_TARGET = "obmc-chassis-poweron@0.target";
constexpr auto CHASSIS_POWERCYCLE_TARGET = "obmc-chassis-powercycle@0.target";
constexpr auto CHASSIS_OBJ_PATH = "/xyz/openbmc_project/state/chassis{}";
constexpr auto CHASSIS_SERVICE = "xyz.openbmc_project.State.Chassis{}";

ChassisSMP::ChassisSMP(sdbusplus::bus_t& bus,
                       const sdbusplus::object_path& objPath,
                       size_t numChassis) :
    ChassisInherit(bus, objPath, ChassisInherit::action::defer_emit), bus(bus),
    numChassis(numChassis),
    systemdSignalJobNew(
        bus,
        sdbusRule::type::signal() + sdbusRule::member("JobNew") +
            sdbusRule::path("/org/freedesktop/systemd1") +
            sdbusRule::interface("org.freedesktop.systemd1.Manager"),
        [this](sdbusplus::message_t& m) { sysStateChangeJobNew(m); })
{
    if (numChassis == 0)
    {
        throw std::invalid_argument(
            "ChassisSMP requires at least 1 chassis to aggregate");
    }

    info("Chassis0: Creating SMP aggregator for chassis 0, "
         "monitoring up to {NUM_CHASSIS} chassis instances",
         "NUM_CHASSIS", numChassis);

    // Initialize cached states to Off/Good and not present
    for (size_t i = 1; i <= numChassis; ++i)
    {
        chassisPowerStates[i] = PowerState::Off;
        chassisPowerStatus[i] = PowerStatus::Good;
        chassisPresentStatus[i] = false;
    }

    // Set initial aggregated state
    currentPowerState(PowerState::Off);
    currentPowerStatus(PowerStatus::Good);

    // Query actual chassis states before emitting object to prevent clients
    // from reading invalid default values
    startMonitoring();

    // Now that we have actual state, emit the object
    this->emit_object_added();
}

void ChassisSMP::startUnit(const std::string& sysdUnit)
{
    auto method = this->bus.new_method_call(SYSTEMD_SERVICE, SYSTEMD_OBJ_PATH,
                                            SYSTEMD_INTERFACE, "StartUnit");

    method.append(sysdUnit);
    method.append("replace");

    this->bus.call_noreply(method);
}

void ChassisSMP::startMonitoring()
{
    // Set up property change monitoring for all chassis
    for (size_t i = 1; i <= numChassis; ++i)
    {
        // Always register inventory present property monitor for all chassis
        // so we can detect when a chassis becomes present later
        sdbusplus::object_path inventoryPath =
            std::format("/xyz/openbmc_project/inventory/system/chassis{}", i);

        auto inventoryMatch = std::make_unique<sdbusplus::match>(
            bus,
            sdbusRule::propertiesChanged(inventoryPath.string(),
                                         "xyz.openbmc_project.Inventory.Item"),
            [this, i](sdbusplus::message_t& msg) {
                this->inventoryPresentChanged(msg, i);
            });

        inventoryPresentMatches.push_back(std::move(inventoryMatch));

        // Initialize the present status cache by reading current value
        chassisPresentStatus[i] = isChassisPresent(i);

        // Only monitor chassis state properties for chassis that are present
        if (!chassisPresentStatus[i])
        {
            info("Chassis0: Skipping state monitoring for chassis "
                 "{MONITORED_CHASSIS_ID} because it is not present",
                 "MONITORED_CHASSIS_ID", i);
            continue;
        }

        sdbusplus::object_path chassisPath = std::format(CHASSIS_OBJ_PATH, i);

        auto match = std::make_unique<sdbusplus::match>(
            bus,
            sdbusRule::propertiesChanged(chassisPath.string(),
                                         server::Chassis::interface),
            [this, i](sdbusplus::message_t& msg) {
                this->chassisPropertyChanged(msg, i);
            });

        chassisMatches[i] = std::move(match);

        debug("Chassis0: Monitoring chassis {MONITORED_CHASSIS_ID}",
              "MONITORED_CHASSIS_ID", i);
    }

    // Do initial aggregation
    aggregatePowerState();
    aggregatePowerStatus();
}

void ChassisSMP::aggregatePowerState()
{
    // Aggregate power state with priority:
    // 1. If ANY chassis is TransitioningToOff -> TransitioningToOff
    // 2. If ANY chassis is TransitioningToOn -> TransitioningToOn
    // 3. If ANY chassis is On -> On
    // 4. Only report Off if ALL present chassis are Off
    PowerState aggregatedState = PowerState::Off;
    bool hasTransitioningToOff = false;
    bool hasTransitioningToOn = false;
    bool hasOn = false;

    for (size_t i = 1; i <= numChassis; ++i)
    {
        if (!chassisPresentStatus[i])
        {
            continue;
        }

        sdbusplus::object_path chassisPath = std::format(CHASSIS_OBJ_PATH, i);
        std::string chassisService = std::format(CHASSIS_SERVICE, i);
        try
        {
            auto method = bus.new_method_call(
                chassisService.c_str(), chassisPath, PROPERTY_INTERFACE, "Get");
            method.append(server::Chassis::interface,
                          server::Chassis::property_names::current_power_state);

            auto reply = bus.call(method);
            auto propertyValue = reply.unpack<std::variant<PowerState>>();
            auto state = std::get<PowerState>(propertyValue);

            chassisPowerStates[i] = state;

            if (state == PowerState::TransitioningToOff)
            {
                hasTransitioningToOff = true;
            }
            else if (state == PowerState::TransitioningToOn)
            {
                hasTransitioningToOn = true;
            }
            else if (state == PowerState::On)
            {
                hasOn = true;
            }
        }
        catch (const sdbusplus::exception_t& e)
        {
            error("Chassis0: Failed to get power state for chassis "
                  "{TARGET_CHASSIS_ID}: {ERROR}",
                  "TARGET_CHASSIS_ID", i, "ERROR", e);
            chassisPowerStates[i] = PowerState::Off;
        }
    }

    if (hasTransitioningToOff)
    {
        aggregatedState = PowerState::TransitioningToOff;
    }
    else if (hasTransitioningToOn)
    {
        aggregatedState = PowerState::TransitioningToOn;
    }
    else if (hasOn)
    {
        aggregatedState = PowerState::On;
    }
    else // No present chassis or all present chassis are Off
    {
        aggregatedState = PowerState::Off;
        // Reset per-chassis failure tracking when all chassis are off
        // This allows the system to detect new failures on the next power on
        chassisFailureTriggered.clear();
    }

    if (server::Chassis::currentPowerState() != aggregatedState)
    {
        info("Chassis0: SMP Aggregator power state changing to: "
             "{POWER_STATE}",
             "POWER_STATE", aggregatedState);
        currentPowerState(aggregatedState);
    }
}

void ChassisSMP::aggregatePowerStatus()
{
    // Aggregate power status: worst case (BrownOut > UPS > Good)
    PowerStatus aggregatedStatus = PowerStatus::Good;

    for (size_t i = 1; i <= numChassis; ++i)
    {
        if (!chassisPresentStatus[i])
        {
            continue;
        }

        sdbusplus::object_path chassisPath = std::format(CHASSIS_OBJ_PATH, i);
        std::string chassisService = std::format(CHASSIS_SERVICE, i);

        try
        {
            auto method = bus.new_method_call(
                chassisService.c_str(), chassisPath, PROPERTY_INTERFACE, "Get");
            method.append(
                server::Chassis::interface,
                server::Chassis::property_names::current_power_status);

            auto reply = bus.call(method);
            auto propertyValue = reply.unpack<std::variant<PowerStatus>>();
            auto status = std::get<PowerStatus>(propertyValue);

            chassisPowerStatus[i] = status;

            // Worst case: BrownOut > UPS > Good
            if (status == PowerStatus::BrownOut)
            {
                aggregatedStatus = PowerStatus::BrownOut;
            }
            else if (status == PowerStatus::UninterruptiblePowerSupply &&
                     aggregatedStatus != PowerStatus::BrownOut)
            {
                aggregatedStatus = PowerStatus::UninterruptiblePowerSupply;
            }
        }
        catch (const sdbusplus::exception_t& e)
        {
            error("Chassis0: Failed to get power status for chassis "
                  "{TARGET_CHASSIS_ID}: {ERROR}",
                  "TARGET_CHASSIS_ID", i, "ERROR", e);
            // Assume good if we can't read the status
            chassisPowerStatus[i] = PowerStatus::Good;
        }
    }

    if (server::Chassis::currentPowerStatus() != aggregatedStatus)
    {
        info("Chassis0: SMP Aggregator power status changing to: "
             "{POWER_STATUS}",
             "POWER_STATUS", aggregatedStatus);
        currentPowerStatus(aggregatedStatus);
    }
}

void ChassisSMP::chassisPropertyChanged(sdbusplus::message_t& msg,
                                        size_t chassisId)
{
    std::string interface;
    std::map<std::string, std::variant<std::string>> properties;

    msg.read(interface, properties);

    for (const auto& [property, value] : properties)
    {
        if (property == server::Chassis::property_names::current_power_state)
        {
            auto stateStr = std::get<std::string>(value);
            PowerState state =
                server::Chassis::convertPowerStateFromString(stateStr);

            // Check if this chassis is transitioning to off due to a failure
            // This is only a failure if we're currently trying to power on
            // (TransitioningToOn or On state), not during a normal power off
            // Only do this if we haven't already initiated a coordinated power
            // off
            auto currentState = server::Chassis::currentPowerState();
            if (state == PowerState::TransitioningToOff &&
                chassisPowerStates[chassisId] !=
                    PowerState::TransitioningToOff &&
                !chassisFailureTriggered[chassisId] &&
                (currentState == PowerState::TransitioningToOn ||
                 currentState == PowerState::On))
            {
                warning(
                    "Chassis0: Chassis {FAILED_CHASSIS_ID} is "
                    "transitioning to off while system is in {POWER_STATE}, "
                    "initiating power off for all chassis",
                    "FAILED_CHASSIS_ID", chassisId, "POWER_STATE",
                    currentState);

                // Set per-chassis flag to prevent repeated power off requests
                // from this chassis
                chassisFailureTriggered[chassisId] = true;

                // Set requested power transition to off so state is correct
                server::Chassis::requestedPowerTransition(Transition::Off);

                // Start the chassis 0 poweroff target
                startUnit(CHASSIS_POWEROFF_TARGET);

                // Request power off transition on all chassis instances
                requestTransitionOnAllChassis(Transition::Off);
            }

            chassisPowerStates[chassisId] = state;
            aggregatePowerState();
        }
        else if (property ==
                 server::Chassis::property_names::current_power_status)
        {
            auto statusStr = std::get<std::string>(value);
            PowerStatus status =
                server::Chassis::convertPowerStatusFromString(statusStr);

            chassisPowerStatus[chassisId] = status;
            aggregatePowerStatus();
        }
    }
}

void ChassisSMP::requestTransitionOnAllChassis(Transition transition)
{
    info("Chassis0: Forwarding transition request {TRANSITION} to "
         "all chassis instances",
         "TRANSITION", transition);

    for (size_t i = 1; i <= numChassis; ++i)
    {
        if (!chassisPresentStatus[i])
        {
            info("Chassis0: Skipping transition for chassis "
                 "{TARGET_CHASSIS_ID} because it is not present",
                 "TARGET_CHASSIS_ID", i);
            continue;
        }

        sdbusplus::object_path chassisPath = std::format(CHASSIS_OBJ_PATH, i);
        std::string chassisService = std::format(CHASSIS_SERVICE, i);

        try
        {
            std::string transitionStr = convertForMessage(transition);

            auto method = bus.new_method_call(
                chassisService.c_str(), chassisPath, PROPERTY_INTERFACE, "Set");
            method.append(
                server::Chassis::interface,
                server::Chassis::property_names::requested_power_transition,
                std::variant<std::string>(transitionStr));

            bus.call_noreply(method);

            debug("Chassis0: Forwarded transition to chassis "
                  "{TARGET_CHASSIS_ID}",
                  "TARGET_CHASSIS_ID", i);
        }
        catch (const sdbusplus::exception_t& e)
        {
            error("Chassis0: Failed to forward transition to chassis "
                  "{TARGET_CHASSIS_ID}: {ERROR}",
                  "TARGET_CHASSIS_ID", i, "ERROR", e);
        }
    }
}

ChassisSMP::Transition ChassisSMP::requestedPowerTransition(Transition value)
{
    info("Chassis0: SMP Aggregator received transition request: "
         "{TRANSITION}",
         "TRANSITION", value);

    // Reset per-chassis failure tracking when a new transition is requested
    // This allows the system to detect new failures after a power on attempt
    chassisFailureTriggered.clear();

    // Start the systemd target for chassis 0
    if (value == Transition::Off)
    {
        startUnit(CHASSIS_POWEROFF_TARGET);
    }
    else if (value == Transition::On)
    {
        startUnit(CHASSIS_POWERON_TARGET);
    }
    else if (value == Transition::PowerCycle)
    {
        startUnit(CHASSIS_POWERCYCLE_TARGET);
    }

    // Forward the transition request to all chassis instances
    requestTransitionOnAllChassis(value);

    return server::Chassis::requestedPowerTransition(value);
}

ChassisSMP::PowerState ChassisSMP::currentPowerState(PowerState value)
{
    info("Chassis0: SMP Aggregator power state set to: "
         "{POWER_STATE}",
         "POWER_STATE", value);
    return server::Chassis::currentPowerState(value);
}

bool ChassisSMP::isChassisPresent(size_t chassisId)
{
    constexpr auto inventoryBusName = "xyz.openbmc_project.Inventory.Manager";
    constexpr auto inventoryObjPathFmt =
        "/xyz/openbmc_project/inventory/system/chassis{}";

    sdbusplus::object_path inventoryPath =
        std::format(inventoryObjPathFmt, chassisId);

    try
    {
        auto method = bus.new_method_call(inventoryBusName, inventoryPath,
                                          PROPERTY_INTERFACE, "Get");
        method.append(InventoryItem::interface,
                      InventoryItem::property_names::present);

        auto response = bus.call(method);
        std::variant<bool> value;
        response.read(value);

        bool present = std::get<bool>(value);
        debug("Chassis0: Chassis {CHASSIS_ID} present status: "
              "{PRESENT}",
              "CHASSIS_ID", chassisId, "PRESENT", present);
        return present;
    }
    catch (const sdbusplus::exception_t& e)
    {
        debug("Chassis0: Could not read Present property for "
              "chassis {CHASSIS_ID}: {ERROR}",
              "CHASSIS_ID", chassisId, "ERROR", e.what());
        return false;
    }
}

void ChassisSMP::inventoryPresentChanged(sdbusplus::message_t& msg,
                                         size_t chassisId)
{
    std::string interface;
    std::map<std::string, std::variant<bool>> properties;

    msg.read(interface, properties);

    auto present = properties.find(InventoryItem::property_names::present);
    if (present == properties.end())
    {
        return;
    }

    bool isPresent = std::get<bool>(present->second);

    info("Chassis0: Chassis {TARGET_CHASSIS_ID} inventory presence "
         "changed to {PRESENT}",
         "TARGET_CHASSIS_ID", chassisId, "PRESENT", isPresent);

    // Update the cached present status
    chassisPresentStatus[chassisId] = isPresent;

    // If chassis became present, start monitoring its state properties
    if (isPresent)
    {
        // Only create a new match if we're not already monitoring this chassis
        if (chassisMatches.find(chassisId) == chassisMatches.end())
        {
            sdbusplus::object_path chassisPath =
                std::format("/xyz/openbmc_project/state/chassis{}", chassisId);

            auto match = std::make_unique<sdbusplus::match>(
                bus,
                sdbusRule::propertiesChanged(
                    chassisPath.string(), "xyz.openbmc_project.State.Chassis"),
                [this, chassisId](sdbusplus::message_t& msg) {
                    this->chassisPropertyChanged(msg, chassisId);
                });

            chassisMatches[chassisId] = std::move(match);

            info("Chassis0: Started monitoring chassis "
                 "{MONITORED_CHASSIS_ID}",
                 "MONITORED_CHASSIS_ID", chassisId);
        }
    }

    aggregatePowerState();
    aggregatePowerStatus();
}

void ChassisSMP::sysStateChangeJobNew(sdbusplus::message_t& msg)
{
    uint32_t newStateID{};
    sdbusplus::object_path newStateObjPath;
    std::string newStateUnit{};

    msg.read(newStateID, newStateObjPath, newStateUnit);

    // Check if the chassis 0 poweron target was started outside of this
    // application
    std::string poweronTarget =
        std::format("obmc-chassis-poweron@{}.target", std::to_string(0));

    if (newStateUnit == poweronTarget)
    {
        // Only initiate power on if our current requested power state is off
        // and our current power state is off
        auto currentRequestedTransition =
            server::Chassis::requestedPowerTransition();
        auto currentPowerState = server::Chassis::currentPowerState();
        if ((currentRequestedTransition == Transition::Off) &&
            (currentPowerState == PowerState::Off))
        {
            info("Chassis0: Chassis 0 poweron target started while "
                 "in state {POWER_STATE}, forwarding to all chassis instances",
                 "POWER_STATE", currentPowerState);

            requestedPowerTransition(Transition::On);
        }
        return;
    }

    // Check if the chassis 0 poweroff target was started
    if (newStateUnit == CHASSIS_POWEROFF_TARGET)
    {
        // Only initiate auto power off if we were actively trying to power on
        // and we have not already processed a request to power off
        auto currentState = server::Chassis::currentPowerState();
        auto currentRequestedTransition =
            server::Chassis::requestedPowerTransition();
        if ((currentState == PowerState::TransitioningToOn ||
             currentState == PowerState::On) &&
            (currentRequestedTransition != Transition::Off))
        {
            info("Chassis0: Chassis 0 poweroff target started while "
                 "in state {POWER_STATE}, initiating power off for all chassis "
                 "instances",
                 "POWER_STATE", currentState);

            requestedPowerTransition(Transition::Off);
        }
    }
}

} // namespace phosphor::state::manager
