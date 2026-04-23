#pragma once

#include "utils.hpp"

#include <sdbusplus/bus.hpp>
#include <xyz/openbmc_project/Association/Definitions/server.hpp>
#include <xyz/openbmc_project/State/Drive/server.hpp>

#include <map>
#include <memory>
#include <optional>
#include <string>

namespace phosphor
{
namespace state
{
namespace manager
{

using DriveInherit = sdbusplus::server::object_t<
    sdbusplus::server::xyz::openbmc_project::state::Drive,
    sdbusplus::server::xyz::openbmc_project::association::Definitions>;

namespace sdbusRule = sdbusplus::bus::match::rules;

/** @class Drive
 *  @brief OpenBMC drive state management implementation.
 *  @details A concrete implementation for xyz.openbmc_project.State.Drive
 *  DBus API. Each instance represents one physical drive discovered
 *  through entity-manager.
 */
class Drive : public DriveInherit
{
  public:
    /** @brief Constructs Drive State Manager
     *
     *  @note This constructor passes 'true' to the base class in order to
     *        defer dbus object registration until we can run
     *        determineInitialState() and set our properties
     *
     *  @param[in] bus           - The Dbus bus object
     *  @param[in] objPath       - The Dbus object path
     *  @param[in] instanceName  - The drive instance name (e.g. nvme0)
     *  @param[in] inventoryPath - The entity-manager inventory path
     *  @param[in] hostId        - Host index from ManagedHost, or nullopt
     */
    Drive(sdbusplus::bus_t& bus, const char* objPath,
          const std::string& instanceName, const std::string& inventoryPath,
          std::optional<size_t> hostId);

    /** @brief Set value of RequestedDriveTransition */
    Transition requestedDriveTransition(Transition value) override;

    /** @brief Set value of CurrentDriveState */
    DriveState currentDriveState(DriveState value) override;

    /** @brief Get the instance name */
    const std::string& getInstanceName() const
    {
        return instanceName;
    }

  private:
    /** @brief Create the systemd target to transition mapping */
    void createSystemdTargetTable();

    /** @brief Determine initial drive state based on systemd targets */
    void determineInitialState();

    /** @brief Start the systemd unit */
    void startUnit(const std::string& sysdUnit);

    /** @brief Check if systemd target is active */
    bool stateActive(const std::string& target);

    /** @brief Execute the requested transition */
    void executeTransition(Transition value);

    /** @brief Callback for systemd JobRemoved signal */
    void sysStateChangeJobRemoved(sdbusplus::message_t& msg);

    /** @brief Set up host state monitoring if hostId is set */
    void setupHostStateMonitoring();

    /** @brief Read current host state from D-Bus and update cache */
    void readHostState();

    /** @brief Check if host is running (from cache) */
    bool isHostRunning() const;

    sdbusplus::bus_t& bus;
    sdbusplus::bus::match_t systemdSignals;
    const std::string instanceName;
    const std::string inventoryPath;
    const std::optional<size_t> hostId;
    std::map<Transition, std::string> systemdTargetTable;

    /** @brief Cached host state for fast transition validation */
    bool hostRunning = false;

    /** @brief Match for host PropertiesChanged signal */
    std::unique_ptr<sdbusplus::bus::match_t> hostStateMatch;

    /** @brief Match for host state manager NameOwnerChanged (restart) */
    std::unique_ptr<sdbusplus::bus::match_t> hostOwnerMatch;
};

} // namespace manager
} // namespace state
} // namespace phosphor
