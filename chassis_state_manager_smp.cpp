#include "config.h"

#include "chassis_state_manager_smp.hpp"

#include <fmt/format.h>

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/exception.hpp>
#include <xyz/openbmc_project/Common/error.hpp>
#include <xyz/openbmc_project/Inventory/Item/common.hpp>

#include <fstream>
#include <iostream>
#include <map>
#include <string>

namespace phosphor
{
namespace state
{
namespace manager
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

ChassisSMP::ChassisSMP(sdbusplus::bus_t& bus, const char* objPath, size_t id,
                       size_t numChassis) :
    ChassisInherit(bus, objPath, ChassisInherit::action::defer_emit), bus(bus),
    id(id), numChassis(numChassis)
{
    if (id != 0)
    {
        throw std::invalid_argument(
            "ChassisSMP can only be used with chassis ID 0");
    }

    if (numChassis == 0)
    {
        throw std::invalid_argument(
            "ChassisSMP requires at least 1 chassis to aggregate");
    }

    info("Chassis{CHASSIS_ID}: Creating SMP aggregator for chassis 0, "
         "monitoring up to {NUM_CHASSIS} chassis instances",
         "CHASSIS_ID", id, "NUM_CHASSIS", numChassis);

    // Initialize cached states to Off/Good
    for (size_t i = 1; i <= numChassis; ++i)
    {
        chassisPowerStates[i] = PowerState::Off;
        chassisPowerStatus[i] = PowerStatus::Good;
    }

    // Set initial aggregated state
    currentPowerState(PowerState::Off);
    currentPowerStatus(PowerStatus::Good);

    createSystemdTargetTable();

    // Query actual chassis states before emitting object to prevent clients
    // from reading invalid default values
    startMonitoring();

    // Now that we have actual state, emit the object
    this->emit_object_added();
}

void ChassisSMP::createSystemdTargetTable()
{
    systemdTargetTable = {
        {Transition::Off,
         fmt::format("obmc-chassis-poweroff@{}.target", std::to_string(id))},
        {Transition::On,
         fmt::format("obmc-chassis-poweron@{}.target", std::to_string(id))},
        {Transition::PowerCycle,
         fmt::format("obmc-chassis-powercycle@{}.target", std::to_string(id))}};
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
    info("Chassis{CHASSIS_ID}: Starting monitoring of up to {NUM_CHASSIS} "
         "chassis instances",
         "CHASSIS_ID", id, "NUM_CHASSIS", numChassis);

    // Set up property change monitoring for all chassis
    for (size_t i = 1; i <= numChassis; ++i)
    {
        // Always register inventory present property monitor for all chassis
        // so we can detect when a chassis becomes present later
        sdbusplus::object_path inventoryPath =
            fmt::format("/xyz/openbmc_project/inventory/system/chassis{}", i);

        auto inventoryMatch = std::make_unique<sdbusplus::bus::match_t>(
            bus,
            sdbusRule::propertiesChanged(inventoryPath.str,
                                         "xyz.openbmc_project.Inventory.Item"),
            [this, i](sdbusplus::message_t& msg) {
                this->inventoryPresentChanged(msg, i);
            });

        inventoryPresentMatches.push_back(std::move(inventoryMatch));

        // Only monitor chassis state properties for chassis that are present
        if (!isChassisPresent(i))
        {
            info("Chassis{CHASSIS_ID}: Skipping state monitoring for chassis "
                 "{MONITORED_CHASSIS_ID} because it is not present",
                 "CHASSIS_ID", id, "MONITORED_CHASSIS_ID", i);
            continue;
        }

        sdbusplus::object_path chassisPath =
            fmt::format("/xyz/openbmc_project/state/chassis{}", i);

        auto match = std::make_unique<sdbusplus::bus::match_t>(
            bus,
            sdbusRule::propertiesChanged(chassisPath.str,
                                         "xyz.openbmc_project.State.Chassis"),
            [this, i](sdbusplus::message_t& msg) {
                this->chassisPropertyChanged(msg, i);
            });

        chassisMatches.push_back(std::move(match));

        info("Chassis{CHASSIS_ID}: Monitoring chassis {MONITORED_CHASSIS_ID}",
             "CHASSIS_ID", id, "MONITORED_CHASSIS_ID", i);
    }

    // Do initial aggregation
    aggregatePowerState();
    aggregatePowerStatus();
}

void ChassisSMP::aggregatePowerState()
{
    // Aggregate power state: Off if ANY chassis is off, On only if ALL are on
    PowerState aggregatedState = PowerState::On;

    bool foundPresentChassis = false;

    for (size_t i = 1; i <= numChassis; ++i)
    {
        if (!isChassisPresent(i))
        {
            continue;
        }

        foundPresentChassis = true;

        sdbusplus::object_path chassisPath =
            fmt::format("/xyz/openbmc_project/state/chassis{}", i);
        std::string chassisService =
            fmt::format("xyz.openbmc_project.State.Chassis{}", i);

        try
        {
            auto method =
                bus.new_method_call(chassisService.c_str(), chassisPath.str,
                                    PROPERTY_INTERFACE, "Get");
            method.append("xyz.openbmc_project.State.Chassis",
                          "CurrentPowerState");

            auto reply = bus.call(method);
            std::variant<std::string> propertyValue;
            reply.read(propertyValue);

            auto stateStr = std::get<std::string>(propertyValue);
            PowerState state = PowerState::Off;

            if (stateStr == "xyz.openbmc_project.State.Chassis.PowerState.On")
            {
                state = PowerState::On;
            }
            else if (stateStr ==
                     "xyz.openbmc_project.State.Chassis.PowerState.Off")
            {
                state = PowerState::Off;
            }

            chassisPowerStates[i] = state;

            // If any chassis is off, aggregate is off
            if (state == PowerState::Off)
            {
                aggregatedState = PowerState::Off;
            }
        }
        catch (const sdbusplus::exception_t& e)
        {
            error("Chassis{CHASSIS_ID}: Failed to get power state for chassis "
                  "{TARGET_CHASSIS_ID}: {ERROR}",
                  "CHASSIS_ID", id, "TARGET_CHASSIS_ID", i, "ERROR", e);
            // Assume off if we can't read the state
            chassisPowerStates[i] = PowerState::Off;
            aggregatedState = PowerState::Off;
        }
    }

    if (!foundPresentChassis)
    {
        aggregatedState = PowerState::Off;
    }

    if (server::Chassis::currentPowerState() != aggregatedState)
    {
        info("Chassis{CHASSIS_ID}: SMP Aggregator power state changing to: "
             "{POWER_STATE}",
             "CHASSIS_ID", id, "POWER_STATE", aggregatedState);
        currentPowerState(aggregatedState);
    }
}

void ChassisSMP::aggregatePowerStatus()
{
    // Aggregate power status: worst case (BrownOut > UPS > Good)
    PowerStatus aggregatedStatus = PowerStatus::Good;

    for (size_t i = 1; i <= numChassis; ++i)
    {
        if (!isChassisPresent(i))
        {
            continue;
        }

        sdbusplus::object_path chassisPath =
            fmt::format("/xyz/openbmc_project/state/chassis{}", i);
        std::string chassisService =
            fmt::format("xyz.openbmc_project.State.Chassis{}", i);

        try
        {
            auto method =
                bus.new_method_call(chassisService.c_str(), chassisPath.str,
                                    PROPERTY_INTERFACE, "Get");
            method.append("xyz.openbmc_project.State.Chassis",
                          "CurrentPowerStatus");

            auto reply = bus.call(method);
            std::variant<std::string> propertyValue;
            reply.read(propertyValue);

            auto statusStr = std::get<std::string>(propertyValue);
            PowerStatus status = PowerStatus::Good;

            if (statusStr ==
                "xyz.openbmc_project.State.Chassis.PowerStatus.BrownOut")
            {
                status = PowerStatus::BrownOut;
            }
            else if (statusStr == "xyz.openbmc_project.State.Chassis."
                                  "PowerStatus.UninterruptiblePowerSupply")
            {
                status = PowerStatus::UninterruptiblePowerSupply;
            }

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
            error("Chassis{CHASSIS_ID}: Failed to get power status for chassis "
                  "{TARGET_CHASSIS_ID}: {ERROR}",
                  "CHASSIS_ID", id, "TARGET_CHASSIS_ID", i, "ERROR", e);
            // Assume good if we can't read the status
            chassisPowerStatus[i] = PowerStatus::Good;
        }
    }

    if (server::Chassis::currentPowerStatus() != aggregatedStatus)
    {
        info("Chassis{CHASSIS_ID}: SMP Aggregator power status changing to: "
             "{POWER_STATUS}",
             "CHASSIS_ID", id, "POWER_STATUS", aggregatedStatus);
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
        if (property == "CurrentPowerState")
        {
            auto stateStr = std::get<std::string>(value);
            PowerState state = PowerState::Off;

            if (stateStr == "xyz.openbmc_project.State.Chassis.PowerState.On")
            {
                state = PowerState::On;
            }

            chassisPowerStates[chassisId] = state;
            aggregatePowerState();
        }
        else if (property == "CurrentPowerStatus")
        {
            auto statusStr = std::get<std::string>(value);
            PowerStatus status = PowerStatus::Good;

            if (statusStr ==
                "xyz.openbmc_project.State.Chassis.PowerStatus.BrownOut")
            {
                status = PowerStatus::BrownOut;
            }
            else if (statusStr == "xyz.openbmc_project.State.Chassis."
                                  "PowerStatus.UninterruptiblePowerSupply")
            {
                status = PowerStatus::UninterruptiblePowerSupply;
            }

            chassisPowerStatus[chassisId] = status;
            aggregatePowerStatus();
        }
    }
}

void ChassisSMP::requestTransitionOnAllChassis(Transition transition)
{
    info("Chassis{CHASSIS_ID}: Forwarding transition request {TRANSITION} to "
         "all chassis instances",
         "CHASSIS_ID", id, "TRANSITION", transition);

    for (size_t i = 1; i <= numChassis; ++i)
    {
        if (!isChassisPresent(i))
        {
            info("Chassis{CHASSIS_ID}: Skipping transition for chassis "
                 "{TARGET_CHASSIS_ID} because it is not present",
                 "CHASSIS_ID", id, "TARGET_CHASSIS_ID", i);
            continue;
        }

        sdbusplus::object_path chassisPath =
            fmt::format("/xyz/openbmc_project/state/chassis{}", i);
        std::string chassisService =
            fmt::format("xyz.openbmc_project.State.Chassis{}", i);

        try
        {
            std::string transitionStr;
            if (transition == Transition::Off)
            {
                transitionStr =
                    "xyz.openbmc_project.State.Chassis.Transition.Off";
            }
            else if (transition == Transition::On)
            {
                transitionStr =
                    "xyz.openbmc_project.State.Chassis.Transition.On";
            }
            else if (transition == Transition::PowerCycle)
            {
                transitionStr =
                    "xyz.openbmc_project.State.Chassis.Transition.PowerCycle";
            }

            auto method =
                bus.new_method_call(chassisService.c_str(), chassisPath.str,
                                    PROPERTY_INTERFACE, "Set");
            method.append("xyz.openbmc_project.State.Chassis",
                          "RequestedPowerTransition",
                          std::variant<std::string>(transitionStr));

            bus.call_noreply(method);

            info("Chassis{CHASSIS_ID}: Forwarded transition to chassis "
                 "{TARGET_CHASSIS_ID}",
                 "CHASSIS_ID", id, "TARGET_CHASSIS_ID", i);
        }
        catch (const sdbusplus::exception_t& e)
        {
            error(
                "Chassis{CHASSIS_ID}: Failed to forward transition to chassis "
                "{TARGET_CHASSIS_ID}: {ERROR}",
                "CHASSIS_ID", id, "TARGET_CHASSIS_ID", i, "ERROR", e);
        }
    }
}

ChassisSMP::Transition ChassisSMP::requestedPowerTransition(Transition value)
{
    info("Chassis{CHASSIS_ID}: SMP Aggregator received transition request: "
         "{TRANSITION}",
         "CHASSIS_ID", id, "TRANSITION", value);

    // Start the systemd target for chassis 0
    startUnit(systemdTargetTable.find(value)->second);

    // Forward the transition request to all chassis instances
    requestTransitionOnAllChassis(value);

    return server::Chassis::requestedPowerTransition(value);
}

ChassisSMP::PowerState ChassisSMP::currentPowerState(PowerState value)
{
    info("Chassis{CHASSIS_ID}: SMP Aggregator power state set to: "
         "{POWER_STATE}",
         "CHASSIS_ID", id, "POWER_STATE", value);
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
        auto method = bus.new_method_call(inventoryBusName, inventoryPath.str,
                                          PROPERTY_INTERFACE, "Get");
        method.append(InventoryItem::interface,
                      InventoryItem::property_names::present);

        auto response = bus.call(method);
        std::variant<bool> value;
        response.read(value);

        bool present = std::get<bool>(value);
        debug("Chassis{AGGREGATOR_ID}: Chassis {CHASSIS_ID} present status: "
              "{PRESENT}",
              "AGGREGATOR_ID", id, "CHASSIS_ID", chassisId, "PRESENT", present);
        return present;
    }
    catch (const sdbusplus::exception_t& e)
    {
        debug("Chassis{AGGREGATOR_ID}: Could not read Present property for "
              "chassis {CHASSIS_ID}: {ERROR}",
              "AGGREGATOR_ID", id, "CHASSIS_ID", chassisId, "ERROR", e.what());
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

    info("Chassis{CHASSIS_ID}: Chassis {TARGET_CHASSIS_ID} inventory presence "
         "changed to {PRESENT}",
         "CHASSIS_ID", id, "TARGET_CHASSIS_ID", chassisId, "PRESENT",
         isPresent);

    // If chassis became present, start monitoring its state properties
    if (isPresent)
    {
        sdbusplus::object_path chassisPath =
            fmt::format("/xyz/openbmc_project/state/chassis{}", chassisId);

        auto match = std::make_unique<sdbusplus::bus::match_t>(
            bus,
            sdbusRule::propertiesChanged(chassisPath.str,
                                         "xyz.openbmc_project.State.Chassis"),
            [this, chassisId](sdbusplus::message_t& msg) {
                this->chassisPropertyChanged(msg, chassisId);
            });

        chassisMatches.push_back(std::move(match));

        info("Chassis{CHASSIS_ID}: Started monitoring chassis "
             "{MONITORED_CHASSIS_ID}",
             "CHASSIS_ID", id, "MONITORED_CHASSIS_ID", chassisId);
    }

    aggregatePowerState();
    aggregatePowerStatus();
}

} // namespace manager
} // namespace state
} // namespace phosphor
