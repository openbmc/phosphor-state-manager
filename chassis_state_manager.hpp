#pragma once

#include <sdbusplus/bus.hpp>
#include "xyz/openbmc_project/State/Chassis/server.hpp"

namespace phosphor
{
namespace state
{
namespace manager
{

/** @class Chassis
 *  @brief OpenBMC chassis state management implementation.
 *  @details A concrete implementation for xyz.openbmc_project.State.Chassis
 *  DBus API.
 */
class Chassis : public sdbusplus::server::object::object<
                sdbusplus::xyz::openbmc_project::State::server::Chassis>
{
    public:
        /** @brief Constructs Chassis State Manager
         *
         * @note This constructor passes 'true' to the base class in order to
         *       defer dbus object registration until we can run
         *       determineInitialState() and set our properties
         *
         * @param[in] bus       - The Dbus bus object
         * @param[in] instance  - The instance of this object
         * @param[in] objPath   - The Dbus object path
         */
        Chassis(sdbusplus::bus::bus& bus,
                const char* busName,
                const char* objPath) :
                sdbusplus::server::object::object<
                    sdbusplus::xyz::openbmc_project::State::server::Chassis>(
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
            subscribeToSystemdSignals();

            determineInitialState();

            // We deferred this until we could get our property correct
            this->emit_object_added();
        }

        /** @brief Set value of RequestedPowerTransition */
        Transition requestedPowerTransition(Transition value) override;

        /** @brief Set value of CurrentPowerState */
        PowerState currentPowerState(PowerState value) override;

    private:
        /** @brief Determine initial chassis state and set internally */
        void determineInitialState();

        /**
         * @brief subscribe to the systemd signals
         *
         * This object needs to capture when it's systemd targets complete
         * so it can keep it's state updated
         *
         **/
        void subscribeToSystemdSignals();

        /** @brief Execute the transition request
         *
         * This function calls the appropriate systemd target for the input
         * transition.
         *
         * @param[in] tranReq    - Transition requested
         */
        void executeTransition(Transition tranReq);

        /**
         * @brief Determine if target is powered off
         *
         * This function determines if the target is powered off and
         * helps prevent misleading log recorded states.
         *
         * @return boolean corresponding to host powered off
         **/
        bool poweredOff();

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

        /** @brief Persistent sdbusplus DBus connection. */
        sdbusplus::bus::bus& bus;

        /** @brief Used to subscribe to dbus systemd signals **/
        sdbusplus::server::match::match systemdSignals;
};

} // namespace manager
} // namespace state
} // namespace phosphor
