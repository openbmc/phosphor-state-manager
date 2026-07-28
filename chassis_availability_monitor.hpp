#pragma once

#include <nlohmann/json.hpp>
#include <sdbusplus/bus.hpp>

#include <set>
#include <string>
#include <variant>
#include <vector>

namespace phosphor
{
namespace state
{
namespace manager
{

using json = nlohmann::json;
using SubTreeResponse =
    std::map<std::string, std::map<std::string, std::vector<std::string>>>;

/** @struct PropertyCondition
 * @brief Holds all the information needed to monitor one condition
 * from the JSON config file
 */
struct PropertyCondition
{
    std::string baseObjectPath;
    std::string interface;
    std::string property;
    std::variant<bool, std::string, int64_t> availableValue;
};

/** @class ChassisAvailability
 * @brief Monitors chassis availability based on configured D-Bus property
 * conditions
 * @details Reads a JSON config file to determine which D-Bus properties to
 * monitor
 */
class ChassisAvailability
{
  public:
    ChassisAvailability(sdbusplus::bus_t& bus, const std::string& configPath);
    ~ChassisAvailability() = default;

    ChassisAvailability(const ChassisAvailability&) = delete;
    ChassisAvailability& operator=(const ChassisAvailability&) = delete;
    ChassisAvailability(ChassisAvailability&&) = delete;
    ChassisAvailability& operator=(ChassisAvailability&&) = delete;

    /** @brief Get discovered chassis numbers (used for minimal testing of
     * getDiscoverChassis() method)
     * @return Set of discovered chassis numbers
     */
    const std::set<int>& getDiscoveredChassis() const
    {
        return discoveredChassisNumbers;
    }

  private:
    /** @brief Load and parse JSON configuration file */
    void loadConfiguration();

    /** @brief Discover all connected server chassis on a system */
    void discoverChassis();

    /** @brief Extract chassis number from D-bus object path
     * @param[in] path D-Bus object path to extract chassis number from
     * @return Chassis number if found, otherwise returns -1
     */
    static int getChassisNumber(const std::string& path);

    /** @brief Persistent sdbusplus D-Bus connection (marked as unused for now)
     */
    [[maybe_unused]] sdbusplus::bus_t& bus;

    /** @brief Path to JSON configuration file */
    std::string configPath;

    /** @brief Template for Available property object path (with <N>
     * placeholder) */
    std::string availableObjectPathTemplate;

    /** @brief List of conditions to monitor from JSON config */
    std::vector<PropertyCondition> conditions;

    /** @brief Set of discovered chassis numbers connected to system*/
    std::set<int> discoveredChassisNumbers;
};

} // namespace manager
} // namespace state
} // namespace phosphor
