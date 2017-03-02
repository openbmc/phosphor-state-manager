#pragma once

#include <string>
#include <sdbusplus/bus.hpp>
#include "xyz/openbmc_project/State/Host/server.hpp"

namespace phosphor
{
namespace state
{
namespace manager
{

/** @class Host
 *  @brief OpenBMC host state management implementation.
 *  @details A concrete implementation for xyz.openbmc_project.State.Host
 *  DBus API.
 */
class Host : public sdbusplus::server::object::object<
                sdbusplus::xyz::openbmc_project::State::server::Host>
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
                sdbusplus::server::object::object<
                    sdbusplus::xyz::openbmc_project::State::server::Host>(
                            bus, objPath, true),
                bus(bus),
                systemdSignals(bus,
                               "type='signal',"
                               "member='JobRemoved',"
                               "path='/org/freedesktop/systemd1',"
                               "interface='org.freedesktop.systemd1.Manager'",
                                sysStateChangeSignal,
                                this)
        {
            // Enable systemd signals
            subscribeToSystemdSignals();

            // Will throw exception on fail
            determineInitialState();

            // We deferred this until we could get our property correct
            this->emit_object_added();
        }

        /** @brief Set value of HostTransition */
        Transition requestedHostTransition(Transition value) override;

        /** @brief Set value of CurrentHostState */
        HostState currentHostState(HostState value) override;

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

        /** @brief Callback function on systemd state changes
         *
         * Will just do a call into the appropriate object for processing
         *
         * @param[in]  msg       - Data associated with subscribed signal
         * @param[in]  userData  - Pointer to this object instance
         * @param[out] retError  - Not used but required with signal API
         *
         */
        static int sysStateChangeSignal(sd_bus_message* msg,
                                        void* userData,
                                        sd_bus_error* retError);

        /** @brief Check if systemd state change is relevant to this object
         *
         * Instance specific interface to handle the detected systemd state
         * change
         *
         * @param[in]  msg       - Data associated with subscribed signal
         * @param[out] retError  - Not used but required with signal API
         *
         */
        int sysStateChange(sd_bus_message* msg,
                           sd_bus_error* retError);

        /** @brief Persistent sdbusplus DBus bus connection. */
        sdbusplus::bus::bus& bus;

        /** @brief Used to subscribe to dbus systemd signals **/
        sdbusplus::server::match::match systemdSignals;
};

} // namespace manager
} // namespace state
} // namespace phosphor
