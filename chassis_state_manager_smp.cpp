#include "config.h"

#include "chassis_state_manager_smp.hpp"

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/exception.hpp>
#include <xyz/openbmc_project/Common/error.hpp>

#include <format>
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
    numChassis(numChassis)
{
    if (numChassis == 0)
    {
        throw std::invalid_argument(
            "ChassisSMP requires at least 1 chassis to aggregate");
    }

    info("Chassis0: Creating SMP aggregator for chassis 0, "
         "monitoring {NUM_CHASSIS} chassis instances",
         "NUM_CHASSIS", numChassis);

    // Initialize cached states to Off/Good
    for (size_t i = 1; i <= numChassis; ++i)
    {
        chassisPowerStates[i] = PowerState::Off;
        chassisPowerStatus[i] = PowerStatus::Good;
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
    // Set up property change monitoring for each chassis instance
    for (size_t i = 1; i <= numChassis; ++i)
    {
        sdbusplus::object_path chassisPath = std::format(CHASSIS_OBJ_PATH, i);
        std::string chassisService = std::format(CHASSIS_SERVICE, i);

        auto match = std::make_unique<sdbusplus::bus::match_t>(
            bus,
            sdbusRule::propertiesChanged(chassisPath.str,
                                         server::Chassis::interface),
            [this, i](sdbusplus::message_t& msg) {
                this->chassisPropertyChanged(msg, i);
            });

        chassisMatches.push_back(std::move(match));

        debug("Chassis0: Monitoring chassis {MONITORED_CHASSIS_ID}",
              "MONITORED_CHASSIS_ID", i);
    }

    // Do initial aggregation
    aggregatePowerState();
    aggregatePowerStatus();
}

void ChassisSMP::aggregatePowerState()
{
    // Aggregate power state: Off if ANY chassis is off, On only if ALL are on
    PowerState aggregatedState = PowerState::On;

    for (size_t i = 1; i <= numChassis; ++i)
    {
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

            // If any chassis is off, aggregate is off
            if (state == PowerState::Off)
            {
                aggregatedState = PowerState::Off;
            }
        }
        catch (const sdbusplus::exception_t& e)
        {
            error("Chassis0: Failed to get power state for chassis "
                  "{TARGET_CHASSIS_ID}: {ERROR}",
                  "TARGET_CHASSIS_ID", i, "ERROR", e);
            // Assume off if we can't read the state
            chassisPowerStates[i] = PowerState::Off;
            aggregatedState = PowerState::Off;
        }
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
            error(
                "Chassis0: Failed to forward transition to chassis "
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

} // namespace manager
} // namespace state
} // namespace phosphor
