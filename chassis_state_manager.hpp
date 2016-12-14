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
         * @param[in] bus       - The Dbus bus object
         * @param[in] instance  - The instance of this object
         * @param[in] objPath   - The Dbus object path
         */
        Chassis(sdbusplus::bus::bus& bus,
                int instance,
                const char* objPath);

        /** @brief Determine initial chassis state and set internally */
        void determineInitialState();

        /** @brief Set value of RequestedPowerTransition */
        Transition requestedPowerTransition(Transition value) override;

        /** @brief Set value of CurrentPowerState */
        PowerState currentPowerState(PowerState value) override;

    private:

        /** @brief Callback function for pgood going on
         *
         *  Update corresponding chassis object state
         *
         * @param[in] msg        - Data associated with subscribed signal
         * @param[in] userData   - Pointer to this object instance
         * @param[in] retError   - Return error data
         *
         */
        static int handlePgoodOn(sd_bus_message* msg,
                                 void* userData,
                                 sd_bus_error* retError);

        /** @brief Callback function for pgood going off
         *
         *  Update corresponding chassis object state
         *
         * @param[in] msg        - Data associated with subscribed signal
         * @param[in] userData   - Pointer to this object instance
         * @param[in] retError   - Return error data
         *
         */
        static int handlePgoodOff(sd_bus_message* msg,
                                  void* userData,
                                  sd_bus_error* retError);

        /** @brief Persistent sdbusplus DBus bus connection. */
        sdbusplus::bus::bus& bus;

        /** @brief Instance number of this chassis */
        int instance;

        /** @brief Used to subscribe to dbus pgood on state changes */
        sdbusplus::server::match::match pgoodOn;

        /** @brief Used to subscribe to dbus pgood off state changes */
        sdbusplus::server::match::match pgoodOff;
};

} // namespace manager
} // namespace state
} // namespace phosphor
