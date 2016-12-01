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
                bus(bus),
                path(objPath)
        {
            // Will throw exception on fail
            determineInitialState();
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

    private:
        /** @brief Persistent sdbusplus DBus bus connection. */
        sdbusplus::bus::bus& bus;

        /** @brief Path of the host instance */
        std::string path;
};

} // namespace manager
} // namespace state
} // namespace phosphor
