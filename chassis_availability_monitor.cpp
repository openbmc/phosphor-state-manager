#include "chassis_availability_monitor.hpp"

#include <phosphor-logging/lg2.hpp>

#include <fstream>
#include <map>
#include <regex>
#include <stdexcept>

namespace phosphor
{
namespace state
{
namespace manager
{

PHOSPHOR_LOG2_USING;

ChassisAvailability::ChassisAvailability(sdbusplus::bus_t& bus,
                                         const std::string& configPath) :
    bus(bus), configPath(configPath)
{
    loadConfiguration();
    discoverChassis();
}

void ChassisAvailability::loadConfiguration()
{
    try
    {
        std::ifstream fileStream(configPath);
        if (!fileStream.is_open())
        {
            throw std::runtime_error("Failed to open configuration file.");
        }

        auto config = json::parse(fileStream);
        availableObjectPathTemplate = config["availableObjectPath"];

        for (const auto& cond : config["conditions"])
        {
            PropertyCondition condition;

            condition.baseObjectPath = cond["baseObjectPath"];
            condition.interface = cond["interface"];
            condition.property = cond["property"];
            const auto& val = cond["availableValue"];
            if (val.is_boolean())
            {
                condition.availableValue = val.get<bool>();
            }
            else if (val.is_string())
            {
                condition.availableValue = val.get<std::string>();
            }
            else if (val.is_number_integer())
            {
                condition.availableValue = val.get<int64_t>();
            }
            else
            {
                throw std::invalid_argument("Invalid availableValue type");
            }
            conditions.push_back(condition);
        }
        info("Loaded {COUNT} conditions", "COUNT", conditions.size());
    }
    catch (const std::exception& e)
    {
        error("Config error: {ERROR}", "ERROR", e.what());
        throw;
    }
}

void ChassisAvailability::discoverChassis()
{
    try
    {
        const std::string searchPath = "/xyz/openbmc_project/inventory";
        constexpr int searchDepth = 0;

        auto mapperCall = bus.new_method_call(
            "xyz.openbmc_project.ObjectMapper",
            "/xyz/openbmc_project/object_mapper",
            "xyz.openbmc_project.ObjectMapper", "GetSubTree");

        mapperCall.append(searchPath);
        mapperCall.append(searchDepth);
        mapperCall.append(std::vector<std::string>{});

        SubTreeResponse response;
        auto mapperReply = bus.call(mapperCall);
        mapperReply.read(response);

        for (const auto& [path, services] : response)
        {
            int chassisNum = getChassisNumber(path);
            if (chassisNum >= 0)
            {
                discoveredChassisNumbers.insert(chassisNum);
                info("Discovered chassis {NUM}", "NUM", chassisNum);
            }
        }
    }
    catch (const std::exception& e)
    {
        error("Chassis discovery failed: {ERROR}", "ERROR", e.what());
        throw;
    }
}

int ChassisAvailability::getChassisNumber(const std::string& path)
{
    try
    {
        std::regex pattern(R"(chassis(\d+))");
        std::smatch match;

        if (std::regex_search(path, match, pattern) && match.size() > 1)
        {
            return std::stoi(match[1].str());
        }
        return -1;
    }
    catch (...)
    {
        error("No chassis number found in path: {PATH}", "PATH", path);
        return -1;
    }
}

} // namespace manager
} // namespace state
} // namespace phosphor
