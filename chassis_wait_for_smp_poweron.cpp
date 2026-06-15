#include "config.h"

#include "chassis_wait_for_smp_poweron.hpp"

#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/exception.hpp>

#include <chrono>
#include <format>

namespace phosphor::state::manager
{

PHOSPHOR_LOG2_USING;

namespace server = sdbusplus::server::xyz::openbmc_project::state;
namespace sdbusRule = sdbusplus::bus::match::rules;

using PowerState = server::Chassis::PowerState;

constexpr auto PROPERTY_INTERFACE = "org.freedesktop.DBus.Properties";
constexpr auto INVENTORY_INTERFACE = "xyz.openbmc_project.Inventory.Item";
constexpr auto CHASSIS_INTERFACE = "xyz.openbmc_project.State.Chassis";

SMPChassisWaiter::SMPChassisWaiter(
    sdbusplus::bus_t& bus, sdeventplus::Event& event, size_t numChassis) :
    bus(bus), event(event), numChassis(numChassis)
{
    info("SMP Chassis Waiter: Monitoring {NUM_CHASSIS} chassis instances",
         "NUM_CHASSIS", numChassis);

    // Initialize monitoring for all chassis
    initializeMonitoring();

    // Do initial check - might already be done
    checkAllChassisReady();
}

int SMPChassisWaiter::run()
{
    return event.loop();
}

void SMPChassisWaiter::initializeMonitoring()
{
    for (size_t i = 1; i <= numChassis; ++i)
    {
        // Check if chassis is present (read once at startup)
        if (!isChassisPresent(i))
        {
            info("SMP Chassis Waiter: Chassis {CHASSIS_ID} is not "
                 "present, skipping",
                 "CHASSIS_ID", i);
            continue;
        }

        presentChassis.insert(i);

        // Monitor chassis CurrentPowerState property changes
        sdbusplus::object_path chassisPath =
            std::format("/xyz/openbmc_project/state/chassis{}", i);

        auto chassisMatch = std::make_unique<sdbusplus::bus::match_t>(
            bus,
            sdbusRule::propertiesChanged(chassisPath.str, CHASSIS_INTERFACE),
            [this, i](sdbusplus::message_t& msg) {
                chassisPowerStateChanged(msg, i);
            });

        chassisMatches.push_back(std::move(chassisMatch));

        // Get initial power state
        updateChassisPowerState(i);
    }

    info("SMP Chassis Waiter: Monitoring {NUM_PRESENT} present chassis",
         "NUM_PRESENT", presentChassis.size());
}

bool SMPChassisWaiter::isChassisPresent(size_t chassisId)
{
    constexpr auto inventoryBusName = "xyz.openbmc_project.Inventory.Manager";
    sdbusplus::object_path inventoryPath = std::format(
        "/xyz/openbmc_project/inventory/system/chassis{}", chassisId);

    // Pre-flight check: verify inventory service is available
    // This prevents unnecessary D-Bus error allocations in test environments
    try
    {
        auto method = bus.new_method_call(
            "org.freedesktop.DBus", "/org/freedesktop/DBus",
            "org.freedesktop.DBus", "GetNameOwner");
        method.append(inventoryBusName);

        auto response = bus.call(method);
        std::string owner;
        response.read(owner);

        if (owner.empty())
        {
            debug("Inventory service not running");
            return false;
        }
    }
    catch (const sdbusplus::exception_t& e)
    {
        debug("SMP Chassis Waiter: Inventory service unavailable for chassis "
              "{CHASSIS_ID}: {ERROR}",
              "CHASSIS_ID", chassisId, "ERROR", e.what());
        return false;
    }

    try
    {
        auto method = bus.new_method_call(inventoryBusName, inventoryPath.str,
                                          PROPERTY_INTERFACE, "Get");
        method.append(INVENTORY_INTERFACE, "Present");

        auto response = bus.call(method);
        std::variant<bool> value;
        response.read(value);

        return std::get<bool>(value);
    }
    catch (const sdbusplus::exception_t& e)
    {
        debug("SMP Chassis Waiter: Could not read Present property for "
              "chassis {CHASSIS_ID}: {ERROR}",
              "CHASSIS_ID", chassisId, "ERROR", e.what());
        return false;
    }
}

void SMPChassisWaiter::updateChassisPowerState(size_t chassisId)
{
    sdbusplus::object_path chassisPath =
        std::format("/xyz/openbmc_project/state/chassis{}", chassisId);
    std::string chassisService =
        std::format("xyz.openbmc_project.State.Chassis{}", chassisId);

    try
    {
        auto method = bus.new_method_call(
            chassisService.c_str(), chassisPath.str, PROPERTY_INTERFACE, "Get");
        method.append(CHASSIS_INTERFACE, "CurrentPowerState");

        auto reply = bus.call(method);
        std::variant<std::string> propertyValue;
        reply.read(propertyValue);

        auto stateStr = std::get<std::string>(propertyValue);
        auto state = server::Chassis::convertPowerStateFromString(stateStr);
        bool isOn = (state == PowerState::On);

        chassisPowerStates[chassisId] = isOn;

        debug("SMP Chassis Waiter: Chassis {CHASSIS_ID} power state: "
              "{STATE}",
              "CHASSIS_ID", chassisId, "STATE", stateStr);
    }
    catch (const sdbusplus::exception_t& e)
    {
        error("SMP Chassis Waiter: Failed to get power state for chassis "
              "{CHASSIS_ID}: {ERROR}",
              "CHASSIS_ID", chassisId, "ERROR", e.what());
        chassisPowerStates[chassisId] = false;
    }
}

void SMPChassisWaiter::chassisPowerStateChanged(sdbusplus::message_t& msg,
                                                size_t chassisId)
{
    std::string interface;
    std::map<std::string, std::variant<std::string>> properties;

    msg.read(interface, properties);

    auto stateIt = properties.find("CurrentPowerState");
    if (stateIt == properties.end())
    {
        return;
    }

    auto stateStr = std::get<std::string>(stateIt->second);
    auto state = server::Chassis::convertPowerStateFromString(stateStr);
    bool isOn = (state == PowerState::On);

    chassisPowerStates[chassisId] = isOn;

    info("SMP Chassis Waiter: Chassis {CHASSIS_ID} power state changed to "
         "{STATE}",
         "CHASSIS_ID", chassisId, "STATE", stateStr);

    checkAllChassisReady();
}

void SMPChassisWaiter::checkAllChassisReady()
{
    if (presentChassis.empty())
    {
        warning("SMP Chassis Waiter: No chassis are present, exiting with "
                "success");
        event.exit(0);
        return;
    }

    size_t numOn = 0;
    size_t numOff = 0;

    for (size_t chassisId : presentChassis)
    {
        auto it = chassisPowerStates.find(chassisId);
        if (it != chassisPowerStates.end() && it->second)
        {
            numOn++;
        }
        else
        {
            numOff++;
        }
    }

    info("SMP Chassis Waiter: Status - {NUM_ON} chassis on, {NUM_OFF} "
         "chassis off/unknown out of {NUM_PRESENT} present",
         "NUM_ON", numOn, "NUM_OFF", numOff, "NUM_PRESENT",
         presentChassis.size());

    if (numOn == presentChassis.size())
    {
        info("SMP Chassis Waiter: All {NUM_CHASSIS} present chassis are "
             "powered on, exiting with success",
             "NUM_CHASSIS", presentChassis.size());
        event.exit(0);
    }
}

} // namespace phosphor::state::manager

int main()
{
    using namespace phosphor::state::manager;

    auto bus = sdbusplus::bus::new_bus();
    auto event = sdeventplus::Event::get_new();

    // Attach the bus to sd_event to service user requests
    bus.attach_event(event.get(), SD_EVENT_PRIORITY_NORMAL);

    SMPChassisWaiter waiter(bus, event, NUM_CHASSIS_SMP);

    return waiter.run();
}
