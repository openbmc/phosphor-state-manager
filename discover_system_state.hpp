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
                bus(bus)
        {
        }

    private:
        auto getBusName(std::string PATH, std::string INTERFACE);

        auto getProperty(std::string PATH, std::string INTERFACE,
            std::string PROPERTY);

        void setProperty(std::string PATH, std::string INTERFACE,
            std::string PROPERTY, std::string VALUE);

        /** @brief Persistent sdbusplus DBus bus connection. */
        sdbusplus::bus::bus& bus;

};

} // namespace manager
} // namespace state
} // namespace phosphor
