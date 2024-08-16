#pragma once

#include "utils.hpp"
#include "xyz/openbmc_project/State/BMC/server.hpp"

#include <linux/watchdog.h>
#include <sys/sysinfo.h>

#include <sdbusplus/bus.hpp>

#include <cassert>
#include <chrono>

namespace phosphor
{
namespace state
{
namespace manager
{

using BMCInherit = sdbusplus::server::object_t<
    sdbusplus::server::xyz::openbmc_project::state::BMC>;
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
    BMC(sdbusplus::bus_t& bus, const char* objPath) :
        BMCInherit(bus, objPath, BMCInherit::action::defer_emit), bus(bus),
        stateSignal(std::make_unique<decltype(stateSignal)::element_type>(
            bus,
            sdbusRule::type::signal() + sdbusRule::member("JobRemoved") +
                sdbusRule::path("/org/freedesktop/systemd1") +
                sdbusRule::interface("org.freedesktop.systemd1.Manager"),
            [this](sdbusplus::message_t& m) { bmcStateChange(m); })),

        timeSyncSignal(std::make_unique<decltype(timeSyncSignal)::element_type>(
            bus,
            sdbusRule::propertiesChanged(
                "/org/freedesktop/systemd1/unit/time_2dsync_2etarget",
                "org.freedesktop.systemd1.Unit"),
            [this](sdbusplus::message_t& m) {
                std::string interface;
                std::unordered_map<std::string, std::variant<std::string>>
                    propertyChanged;
                m.read(interface, propertyChanged);

                for (const auto& [key, value] : propertyChanged)
                {
                    if (key == "ActiveState" &&
                        std::holds_alternative<std::string>(value) &&
                        std::get<std::string>(value) == "active")
                    {
                        updateLastRebootTime();
                        timeSyncSignal.reset();
                    }
                }
            }))
    {
        utils::subscribeToSystemdSignals(bus);
        discoverInitialState();
        discoverLastRebootCause();
        updateLastRebootTime();

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

    /** @brief Set value of LastRebootCause **/
    RebootCause lastRebootCause(RebootCause value) override;

  private:
    /**
     * @brief Retrieve input systemd unit state
     **/
    std::string getUnitState(const std::string& unitToCheck);
    /**
     * @brief discover the state of the bmc
     **/
    void discoverInitialState();

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
    int bmcStateChange(sdbusplus::message_t& msg);

    /** @brief Persistent sdbusplus DBus bus connection. **/
    sdbusplus::bus_t& bus;

    /** @brief Used to subscribe to dbus system state changes **/
    std::unique_ptr<sdbusplus::bus::match_t> stateSignal;

    /** @brief Used to subscribe to timesync **/
    std::unique_ptr<sdbusplus::bus::match_t> timeSyncSignal;

    /**
     * @brief discover the last reboot cause of the bmc
     **/
    void discoverLastRebootCause();

    /**
     * @brief update the last reboot time of the bmc
     **/
    void updateLastRebootTime();

    /**
     * @brief the lastRebootTime calculated at startup.
     **/
    uint64_t rebootTime;
};

} // namespace manager
} // namespace state
} // namespace phosphor
