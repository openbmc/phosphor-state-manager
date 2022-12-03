#pragma once

#include "config.h"

#include "xyz/openbmc_project/State/Chassis/server.hpp"
#include "xyz/openbmc_project/State/PowerOnHours/server.hpp"

#include <cereal/cereal.hpp>
#include <sdbusplus/bus.hpp>
#include <sdeventplus/clock.hpp>
#include <sdeventplus/event.hpp>
#include <sdeventplus/utility/timer.hpp>

#include <chrono>
#include <filesystem>

namespace phosphor
{
namespace state
{
namespace manager
{

using ChassisInherit = sdbusplus::server::object_t<
    sdbusplus::xyz::openbmc_project::State::server::Chassis,
    sdbusplus::xyz::openbmc_project::State::server::PowerOnHours>;
namespace sdbusRule = sdbusplus::bus::match::rules;
namespace fs = std::filesystem;

/** @class Chassis
 *  @brief OpenBMC chassis state management implementation.
 *  @details A concrete implementation for xyz.openbmc_project.State.Chassis
 *  DBus API.
 */
class Chassis : public ChassisInherit
{
  public:
    /** @brief Constructs Chassis State Manager
     *
     * @note This constructor passes 'true' to the base class in order to
     *       defer dbus object registration until we can run
     *       determineInitialState() and set our properties
     *
     * @param[in] bus       - The Dbus bus object
     * @param[in] objPath   - The Dbus object path
     * @param[in] id        - Chassis id
     */
    Chassis(sdbusplus::bus_t& bus, const char* objPath, size_t id) :
        ChassisInherit(bus, objPath, ChassisInherit::action::defer_emit),
        bus(bus),
        systemdSignals(
            bus,
            sdbusRule::type::signal() + sdbusRule::member("JobRemoved") +
                sdbusRule::path("/org/freedesktop/systemd1") +
                sdbusRule::interface("org.freedesktop.systemd1.Manager"),
            [this](sdbusplus::message_t& m) { sysStateChange(m); }),
        id(id),
        pohTimer(
            sdeventplus::Event::get_default(), [this](auto&) { pohCallback(); },
            std::chrono::hours{1}, std::chrono::minutes{1})
    {
        subscribeToSystemdSignals();

        createSystemdTargetTable();

        restoreChassisStateChangeTime();

        // No default in PDI so start at Good, skip D-Bus signal for now
        currentPowerStatus(PowerStatus::Good, true);
        determineInitialState();

        restorePOHCounter(); // restore POHCounter from persisted file

        // We deferred this until we could get our property correct
        this->emit_object_added();
    }

    /** @brief Set value of RequestedPowerTransition */
    Transition requestedPowerTransition(Transition value) override;

    /** @brief Set value of CurrentPowerState */
    PowerState currentPowerState(PowerState value) override;

    /** @brief Get value of POHCounter */
    using ChassisInherit::pohCounter;

    /** @brief Increment POHCounter if Chassis Power state is ON */
    void startPOHCounter();

  private:
    /** @brief Create systemd target instance names and mapping table */
    void createSystemdTargetTable();

    /** @brief Determine initial chassis state and set internally */
    void determineInitialState();

    /** @brief Determine status of power into system by examining all the
     *        power-related interfaces of interest
     */
    void determineStatusOfPower();

    /** @brief Determine status of power provided by an Uninterruptible Power
     *         Supply into the system
     *
     *  @return True if UPS power is good, false otherwise
     */
    bool determineStatusOfUPSPower();

    /** @brief Determine status of power provided by the power supply units into
     *         the system
     *
     *  @return True if PSU power is good, false otherwise
     */
    bool determineStatusOfPSUPower();

    /**
     * @brief subscribe to the systemd signals
     *
     * This object needs to capture when it's systemd targets complete
     * so it can keep it's state updated
     *
     **/
    void subscribeToSystemdSignals();

    /** @brief Start the systemd unit requested
     *
     * This function calls `StartUnit` on the systemd unit given.
     *
     * @param[in] sysdUnit    - Systemd unit
     */
    void startUnit(const std::string& sysdUnit);

