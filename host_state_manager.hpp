#pragma once

#include <string>
#include <functional>
#include <sdbusplus/bus.hpp>
#include <phosphor-logging/log.hpp>
#include <xyz/openbmc_project/State/Boot/Progress/server.hpp>
#include <xyz/openbmc_project/Control/Boot/RebootAttempts/server.hpp>
#include <xyz/openbmc_project/State/OperatingSystem/Status/server.hpp>
#include "xyz/openbmc_project/State/Host/server.hpp"
#include "settings.hpp"
#include "config.h"

namespace phosphor
{
namespace state
{
namespace manager
{

using HostInherit = sdbusplus::server::object::object<
        sdbusplus::xyz::openbmc_project::State::server::Host,
        sdbusplus::xyz::openbmc_project::State::Boot::server::Progress,
        sdbusplus::xyz::openbmc_project::Control::Boot::server::RebootAttempts,
        sdbusplus::xyz::openbmc_project::State::OperatingSystem::server::Status>;

using namespace phosphor::logging;

namespace sdbusRule = sdbusplus::bus::match::rules;

/** @class Host
 *  @brief OpenBMC host state management implementation.
 *  @details A concrete implementation for xyz.openbmc_project.State.Host
 *  DBus API.
 */
class Host : public HostInherit
{
    public:
        /** @brief Constructs Host State Manager
         *
         * @note This constructor passes 'true' to the base class in order to
         *       defer dbus object registration until we can run
         *       determineInitialState() and set our properties
         *
         * @param[in] bus       - The Dbus bus object
         * @param[in] busName   - The Dbus name to own
         * @param[in] objPath   - The Dbus object path
         */
        Host(sdbusplus::bus::bus& bus,
             const char* busName,
             const char* objPath) :
                HostInherit(bus, objPath, true),
                bus(bus),
                systemdSignals(
                        bus,
                        sdbusRule::type::signal() +
                        sdbusRule::member("JobRemoved") +
                        sdbusRule::path("/org/freedesktop/systemd1") +
                        sdbusRule::interface(
                                "org.freedesktop.systemd1.Manager"),
                        std::bind(std::mem_fn(&Host::sysStateChange),
                                  this, std::placeholders::_1)),
                settings(bus)
        {
            // Enable systemd signals
            subscribeToSystemdSignals();

            // Will throw exception on fail
            determineInitialState();

            attemptsLeft(BOOT_COUNT_MAX_ALLOWED);

            // We deferred this until we could get our property correct
            this->emit_object_added();
        }

        /** @brief Set value of HostTransition */
        Transition requestedHostTransition(Transition value) override;

        /** @brief Set value of CurrentHostState */
        HostState currentHostState(HostState value) override;

        /**
         * @brief Set host reboot count to default
         *
         * OpenBMC software controls the number of allowed reboot attempts so
         * any external set request of this property will be overridden by
         * this function and set to the default.
         *
         * The only code responsible for decrementing the boot count resides
         * within this process and that will use the sub class interface
         * directly
         *
         * @param[in] value      - Reboot count value, will be ignored
         */
        uint32_t attemptsLeft(uint32_t value) override
        {
            namespace reboot = sdbusplus::xyz::openbmc_project::Control::
                    Boot::server;
            log<level::DEBUG>("External request to reset reboot count");
            return (reboot::RebootAttempts::attemptsLeft(
                    BOOT_COUNT_MAX_ALLOWED));
        }

    private:
        /**
         * @brief subscribe to the systemd signals
         *
         * This object needs to capture when it's systemd targets complete
         * so it can keep it's state updated
         *
         **/
        void subscribeToSystemdSignals();

        /**
         * @brief Determine initial host state and set internally
         *
         * @return Will throw exceptions on failure
         **/
        void determineInitialState();

        /** @brief Execute the transition request
         *
         * This function assumes the state has been validated and the host
         * is in an appropriate state for the transition to be started.
         *
         * @param[in] tranReq    - Transition requested
         */
        void executeTransition(Transition tranReq);

        /**
         * @brief Determine if target is active
         *
         * This function determines if the target is active and
         * helps prevent misleading log recorded states.
         *
         * @param[in] target - Target string to check on
         *
         * @return boolean corresponding to state active
         **/
        bool stateActive(const std::string& target);

        /**
         * @brief Determine if auto reboot flag is set
         *
         * @return boolean corresponding to current auto_reboot setting
         **/
        bool isAutoReboot();

        /** @brief Check if systemd state change is relevant to this object
         *
         * Instance specific interface to handle the detected systemd state
         * change
         *
         * @param[in]  msg       - Data associated with subscribed signal
         *
         */
        void sysStateChange(sdbusplus::message::message& msg);

        /** @brief Determine whether restoring of host requested state is enabled
         *
         * @return boolean corresponding to restore setting
         */
        bool getStateRestoreSetting() const;

        /** @brief Decrement reboot count
         *
         * This is used internally to this application to decrement the boot
         * count on each boot attempt. The host will use the external
         * attemptsLeft() interface to reset the count when a boot is successful
         *
         * @return number of reboot count attempts left
         */
        uint32_t decrementRebootCount();

        /** @brief Persistent sdbusplus DBus bus connection. */
        sdbusplus::bus::bus& bus;

        /** @brief Used to subscribe to dbus systemd signals **/
        sdbusplus::bus::match_t systemdSignals;

        // Settings objects of interest
        settings::Objects settings;
};

} // namespace manager
} // namespace state
} // namespace phosphor
