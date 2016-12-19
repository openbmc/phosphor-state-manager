#pragma once

#include <sdbusplus/bus.hpp>
#include "xyz/openbmc_project/State/BMC/server.hpp"

namespace phosphor
{
namespace state
{
namespace manager
{

/** @class BMC
 *  @brief OpenBMC BMC state management implementation.
 *  @details A concrete implementation for xyz.openbmc_project.State.BMC
 *  DBus API.
 */
class BMC : public sdbusplus::server::object::object<
                sdbusplus::xyz::openbmc_project::State::server::BMC>
{
    public:
        /** @brief Constructs BMC State Manager
         *
         * @param[in] bus       - The Dbus bus object
         * @param[in] busName   - The Dbus name to own
         * @param[in] objPath   - The Dbus object path
         */
        BMC(sdbusplus::bus::bus& bus,
             const char* objPath) :
             sdbusplus::server::object::object<
                 sdbusplus::xyz::openbmc_project::State::server::BMC>(
                         bus, objPath) {};

};

} // namespace manager
} // namespace state
} // namespace phosphor
