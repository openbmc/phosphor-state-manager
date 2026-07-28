#pragma once

#include <sdbusplus/bus.hpp>

#include <optional>
#include <set>
#include <string>
#include <variant>
#include <vector>

namespace phosphor::state::manager
{

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
 * monitor.
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

  private:
    /** @brief Load and parse JSON configuration file */
    void loadConfiguration();

    /** @brief Discover all connected server chassis on a system */
    void discoverChassis();

    /** @brief Extract chassis number from D-bus object path
     * @param[in] path D-Bus object path to extract chassis number from
     * @return Chassis number if found, otherwise returns std::nullopt
     */
    static std::optional<int> getChassisNumber(const std::string& path);

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

} // namespace phosphor::state::manager
