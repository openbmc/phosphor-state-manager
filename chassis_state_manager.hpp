#pragma once

#include <functional>
#include <experimental/filesystem>
#include <cereal/cereal.hpp>
#include <sdbusplus/bus.hpp>
#include "xyz/openbmc_project/State/Chassis/server.hpp"
#include "xyz/openbmc_project/State/PowerOnHours/server.hpp"
#include "config.h"
#include "timer.hpp"

namespace phosphor
{
namespace state
{
namespace manager
{
namespace POH
{

using namespace std::chrono_literals;
constexpr auto secondsPerHour = 3600s; //seconds Per Hour

} // namespace  POH

using ChassisInherit = sdbusplus::server::object::object<
    sdbusplus::xyz::openbmc_project::State::server::Chassis,
    sdbusplus::xyz::openbmc_project::State::server::PowerOnHours>;
namespace sdbusRule = sdbusplus::bus::match::rules;
namespace fs = std::experimental::filesystem;

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
     * @param[in] instance  - The instance of this object
     * @param[in] objPath   - The Dbus object path
     */
    Chassis(sdbusplus::bus::bus& bus, const char* busName,
            const char* objPath) :
        ChassisInherit(bus, objPath, true),
        bus(bus),
        systemdSignals(
            bus,
            sdbusRule::type::signal() + sdbusRule::member("JobRemoved") +
                sdbusRule::path("/org/freedesktop/systemd1") +
                sdbusRule::interface("org.freedesktop.systemd1.Manager"),
            std::bind(std::mem_fn(&Chassis::sysStateChange), this,
                      std::placeholders::_1))
    {
        subscribeToSystemdSignals();

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
    using ChassisInherit::pOHCounter;

    void startPOHCounter();

  private:
    /** @brief Determine initial chassis state and set internally */
    void determineInitialState();

    /**
     * @brief subscribe to the systemd signals
     *
     * This object needs to capture when it's systemd targets complete
     * so it can keep it's state updated
     *
     **/
    void subscribeToSystemdSignals();

    /** @brief Execute the transition request
     *
     * This function calls the appropriate systemd target for the input
     * transition.
     *
     * @param[in] tranReq    - Transition requested
     */
    void executeTransition(Transition tranReq);

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
    int sysStateChange(sdbusplus::message::message& msg);

    /** @brief Persistent sdbusplus DBus connection. */
    sdbusplus::bus::bus& bus;

    /** @brief Used to subscribe to dbus systemd signals **/
    sdbusplus::bus::match_t systemdSignals;

    /** @brief Used to Set value of POHCounter */
    uint32_t pOHCounter(uint32_t value) override;

    /** @brief Used to restore POHCounter value form persisted file */
    void restorePOHCounter();

    /** @brief Function required by Cereal to perform serialization.
     *
     *  @tparam Archive - Cereal archive type (json in our case).
     *  @param[in] archive - reference to Cereal archive.
     *  @param[in] version - Class version that enables handling
     *                       a serialized data across code levels
     */
    template <class Archive>
    inline void save(Archive& archive, const std::uint32_t version) const
    {
        archive(pOHCounter());
    }

    /** @brief Function required by Cereal to perform deserialization.
     *
     *  @tparam Archive - Cereal archive type (json in our case).
     *  @param[in] archive - reference to Cereal archive.
     *  @param[in] version - Class version that enables handling
     *                       a serialized data across code levels
     */
    template <class Archive>
    inline void load(Archive& archive, const std::uint32_t version)
    {
        uint32_t value;
        archive(value);
        pOHCounter(value);
    }

    /** @brief Serialize and persist requested POH counter.
     *
     *  @param[in] dir - pathname of file where the serialized POH counter will
     *                   be placed.
     *
     *  @return fs::path - pathname of persisted requested POH counter.
     */
    fs::path
        serialize(const fs::path& dir = fs::path(POH_COUNTER_PERSIST_PATH));

    /** @brief Deserialze a persisted requested POH counter.
     *
     *  @param[in] path - pathname of persisted POH counter file
     *  @param[in] retCounter - deserialized POH counter value
     *
     *  @return bool - true if the deserialization was successful, false
     *                 otherwise.
     */
    bool deserialize(const fs::path& path, uint32_t& retCounter);

    /** @brief Timer */
    std::unique_ptr<phosphor::state::manager::Timer> timer;
};

} // namespace manager
} // namespace state
} // namespace phosphor
