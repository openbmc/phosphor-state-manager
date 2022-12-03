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

using HypervisorInherit = sdbusplus::server::object_t<
    sdbusplus::xyz::openbmc_project::State::server::Host>;

namespace server = sdbusplus::xyz::openbmc_project::State::server;
namespace sdbusRule = sdbusplus::bus::match::rules;

/** @class Host
 *  @brief OpenBMC host state management implementation.
 *  @details A concrete implementation for xyz.openbmc_project.State.Host
 *  DBus API.
 */
class Hypervisor : public HypervisorInherit
{
  public:
    Hypervisor() = delete;
    Hypervisor(const Hypervisor&) = delete;
    Hypervisor& operator=(const Hypervisor&) = delete;
    Hypervisor(Hypervisor&&) = delete;
    Hypervisor& operator=(Hypervisor&&) = delete;
    virtual ~Hypervisor() = default;

    /** @brief Constructs Hypervisor State Manager
     *
     * @param[in] bus       - The Dbus bus object
     * @param[in] objPath   - The Dbus object path
     */
    Hypervisor(sdbusplus::bus_t& bus, const char* objPath) :
        HypervisorInherit(bus, objPath,
                          HypervisorInherit::action::emit_object_added),
        bus(bus),
        bootProgressChangeSignal(
            bus,
            sdbusRule::propertiesChanged(
                "/xyz/openbmc_project/state/host0",
                "xyz.openbmc_project.State.Boot.Progress"),
            [this](sdbusplus::message_t& m) { bootProgressChangeEvent(m); })
    {}

    /** @brief Set value of HostTransition */
    server::Host::Transition
        requestedHostTransition(server::Host::Transition value) override;

    /** @brief Set value of CurrentHostState */
    server::Host::HostState
        currentHostState(server::Host::HostState value) override;

    /** @brief Return value of CurrentHostState */
    server::Host::HostState currentHostState();

    /** @brief Check if BootProgress change affects hypervisor state
     *
     * @param[in]  bootProgress     - BootProgress value to check
     *
     */
    void updateCurrentHostState(std::string& bootProgress);

  private:
    /** @brief Process BootProgress property changes
     *
     * Instance specific interface to monitor for changes to the BootProgress
     * property which may impact Hypervisor state.
     *
     * @param[in]  msg              - Data associated with subscribed signal
     *
     */
    void bootProgressChangeEvent(sdbusplus::message_t& msg);

    /** @brief Persistent sdbusplus DBus bus connection. */
    sdbusplus::bus_t& bus;

    /** @brief Watch BootProgress changes to know hypervisor state **/
    sdbusplus::bus::match_t bootProgressChangeSignal;
};

} // namespace manager
} // namespace state
} // namespace phosphor
