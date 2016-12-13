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
         * @param[in] busName   - The Dbus name to own
         * @param[in] objPath   - The Dbus object path
         */
        Chassis(sdbusplus::bus::bus& bus,
                const char* busName,
                const char* objPath);

        /** @brief Set value of RequestedPowerTransition */
        Transition requestedPowerTransition(Transition value) override;

        /** @brief Set value of CurrentPowerState */
        PowerState currentPowerState(PowerState value) override;

    private:

        /** @brief Persistent sdbusplus DBus bus connection. */
        sdbusplus::bus::bus& bus;

        /** @brief Path of the chassis instance */
        std::string path;
};

} // namespace manager
} // namespace state
} // namespace phosphor
