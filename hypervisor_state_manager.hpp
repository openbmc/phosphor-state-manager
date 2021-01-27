#pragma once

#include "config.h"

#include "settings.hpp"
#include "xyz/openbmc_project/State/Host/server.hpp"

#include <sdbusplus/bus.hpp>

namespace phosphor
{
namespace state
{
namespace manager
{

using HypervisorInherit = sdbusplus::server::object::object<
    sdbusplus::xyz::openbmc_project::State::server::Host>;

namespace server = sdbusplus::xyz::openbmc_project::State::server;

/** @class Host
 *  @brief OpenBMC host state management implementation.
 *  @details A concrete implementation for xyz.openbmc_project.State.Host
 *  DBus API.
 */
class Hypervisor : public HypervisorInherit
{
  public:
    /** @brief Constructs Hypervisor State Manager
     *
     * @param[in] bus       - The Dbus bus object
     * @param[in] objPath   - The Dbus object path
     */
    Hypervisor(sdbusplus::bus::bus& bus, const char* objPath) :
        HypervisorInherit(bus, objPath, false), bus(bus)
    {}

    /** @brief Set value of HostTransition */
    server::Host::Transition
        requestedHostTransition(server::Host::Transition value) override;

    /** @brief Set value of CurrentHostState */
    server::Host::HostState
        currentHostState(server::Host::HostState value) override;

  private:
    /** @brief Persistent sdbusplus DBus bus connection. */
    sdbusplus::bus::bus& bus;
};

} // namespace manager
} // namespace state
} // namespace phosphor
