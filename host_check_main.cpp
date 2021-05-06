#include "config.h"

#include <unistd.h>

#include <phosphor-logging/log.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/exception.hpp>
#include <xyz/openbmc_project/Condition/HostFirmware/server.hpp>

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <vector>

using namespace std::literals;
using namespace phosphor::logging;
using namespace sdbusplus::xyz::openbmc_project::Condition::server;
using sdbusplus::exception::SdBusError;

// Required strings for sending the msg to check on host
constexpr auto MAPPER_BUSNAME = "xyz.openbmc_project.ObjectMapper";
constexpr auto MAPPER_PATH = "/xyz/openbmc_project/object_mapper";
constexpr auto MAPPER_INTERFACE = "xyz.openbmc_project.ObjectMapper";
constexpr auto CONDITION_HOST_INTERFACE =
    "xyz.openbmc_project.Condition.HostFirmware";
constexpr auto CONDITION_HOST_PROPERTY = "CurrentFirmwareCondition";
constexpr auto PROPERTY_INTERFACE = "org.freedesktop.DBus.Properties";

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
    catch (const SdBusError& e)
    {
        log<level::ERR>(
            "Error in mapper GetSubTree call for HostFirmware condition",
            entry("ERROR=%s", e.what()));
        throw;
    }

    if (mapperResponse.empty())
    {
        log<level::INFO>(
            "Mapper response for HostFirmware conditions is empty!");
        return false;
    }

    // Now read the CurrentFirmwareCondition from all interfaces we found
    for (const auto& [path, services] : mapperResponse)
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
            catch (const SdBusError& e)
            {
                log<level::ERR>("Error reading HostFirmware condition",
                                entry("ERROR=%s", e.what()),
                                entry("SERVICE=%s", service.c_str()),
                                entry("PATH=%s", path.c_str()));
                throw;
            }
        }
    }
    return false;
}

int main()
{
    log<level::INFO>("Check if host is running");

    auto bus = sdbusplus::bus::new_default();

    if (checkFirmwareConditionRunning(bus))
    {
        log<level::INFO>("Host is running!");
        // Create file for host instance and create in filesystem to indicate
        // to services that host is running
        auto size = std::snprintf(nullptr, 0, HOST_RUNNING_FILE, 0);
        size++; // null
        std::unique_ptr<char[]> buf(new char[size]);
        std::snprintf(buf.get(), size, HOST_RUNNING_FILE, 0);
        std::ofstream outfile(buf.get());
        outfile.close();
    }
    else
    {
        log<level::INFO>("Host is not running!");
    }

    return 0;
}