    /** @brief Restart the systemd unit requested
     *
     * This function calls `RestartUnit` on the systemd unit given.
     * This is useful when needing to restart a service that is already running
     *
     * @param[in] sysdUnit    - Systemd unit to restart
     */
    void restartUnit(const std::string& sysdUnit);

    /**
     * @brief Determine if target is active
     *
     * This function determines if the target is active and
     * helps prevent misleading log recorded states.
     *
     * @param[in] target - Target string to check on
     *
     * @return boolean corresponding to state active
     **/
    bool stateActive(const std::string& target);

    /** @brief Check if systemd state change is relevant to this object
     *
     * Instance specific interface to handle the detected systemd state
     * change
     *
     * @param[in]  msg       - Data associated with subscribed signal
     *
     */
    int sysStateChange(sdbusplus::message_t& msg);

    /** @brief Persistent sdbusplus DBus connection. */
    sdbusplus::bus_t& bus;

    /** @brief Used to subscribe to dbus systemd signals **/
    sdbusplus::bus::match_t systemdSignals;

    /** @brief Watch for any changes to UPS properties **/
    std::unique_ptr<sdbusplus::bus::match_t> uPowerPropChangeSignal;

    /** @brief Watch for any changes to PowerSystemInputs properties **/
    std::unique_ptr<sdbusplus::bus::match_t> powerSysInputsPropChangeSignal;

    /** @brief Chassis id. **/
    const size_t id = 0;

    /** @brief Transition state to systemd target mapping table. **/
    std::map<Transition, std::string> systemdTargetTable;

    /** @brief Used to Set value of POHCounter */
    uint32_t pohCounter(uint32_t value) override;

    /** @brief Used by the timer to update the POHCounter */
    void pohCallback();

    /** @brief Used to restore POHCounter value from persisted file */
    void restorePOHCounter();

    /** @brief Serialize and persist requested POH counter.
     *
     *  @return fs::path - pathname of persisted requested POH counter.
     */
    fs::path serializePOH();

    /** @brief Deserialize a persisted requested POH counter.
     *
     *  @param[in] retCounter - deserialized POH counter value
     *
     *  @return bool - true if the deserialization was successful, false
     *                 otherwise.
     */
    bool deserializePOH(uint32_t& retCounter);

    /** @brief Sets the LastStateChangeTime property and persists it. */
    void setStateChangeTime();

    /** @brief Serialize the last power state change time.
     *
     *  Save the time the state changed and the state itself.
     *  The state needs to be saved as well so that during rediscovery
     *  on reboots there's a way to know not to update the time again.
     */
    void serializeStateChangeTime();

    /** @brief Deserialize the last power state change time.
     *
     *  @param[out] time - Deserialized time
     *  @param[out] state - Deserialized power state
     *
     *  @return bool - true if successful, false otherwise.
     */
    bool deserializeStateChangeTime(uint64_t& time, PowerState& state);

    /** @brief Restores the power state change time.
     *
     *  The time is loaded into the LastStateChangeTime D-Bus property.
     *  On the very first start after this code has been applied but
     *  before the state has changed, the LastStateChangeTime value
     *  will be zero.
     */
    void restoreChassisStateChangeTime();

    /** @brief Timer used for tracking power on hours */
    sdeventplus::utility::Timer<sdeventplus::ClockId::Monotonic> pohTimer;

    /** @brief Function to check for a standby voltage regulator fault
     *
     *  Determine if a standby voltage regulator fault was detected and
     *  return true or false accordingly.
     *
     *  @return true if fault detected, else false
     */
    bool standbyVoltageRegulatorFault();

    /** @brief Process UPS property changes
     *
     * Instance specific interface to monitor for changes to the UPS
     * properties which may impact CurrentPowerStatus
     *
     * @param[in]  msg              - Data associated with subscribed signal
     *
     */
    void uPowerChangeEvent(sdbusplus::message_t& msg);

    /** @brief Process PowerSystemInputs property changes
     *
     * Instance specific interface to monitor for changes to the
     * PowerSystemInputs properties which may impact CurrentPowerStatus
     *
     * @param[in]  msg              - Data associated with subscribed signal
     *
     */
    void powerSysInputsChangeEvent(sdbusplus::message_t& msg);
};

} // namespace manager
} // namespace state
} // namespace phosphor
