#pragma once

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
        Host() = delete;
        ~Host() = default;
        Host(const Host&) = delete;
        Host& operator=(const Host&) = delete;
        Host(Host&&) = default;
        Host& operator=(Host&&) = delete;

        /** @brief Constructs Host State Manager
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
                stateSignal(bus,
                            "type='signal',member='GotoSystemState'",
                            handleSysStateChange,
                            this),
                tranActive(false)
        {
            // Will throw exception on fail
            determineInitialState();

            // We deferred this until we could get our property correct
            this->emit_object_added();
        }

        /**
         * @brief Determine initial host state and set internally
         *
         * @return Will throw exceptions on failure
         **/
        void determineInitialState();

        /** @brief Set value of HostTransition */
        Transition requestedHostTransition(Transition value) override;

        /** @brief Set value of CurrentHostState */
        HostState currentHostState(HostState value) override;

        /** @brief Verify the transition request is valid
         *
         * @param[in] tranReq    - Transition requested
         * @param[in] curState   - Current state of the object
         * @param[in] tranActive - Current transition activity of object
         */
        static bool verifyValidTransition(Transition tranReq,
                                          HostState curState,
                                          bool tranActive);

    private:

        /** @brief Execute the transition request
         *
         * This function assumes the state has been validated and the host
         * is in an appropriate state for the transition to be started.
         *
         * @param[in] tranReq    - Transition requested
         */
        void executeTransition(Transition tranReq);

        /** @brief Callback function on system state changes
         *
         *  Check if the state is relevant to the Host and if so, update
         *  corresponding host object's state
         *
         * @param[in] msg        - Data associated with subscribed signal
         * @param[in] userData   - Pointer to this object instance
         * @param[in] retError   - Return error data
         *
         */
        static int handleSysStateChange(sd_bus_message* msg,
                                        void* userData,
                                        sd_bus_error* retError);

        /** @brief Persistent sdbusplus DBus bus connection. */
        sdbusplus::bus::bus& bus;

        /** @brief Used to subscribe to dbus system state changes */
        sdbusplus::server::match::match stateSignal;

        /** @brief Indicates whether transition is actively executing */
        bool tranActive;
};

} // namespace manager
} // namespace state
} // namespace phosphor
