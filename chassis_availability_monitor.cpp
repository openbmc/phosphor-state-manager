#include "chassis_availability_monitor.hpp"

#include "utils.hpp"

#include <nlohmann/json.hpp>
#include <phosphor-logging/lg2.hpp>

#include <fstream>
#include <stdexcept>

namespace phosphor::state::manager
{

PHOSPHOR_LOG2_USING;

using json = nlohmann::json;

ChassisAvailability::ChassisAvailability(sdbusplus::bus_t& bus,
                                         const std::string& configPath) :
    bus(bus), configPath(configPath)
{
    loadConfiguration();
}

void ChassisAvailability::loadConfiguration()
{
    try
    {
        std::ifstream fileStream(configPath);
        if (!fileStream.is_open())
        {
            throw std::runtime_error(
                "Failed to open configuration file: " + configPath);
        }

        auto config = json::parse(fileStream);
        availableObjectPathTemplate =
            config["availableObjectPath"].get<std::string>();

        for (const auto& cond : config["conditions"])
        {
            PropertyCondition condition;

            condition.baseObjectPath =
                cond["baseObjectPath"].get<std::string>();
            condition.interface = cond["interface"].get<std::string>();
            condition.property = cond["property"].get<std::string>();
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
            conditions.push_back(std::move(condition));
        }
        info("Loaded {COUNT} conditions", "COUNT", conditions.size());
    }
    catch (const std::exception& e)
    {
        error("Config error: {ERROR}", "ERROR", e);
        throw;
    }
}

} // namespace phosphor::state::manager
