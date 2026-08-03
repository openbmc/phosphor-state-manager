#include "chassis_availability_monitor.hpp"

#include "utils.hpp"

#include <nlohmann/json.hpp>
#include <phosphor-logging/lg2.hpp>

#include <fstream>
#include <map>
#include <regex>
#include <stdexcept>

namespace phosphor::state::manager
{

PHOSPHOR_LOG2_USING;

using json = nlohmann::json;
using SubTreeResponse =
    std::map<std::string, std::map<std::string, std::vector<std::string>>>;

ChassisAvailability::ChassisAvailability(sdbusplus::bus_t& bus,
                                         const std::string& configPath) :
    bus(bus), configPath(configPath)
{
    loadConfiguration();
    discoverChassis();
    subscribeToChassisAdded();

    for (int chassisNum : discoveredChassisNumbers)
    {
        setupMonitoringForChassis(chassisNum);
    }
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

void ChassisAvailability::discoverChassis()
{
    try
    {
        const std::string searchPath = "/xyz/openbmc_project/inventory";
        constexpr int searchDepth = 0; // 0 is unlimited search depth
        const std::vector<std::string> searchInterface = {
            "xyz.openbmc_project.Inventory.Item.Chassis"};

        auto mapperCall = bus.new_method_call(
            "xyz.openbmc_project.ObjectMapper",
            "/xyz/openbmc_project/object_mapper",
            "xyz.openbmc_project.ObjectMapper", "GetSubTree");

        mapperCall.append(searchPath);
        mapperCall.append(searchDepth);
        mapperCall.append(searchInterface);

        SubTreeResponse response;
        auto mapperReply = bus.call(mapperCall);
        mapperReply.read(response);

        for (const auto& [path, services] : response)
        {
            auto chassisNum = getChassisNumber(path);
            if (chassisNum)
            {
                discoveredChassisNumbers.insert(*chassisNum);
                info("Discovered chassis {NUM}", "NUM", *chassisNum);
            }
        }
    }
    catch (const std::exception& e)
    {
        error("Chassis discovery failed: {ERROR}", "ERROR", e);
        throw;
    }
}

std::optional<int> ChassisAvailability::getChassisNumber(
    const std::string& path)
{
    try
    {
        std::regex pattern(R"(chassis(\d+)$)");
        std::smatch match;

        if (std::regex_search(path, match, pattern) && match.size() > 1)
        {
            return std::stoi(match[1].str());
        }
        return std::nullopt;
    }
    catch (...)
    {
        error("No chassis number found in path: {PATH}", "PATH", path);
        return std::nullopt;
    }
}

std::string ChassisAvailability::substituteChassisNumber(
    const std::string& path, int chassisNum)
{
    std::string result = path;
    const std::string placeholder = "<N>";
    size_t pos = result.find(placeholder);

    if (pos != std::string::npos)
    {
        result.replace(pos, placeholder.length(), std::to_string(chassisNum));
    }

    return result;
}

void ChassisAvailability::setupMonitoringForChassis(int chassisNum)
{
    info("Setting up monitoring for chassis {NUM}", "NUM", chassisNum);

    chassisStates[chassisNum] = ChassisState();

    for (const auto& condition : conditions)
    {
        // Replace <N> with actual chassis number
        std::string objectPath =
            substituteChassisNumber(condition.baseObjectPath, chassisNum);

        // Subscribe to PropertiesChanged signal
        auto matchRule = sdbusplus::bus::match::rules::propertiesChanged(
            objectPath, condition.interface);

        auto match = std::make_unique<sdbusplus::bus::match_t>(
            bus, matchRule, [this, chassisNum](sdbusplus::message_t& /*msg*/) {
                checkAvailability(chassisNum);
            });

        propertyMatches.push_back(std::move(match));

        info("Monitoring chassis {NUM} property {IFACE}.{PROP}", "NUM",
             chassisNum, "IFACE", condition.interface, "PROP",
             condition.property);
    }

    checkAvailability(chassisNum);
}

void ChassisAvailability::checkAvailability(int chassisNum)
{
    info("Checking availability for chassis {NUM}", "NUM", chassisNum);

    bool allConditionsMet = true;

    for (const auto& condition : conditions)
    {
        const auto objectPath =
            substituteChassisNumber(condition.baseObjectPath, chassisNum);

        try
        {
            std::string service =
                utils::getService(bus, objectPath, condition.interface);

            auto propCall = bus.new_method_call(
                service.c_str(), objectPath.c_str(), PROPERTY_INTERFACE, "Get");

            propCall.append(condition.interface);
            propCall.append(condition.property);

            auto propReply = bus.call(propCall);
            auto value =
                propReply.unpack<std::variant<bool, std::string, int64_t>>();

            if (value != condition.availableValue)
            {
                info(
                    "Chassis {NUM} condition not met: {IFACE}.{PROP} value mismatch",
                    "NUM", chassisNum, "IFACE", condition.interface, "PROP",
                    condition.property);
                allConditionsMet = false;
                break;
            }
        }
        catch (const std::exception& e)
        {
            error(
                "Failed to read chassis {NUM} property {IFACE}.{PROP}: {ERROR}",
                "NUM", chassisNum, "IFACE", condition.interface, "PROP",
                condition.property, "ERROR", e.what());
            allConditionsMet = false;
            break;
        }
    }

    bool previousAvailability = chassisStates[chassisNum].available;
    chassisStates[chassisNum].available = allConditionsMet;

    if (previousAvailability != allConditionsMet)
    {
        info("Chassis {NUM} availability changed to {AVAIL}", "NUM", chassisNum,
             "AVAIL", allConditionsMet);
        updateAvailableProperty(chassisNum, allConditionsMet);
    }
}

void ChassisAvailability::updateAvailableProperty(int chassisNum,
                                                  bool isAvailable)
{
    try
    {
        std::string objectPath =
            substituteChassisNumber(availableObjectPathTemplate, chassisNum);

        utils::setProperty(bus, objectPath,
                           "xyz.openbmc_project.State.Decorator.Availability",
                           "Available", isAvailable);
    }
    catch (const std::exception& e)
    {
        error("Failed to update Available property for chassis {NUM}: {ERROR}",
              "NUM", chassisNum, "ERROR", e);
    }
}

void ChassisAvailability::subscribeToChassisAdded()
{
    auto matchRule = sdbusplus::bus::match::rules::interfacesAdded(
        "/xyz/openbmc_project/inventory");

    chassisAddedMatch = std::make_unique<sdbusplus::bus::match_t>(
        bus, matchRule,
        [this](sdbusplus::message_t& msg) { onChassisAdded(msg); });
}

void ChassisAvailability::onChassisAdded(sdbusplus::message_t& msg)
{
    using InterfaceMap = std::map<
        std::string,
        std::map<std::string, std::variant<bool, std::string, int64_t>>>;
    auto result = msg.unpack<sdbusplus::object_path, InterfaceMap>();
    const auto& [objectPath, interfaces] = result;

    auto chassisNum = getChassisNumber(objectPath.str);
    if (chassisNum &&
        !std::ranges::contains(discoveredChassisNumbers, *chassisNum))
    {
        auto hasRequiredInterface = std::ranges::any_of(
            conditions, [ifaces = interfaces](const auto& condition) {
                return ifaces.contains(condition.interface);
            });

        if (hasRequiredInterface)
        {
            info("New chassis {NUM} detected", "NUM", *chassisNum);
            discoveredChassisNumbers.insert(*chassisNum);
            setupMonitoringForChassis(*chassisNum);
        }
    }
}

} // namespace phosphor::state::manager
