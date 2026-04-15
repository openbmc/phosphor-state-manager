#pragma once

#include "config.h"

#include <sdbusplus/bus.hpp>
#include <sdeventplus/clock.hpp>
#include <sdeventplus/event.hpp>
#include <sdeventplus/utility/timer.hpp>
#include <xyz/openbmc_project/State/Chassis/server.hpp>
#include <xyz/openbmc_project/State/PowerOnHours/server.hpp>

#include <map>
#include <memory>
#include <vector>

namespace phosphor
{
namespace state
{
namespace manager
{

using ChassisInherit = sdbusplus::server::object_t<
    sdbusplus::server::xyz::openbmc_project::state::Chassis,
    sdbusplus::server::xyz::openbmc_project::state::PowerOnHours>;
namespace sdbusRule = sdbusplus::bus::match::rules;

/** @class ChassisSMP
 *  @brief Multi-chassis SMP aggregator for chassis state management.
 *  @details Aggregates state information from chassis instances 1-N and
 *           presents it on chassis instance 0. This class overrides the
 *           normal chassis_state_manager behavior for instance 0 to only
 *           aggregate data from other chassis instances.
 */
class ChassisSMP : public ChassisInherit
{
  public:
    /** @brief Constructs Chassis SMP State Aggregator
     *
     * @note This constructor passes 'true' to the base class in order to
     *       defer dbus object registration until we can determine initial
     *       state from other chassis instances
     *
     * @param[in] bus       - The Dbus bus object
     * @param[in] objPath   - The Dbus object path
     * @param[in] id        - Chassis id (should be 0)
     * @param[in] numChassis - Number of chassis instances to aggregate (1-N)
     */
    ChassisSMP(sdbusplus::bus_t& bus, const char* objPath, size_t id,
               size_t numChassis);

    /** @brief Set value of RequestedPowerTransition */
    Transition requestedPowerTransition(Transition value) override;

    /** @brief Set value of CurrentPowerState */
    PowerState currentPowerState(PowerState value) override;

    /** @brief Start monitoring chassis instances */
    void startMonitoring();

  private:
    /** @brief Handle systemd JobNew signals for chassis 0 targets
     *
     * This ensures that when systemd targets are started directly (not via
     * the D-Bus API), the transition is still forwarded to all chassis
     * instances to maintain SMP coordination. Monitors both poweron and
     * poweroff targets for chassis 0.
     *
     * @param[in] msg - D-Bus message containing job information
     */
    void sysStateChangeJobNew(sdbusplus::message_t& msg);
    /** @brief Create systemd target instance names and mapping table */
    void createSystemdTargetTable();

    /** @brief Start the systemd unit requested
     *
     * This function calls `StartUnit` on the systemd unit given.
     *
     * @param[in] sysdUnit    - Systemd unit
     */
    void startUnit(const std::string& sysdUnit);

    /** @brief Aggregate power state from all chassis instances
     *
     * Determines the overall power state by checking all chassis instances.
     * If any chassis is On, the aggregate state is On.
     */
    void aggregatePowerState();

    /** @brief Aggregate power status from all chassis instances
     *
     * Determines the overall power status by checking all chassis instances.
     * Uses the worst-case status (e.g., if any chassis has bad power, report
     * bad).
     */
    void aggregatePowerStatus();

    /** @brief Handle property changes from monitored chassis instances
     *
     * @param[in] msg - D-Bus message containing property changes
     * @param[in] chassisId - ID of the chassis that changed
     */
    void chassisPropertyChanged(sdbusplus::message_t& msg, size_t chassisId);

    /** @brief Send power transition request to all chassis instances
     *
     * @param[in] transition - The requested transition
     */
    void requestTransitionOnAllChassis(Transition transition);

    /** @brief Check if a chassis is present in the inventory
     *
     * @param[in] chassisId - ID of the chassis to check
     * @return true if chassis is present, false otherwise
     */
    bool isChassisPresent(size_t chassisId);

    /** @brief Handle inventory Present property changes
     *
     * @param[in] msg - D-Bus message containing property changes
     * @param[in] chassisId - ID of the chassis that changed
     */
    void inventoryPresentChanged(sdbusplus::message_t& msg, size_t chassisId);

    /** @brief Persistent sdbusplus DBus connection. */
    sdbusplus::bus_t& bus;

    /** @brief Chassis id (should always be 0 for SMP aggregator). **/
    const size_t id;

    /** @brief Number of chassis instances to aggregate (1-N). **/
    const size_t numChassis;

    /** @brief Transition state to systemd target mapping table. **/
    std::map<Transition, std::string> systemdTargetTable;

    /** @brief Property change signal matches for each chassis instance. **/
    std::vector<std::unique_ptr<sdbusplus::bus::match_t>> chassisMatches;

    /** @brief Inventory Present property change signal matches. **/
    std::vector<std::unique_ptr<sdbusplus::bus::match_t>>
        inventoryPresentMatches;

    /** @brief Systemd JobNew signal match for chassis 0 target monitoring. **/
    std::unique_ptr<sdbusplus::bus::match_t> systemdSignalJobNew;

    /** @brief Cached power states from each chassis instance. **/
    std::map<size_t, PowerState> chassisPowerStates;

    /** @brief Cached power status from each chassis instance. **/
    std::map<size_t, PowerStatus> chassisPowerStatus;

    /** @brief Flag to track if we've initiated a coordinated power off due to
     * failure. Prevents repeated power off requests as each chassis transitions
     * to off. **/
    bool coordinatedPowerOffInProgress = false;
};

} // namespace manager
} // namespace state
} // namespace phosphor
