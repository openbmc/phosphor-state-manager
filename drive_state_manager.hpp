#pragma once

#include "config.h"

#include "utils.hpp"

#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/bus.hpp>
#include <xyz/openbmc_project/State/Drive/server.hpp>

#include <map>
#include <string>

namespace phosphor
{
namespace state
{
namespace manager
{

using DriveInherit = sdbusplus::server::object_t<
    sdbusplus::server::xyz::openbmc_project::state::Drive>;

namespace sdbusRule = sdbusplus::bus::match::rules;

/** @class Drive
 *  @brief OpenBMC drive state management implementation.
 *  @details A concrete implementation for xyz.openbmc_project.State.Drive
 *  DBus API.
 */
class Drive : public DriveInherit
{
  public:
    /** @brief Constructs Drive State Manager
     *
     *  @param[in] bus           - The Dbus bus object
     *  @param[in] objPath       - The Dbus object path
     *  @param[in] instanceName  - The drive instance name (e.g. nvme0)
     */
    Drive(sdbusplus::bus_t& bus, const char* objPath,
          std::string instanceName) :
        DriveInherit(bus, objPath, DriveInherit::action::defer_emit), bus(bus),
        systemdSignals(
            bus,
            sdbusRule::type::signal() + sdbusRule::member("JobRemoved") +
                sdbusRule::path(SYSTEMD_OBJ_PATH) +
                sdbusRule::interface(SYSTEMD_MANAGER_INTERFACE),
            [this](sdbusplus::message_t& m) { sysStateChangeJobRemoved(m); }),
        instanceName(std::move(instanceName))
    {
        utils::subscribeToSystemdSignals(bus);
        createSystemdTargetTable();
        determineInitialState();
        this->emit_object_added();
    }

    /** @brief Set value of RequestedDriveTransition */
    Transition requestedDriveTransition(Transition value) override;

    /** @brief Set value of CurrentDriveState */
    DriveState currentDriveState(DriveState value) override;

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

    sdbusplus::bus_t& bus;
    sdbusplus::bus::match_t systemdSignals;
    const std::string instanceName;
    std::map<Transition, std::string> systemdTargetTable;
};

} // namespace manager
} // namespace state
} // namespace phosphor
