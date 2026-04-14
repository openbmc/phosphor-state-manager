#pragma once

#include <sdbusplus/bus.hpp>
#include <sdbusplus/bus/match.hpp>
#include <sdeventplus/event.hpp>
#include <sdeventplus/source/signal.hpp>

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace phosphor
{
namespace state
{
namespace manager
{

/**
 * @class SMPChassisWaiter
 * @brief Waits for all SMP chassis instances to reach PowerState::On
 *
 * This service monitors chassis instances 1 through N in an SMP system and
 * exits successfully only when all present chassis have reached the On state.
 * This ensures that host services depending on obmc-power-on@0.target don't
 * start until all physical chassis hardware is actually powered on.
 */
class SMPChassisWaiter
{
  public:
    SMPChassisWaiter() = delete;
    SMPChassisWaiter(const SMPChassisWaiter&) = delete;
    SMPChassisWaiter& operator=(const SMPChassisWaiter&) = delete;
    SMPChassisWaiter(SMPChassisWaiter&&) = delete;
    SMPChassisWaiter& operator=(SMPChassisWaiter&&) = delete;
    ~SMPChassisWaiter() = default;

    /**
     * @brief Constructor
     *
     * @param[in] bus - D-Bus connection
     * @param[in] event - Event loop
     * @param[in] numChassis - Number of chassis to monitor (1-N)
     */
    SMPChassisWaiter(sdbusplus::bus_t& bus, sdeventplus::Event& event,
                     size_t numChassis);

    /**
     * @brief Run the event loop
     *
     * @return Exit code (0 = success, all chassis powered on)
     */
    int run();

  private:
    /**
     * @brief Initialize monitoring for all chassis instances
     */
    void initializeMonitoring();

    /**
     * @brief Check if a chassis is present in inventory
     *
     * @param[in] chassisId - Chassis ID to check
     * @return true if present, false otherwise
     */
    bool isChassisPresent(size_t chassisId);

    /**
     * @brief Update cached power state for a chassis
     *
     * @param[in] chassisId - Chassis ID to update
     */
    void updateChassisPowerState(size_t chassisId);

    /**
     * @brief Handle chassis CurrentPowerState property changes
     *
     * @param[in] msg - D-Bus message
     * @param[in] chassisId - Chassis ID that changed
     */
    void chassisPowerStateChanged(sdbusplus::message_t& msg, size_t chassisId);

    /**
     * @brief Check if all present chassis are powered on
     *
     * If all present chassis are on, exit the event loop with success.
     */
    void checkAllChassisReady();

    /** @brief D-Bus connection */
    sdbusplus::bus_t& bus;

    /** @brief Event loop */
    sdeventplus::Event& event;

    /** @brief Number of chassis to monitor */
    const size_t numChassis;

    /** @brief Set of present chassis IDs */
    std::set<size_t> presentChassis;

    /** @brief Map of chassis ID to power state (true = On) */
    std::map<size_t, bool> chassisPowerStates;

    /** @brief D-Bus matches for chassis property changes */
    std::vector<std::unique_ptr<sdbusplus::bus::match_t>> chassisMatches;

    /** @brief Signal source for SIGINT */
    std::unique_ptr<sdeventplus::source::Signal> sigintSource;

    /** @brief Signal source for SIGTERM */
    std::unique_ptr<sdeventplus::source::Signal> sigtermSource;
};

} // namespace manager
} // namespace state
} // namespace phosphor
