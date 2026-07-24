#pragma once

#include <sdbusplus/bus.hpp>

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
};

} // namespace phosphor::state::manager
