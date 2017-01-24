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
                pgoodOn(bus,
                        "type='signal',member='PowerGood'",
                        Chassis::handlePgoodOn,
                        this),
                pgoodOff(bus,
                        "type='signal',member='PowerLost'",
                        Chassis::handlePgoodOff,
                        this)
        {
            determineInitialState();

            // We deferred this until we could get our property correct
            this->emit_object_added();
        }

        /** @brief Determine initial chassis state and set internally */
        void determineInitialState();

        /** @brief Set value of RequestedPowerTransition */
        Transition requestedPowerTransition(Transition value) override;

        /** @brief Set value of CurrentPowerState */
        PowerState currentPowerState(PowerState value) override;

    private:
        /** @brief Execute the transition request
         *
         * This function calls the appropriate systemd target for the input
         * transition.
         *
         * @param[in] tranReq    - Transition requested
         */
        void executeTransition(Transition tranReq);

        /** @brief Callback function for pgood going to on state
         *
         *  Update chassis object state to reflect pgood going to on state
         *
         * @param[in] msg        - Data associated with subscribed signal
         * @param[in] userData   - Pointer to this object instance
         * @param[in] retError   - Return error data
         *
         */
        static int handlePgoodOn(sd_bus_message* msg,
                                 void* userData,
                                 sd_bus_error* retError);

        /** @brief Callback function for pgood going to off state
         *
         *  Update chassis object state to reflect pgood going to off state
         *
         * @param[in] msg        - Data associated with subscribed signal
         * @param[in] userData   - Pointer to this object instance
         * @param[in] retError   - Return error data
         *
         */
        static int handlePgoodOff(sd_bus_message* msg,
                                  void* userData,
                                  sd_bus_error* retError);

        /** @brief Persistent sdbusplus DBus connection. */
        sdbusplus::bus::bus& bus;

        /** @brief Used to subscribe to dbus pgood on state changes */
        sdbusplus::server::match::match pgoodOn;

        /** @brief Used to subscribe to dbus pgood off state changes */
        sdbusplus::server::match::match pgoodOff;
};

} // namespace manager
} // namespace state
} // namespace phosphor
