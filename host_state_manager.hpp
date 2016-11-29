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
                            bus, objPath),
                path(objPath) {};

    private:

        /** @brief Path of the host instance */
        std::string path;
};

} // namespace manager
} // namespace state
} // namespace phosphor
