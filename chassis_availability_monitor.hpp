#pragma once

#include <sdbusplus/bus.hpp>
#include <sdbusplus/bus/match.hpp>

#include <map>
#include <memory>
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

/** @struct ChassisState
 * @brief Tracks the availability state of a single chassis
 */
struct ChassisState
{
    bool available = false;
    std::vector<std::unique_ptr<sdbusplus::bus::match_t>> propertyMatches;
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

    /** @brief Set up monitoring for a specific chassis
     * @param[in] chassisNum Chassis number to set up monitoring for
     */
    void setupMonitoringForChassis(int chassisNum);

    /** @brief Check and update availability for a chassis
     * @param[in] chassisNum Chassis number to check
     * @details Reads all condition properties from D-Bus, evaluates if ALL
     * conditions are met, and updates the Available property if changed.
     */
    void checkAvailability(int chassisNum);

    /** @brief Update Available property on D-Bus for a chassis
     * @param[in] chassisNum Chassis number to update
     * @param[in] isAvailable New availability value
     * @details Calls setProperty to update the Available property
     */
    void updateAvailableProperty(int chassisNum, bool isAvailable);

    /** @brief Extract chassis number from D-bus object path
     * @param[in] path D-Bus object path to extract chassis number from
     * @return Chassis number if found, otherwise returns std::nullopt
     */
    static std::optional<int> getChassisNumber(const std::string& path);

    /** @brief Substitute <N> placeholder with actual chassis number
     * @param[in] path Path template containing <N> placeholder
     * @param[in] chassisNum Chassis number to substitute
     * @return Path with <N> replaced by chassis number
     */
    static std::string substituteChassisNumber(const std::string& path,
                                               int chassisNum);

    /** @brief Persistent sdbusplus D-Bus connection
     */
    sdbusplus::bus_t& bus;

    /** @brief Path to JSON configuration file */
    std::string configPath;

    /** @brief Template for Available property object path (with <N>
     * placeholder) */
    std::string availableObjectPathTemplate;

    /** @brief List of conditions to monitor from JSON config */
    std::vector<PropertyCondition> conditions;

    /** @brief Set of discovered chassis numbers connected to system*/
    std::set<int> discoveredChassisNumbers;

    /** @brief State tracking for each discovered chassis
     * Key: Chassis number
     * Value: ChassisState containing availability and condition paths
     */
    std::map<int, ChassisState> chassisStates;

    /** @brief D-Bus signal matches for PropertiesChanged subscriptions */
    std::vector<std::unique_ptr<sdbusplus::bus::match_t>> propertyMatches;
};

} // namespace phosphor::state::manager
