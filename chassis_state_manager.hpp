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
                            bus, objPath,true),
                bus(bus)
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
        /** @brief Persistent sdbusplus DBus connection. */
        sdbusplus::bus::bus& bus;
};

} // namespace manager
} // namespace state
} // namespace phosphor
