#include "config.h"

#include "host_check.hpp"

#include <unistd.h>

#include <boost/range/adaptor/reversed.hpp>
#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/exception.hpp>
#include <xyz/openbmc_project/Condition/HostFirmware/server.hpp>

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <thread>
#include <vector>

namespace phosphor
{
namespace state
{
namespace manager
{

PHOSPHOR_LOG2_USING;

using namespace std::literals;
using namespace sdbusplus::xyz::openbmc_project::Condition::server;

// Required strings for sending the msg to check on host
constexpr auto MAPPER_BUSNAME = "xyz.openbmc_project.ObjectMapper";
constexpr auto MAPPER_PATH = "/xyz/openbmc_project/object_mapper";
constexpr auto MAPPER_INTERFACE = "xyz.openbmc_project.ObjectMapper";
constexpr auto CONDITION_HOST_INTERFACE =
    "xyz.openbmc_project.Condition.HostFirmware";
constexpr auto CONDITION_HOST_PROPERTY = "CurrentFirmwareCondition";
constexpr auto PROPERTY_INTERFACE = "org.freedesktop.DBus.Properties";

constexpr auto CHASSIS_STATE_SVC = "xyz.openbmc_project.State.Chassis";
constexpr auto CHASSIS_STATE_PATH = "/xyz/openbmc_project/state/chassis0";
constexpr auto CHASSIS_STATE_INTF = "xyz.openbmc_project.State.Chassis";
constexpr auto CHASSIS_STATE_POWER_PROP = "CurrentPowerState";

// Find all implementations of Condition interface and check if host is
// running over it
bool checkFirmwareConditionRunning(sdbusplus::bus::bus& bus)
{
    // Find all implementations of host firmware condition interface
    auto mapper = bus.new_method_call(MAPPER_BUSNAME, MAPPER_PATH,
                                      MAPPER_INTERFACE, "GetSubTree");

    mapper.append("/", 0, std::vector<std::string>({CONDITION_HOST_INTERFACE}));

    std::map<std::string, std::map<std::string, std::vector<std::string>>>
        mapperResponse;

    try
    {
        auto mapperResponseMsg = bus.call(mapper);
        mapperResponseMsg.read(mapperResponse);
    }
    catch (const sdbusplus::exception::exception& e)
    {
        error(
            "Error in mapper GetSubTree call for HostFirmware condition: {ERROR}",
            "ERROR", e);
        throw;
    }

    if (mapperResponse.empty())
    {
        info("Mapper response for HostFirmware conditions is empty!");
        return false;
    }

    // Now read the CurrentFirmwareCondition from all interfaces we found
    // Currently there are two implementations of this interface. One by IPMI
    // and one by PLDM. The IPMI interface does a realtime check with the host
    // when the interface is called. This means if the host is not running,
    // we will have to wait for the timeout (currently set to 3 seconds). The
    // PLDM interface reads a cached state. The PLDM service does not put itself
    // on D-Bus until it has checked with the host. Therefore it's most
    // efficient to call the PLDM interface first. Do that by going in reverse
    // of the interfaces returned to us (PLDM will be last if available)
    for (const auto& [path, services] :
         boost::adaptors::reverse(mapperResponse))
    {
        for (const auto& serviceIter : services)
        {
            const std::string& service = serviceIter.first;

            try
            {
                auto method = bus.new_method_call(service.c_str(), path.c_str(),
                                                  PROPERTY_INTERFACE, "Get");
                method.append(CONDITION_HOST_INTERFACE,
                              CONDITION_HOST_PROPERTY);

                auto response = bus.call(method);

                std::variant<std::string> currentFwCond;
                response.read(currentFwCond);

                if (std::get<std::string>(currentFwCond) ==
                    "xyz.openbmc_project.Condition.HostFirmware."
                    "FirmwareCondition."
                    "Running")
                {
                    return true;
                }
            }
            catch (const sdbusplus::exception::exception& e)
            {
                error("Error reading HostFirmware condition, error: {ERROR}, "
                      "service: {SERVICE} path: {PATH}",
                      "ERROR", e, "SERVICE", service, "PATH", path);
                throw;
            }
        }
    }
    return false;
}

// Helper function to check if chassis power is on
bool isChassiPowerOn(sdbusplus::bus::bus& bus)
{
    try
    {
        auto method = bus.new_method_call(CHASSIS_STATE_SVC, CHASSIS_STATE_PATH,
                                          PROPERTY_INTERFACE, "Get");
        method.append(CHASSIS_STATE_INTF, CHASSIS_STATE_POWER_PROP);

        auto response = bus.call(method);

        std::variant<std::string> currentPowerState;
        response.read(currentPowerState);

        if (std::get<std::string>(currentPowerState) ==
            "xyz.openbmc_project.State.Chassis.PowerState.On")
        {
            return true;
        }
    }
    catch (const sdbusplus::exception::exception& e)
    {
        error("Error reading Chassis Power State, error: {ERROR}, "
              "service: {SERVICE} path: {PATH}",
              "ERROR", e, "SERVICE", CHASSIS_STATE_SVC, "PATH",
              CHASSIS_STATE_PATH);
        throw;
    }
    return false;
}

bool isHostRunning()
{
    info("Check if host is running");

    auto bus = sdbusplus::bus::new_default();

    // No need to check if chassis power is not on
    if (!isChassiPowerOn(bus))
    {
        info("Chassis power not on, exit");
        return false;
    }

    // This applications systemd service is setup to only run after all other
    // application that could possibly implement the needed interface have
    // been started. However, the use of mapper to find those interfaces means
    // we have a condition where the interface may be on D-Bus but not stored
    // within mapper yet. Keep it simple and just build one retry into the
    // check if it's found the host is not up. This service is only called if
    // chassis power is on when the BMC comes up, so this wont impact most
    // normal cases where the BMC is rebooted with chassis power off. In
    // cases where chassis power is on, the host is likely running so we want
    // to be sure we check all interfaces
    for (int i = 0; i < 2; i++)
    {
        // Give mapper a small window to introspect new objects on bus
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        if (checkFirmwareConditionRunning(bus))
        {
            info("Host is running!");
            // Create file for host instance and create in filesystem to
            // indicate to services that host is running
            auto size = std::snprintf(nullptr, 0, HOST_RUNNING_FILE, 0);
            size++; // null
            std::unique_ptr<char[]> buf(new char[size]);
            std::snprintf(buf.get(), size, HOST_RUNNING_FILE, 0);
            std::ofstream outfile(buf.get());
            outfile.close();
            return true;
        }
    }
    info("Host is not running!");
    return false;
}

} // namespace manager
} // namespace state
} // namespace phosphor
