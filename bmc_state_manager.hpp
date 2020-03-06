#pragma once

#include <functional>
#include <sdbusplus/bus.hpp>
#include "xyz/openbmc_project/State/BMC/server.hpp"

namespace phosphor
{
namespace state
{
namespace manager
{

using BMCInherit = sdbusplus::server::object::object<
    sdbusplus::xyz::openbmc_project::State::server::BMC>;
namespace sdbusRule = sdbusplus::bus::match::rules;

/** @class BMC
 *  @brief OpenBMC BMC state management implementation.
 *  @details A concrete implementation for xyz.openbmc_project.State.BMC
 *  DBus API.
 */
class BMC : public BMCInherit
{
  public:
    /** @brief Constructs BMC State Manager
     *
     * @param[in] bus       - The Dbus bus object
     * @param[in] busName   - The Dbus name to own
     * @param[in] objPath   - The Dbus object path
     */
    BMC(sdbusplus::bus::bus& bus, const char* objPath) :
        BMCInherit(bus, objPath, true), bus(bus),
        stateSignalJobRemoved(
            std::make_unique<decltype(stateSignalJobRemoved)::element_type>(
                bus,
                sdbusRule::type::signal() + sdbusRule::member("JobRemoved") +
                    sdbusRule::path("/org/freedesktop/systemd1") +
                    sdbusRule::interface("org.freedesktop.systemd1.Manager"),
                std::bind(std::mem_fn(&BMC::bmcStateChangeJobRemoved), this,
                          std::placeholders::_1))),
        stateSignalJobNew(
            std::make_unique<decltype(stateSignalJobNew)::element_type>(
                bus,
                sdbusRule::type::signal() + sdbusRule::member("JobNew") +
                    sdbusRule::path("/org/freedesktop/systemd1") +
                    sdbusRule::interface("org.freedesktop.systemd1.Manager"),
                std::bind(std::mem_fn(&BMC::bmcStateChangeJobNew), this,
                          std::placeholders::_1)))
    {
        subscribeToSystemdSignals();
        discoverInitialState();
        this->emit_object_added();
    };

    /** @brief Set value of BMCTransition **/
    Transition requestedBMCTransition(Transition value) override;

    /** @brief Set value of CurrentBMCState **/
    BMCState currentBMCState(BMCState value) override;

    /** @brief Returns the last time the BMC was rebooted
     *
     *  @details Uses uptime information to determine when
     *           the BMC was last rebooted.
     *
     *  @return uint64_t - Epoch time, in milliseconds, of the
     *                     last reboot.
     */
    uint64_t lastRebootTime() const override;

  private:
    /**
     * @brief discover the state of the bmc
     **/
    void discoverInitialState();

    /**
     * @brief subscribe to the systemd signals
     **/
    void subscribeToSystemdSignals();

    /** @brief Execute the transition request
     *
     *  @param[in] tranReq   - Transition requested
     */
    void executeTransition(Transition tranReq);

    /** @brief Callback function on bmc state change
     *
     * Check if the state is relevant to the BMC and if so, update
     * corresponding BMC object's state
     *
     * @param[in]  msg       - Data associated with subscribed signal
     *
     */
    int bmcStateChangeJobRemoved(sdbusplus::message::message& msg);

    /** @brief Check if JobNew systemd signal is relevant to this object
     *
     * In certain instances phosphor-bmc-state-manager needs to monitor for the
     * entry into a systemd target. This function will be used for these cases.
     *
     * Instance specific interface to handle the detected systemd state
     * change
     *
     * @param[in]  msg       - Data associated with subscribed signal
     */
    int bmcStateChangeJobNew(sdbusplus::message::message& msg);

    /** @brief Persistent sdbusplus DBus bus connection. **/
    sdbusplus::bus::bus& bus;

    /** @brief Used to subscribe to dbus system state changes **/
    std::unique_ptr<sdbusplus::bus::match_t> stateSignalJobRemoved;

    /** @brief Used to subscribe to dbus systemd JobNew signals **/
    std::unique_ptr<sdbusplus::bus::match_t> stateSignalJobNew;
};

} // namespace manager
} // namespace state
} // namespace phosphor
