#pragma once

#include <functional>
#include <sdbusplus/bus.hpp>
#include "xyz/openbmc_project/State/Chassis/server.hpp"
#include <xyz/openbmc_project/Control/PowerSupplyRedundancy/server.hpp>

namespace phosphor
{
namespace state
{
namespace manager
{

using ChassisInherit = sdbusplus::server::object::object<
        sdbusplus::xyz::openbmc_project::State::server::Chassis,
        sdbusplus::xyz::openbmc_project::Control::server::PowerSupplyRedundancy>;
namespace sdbusRule = sdbusplus::bus::match::rules;

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
        Chassis(sdbusplus::bus::bus& bus,
                const char* busName,
                const char* objPath) :
                ChassisInherit(bus, objPath, true),
                bus(bus),
                systemdSignals(bus,
                               sdbusRule::type::signal() +
                               sdbusRule::member("JobRemoved") +
                               sdbusRule::path("/org/freedesktop/systemd1") +
                               sdbusRule::interface(
                                    "org.freedesktop.systemd1.Manager"),
                               std::bind(std::mem_fn(&Chassis::sysStateChange),
                                         this, std::placeholders::_1))
        {
            subscribeToSystemdSignals();

            determineInitialState();

            // We deferred this until we could get our property correct
            this->emit_object_added();
        }

        /** @brief Set value of RequestedPowerTransition */
        Transition requestedPowerTransition(Transition value) override;

        /** @brief Set value of CurrentPowerState */
        PowerState currentPowerState(PowerState value) override;

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
};

} // namespace manager
} // namespace state
} // namespace phosphor
