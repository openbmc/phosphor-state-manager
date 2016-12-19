#pragma once

#include <sdbusplus/bus.hpp>
#include "xyz/openbmc_project/State/BMC/server.hpp"

namespace phosphor
{
namespace state
{
namespace manager
{

/** @class BMC
 *  @brief OpenBMC BMC state management implementation.
 *  @details A concrete implementation for xyz.openbmc_project.State.BMC
 *  DBus API.
 */
class BMC : public sdbusplus::server::object::object<
                sdbusplus::xyz::openbmc_project::State::server::BMC>
{
    public:
        /** @brief Constructs BMC State Manager
         *
         *  @note This constructor passes 'true' to the base class in order to
         *  defer dbus object registration until we can run
         *  subscribeToSystemdSignals() and set our properties
         *
         * @param[in] bus       - The Dbus bus object
         * @param[in] busName   - The Dbus name to own
         * @param[in] objPath   - The Dbus object path
         */
        BMC(sdbusplus::bus::bus& bus,
             const char* objPath) :
             sdbusplus::server::object::object<
                 sdbusplus::xyz::openbmc_project::State::server::BMC>(
                         bus, objPath, true),
             bus(bus),
             stateSignal(bus,
                         "type='signal',"
                         "member='JobRemoved',"
                         "path='/org/freedesktop/systemd1',"
                         "interface='org.freedesktop.systemd1.Manager'",
                         BmcStateChange,
                         this)
        {
            subscribeToSystemdSignals();

            emit_object_added();
        };

        /**
         * @brief subscribe to the systemd signals
         **/
        void subscribeToSystemdSignals();

        /** @brief Set value of BMCTransition **/
        Transition requestedBMCTransition(Transition value) override;

        /** @brief Set value of CurrentBMCState **/
        BMCState currentBMCState(BMCState value) override;

    private:

        /** @brief Execute the transition request
         *
         *  @param[in] tranReq   - Transition requested
         */
        void executeTransition(Transition tranReq);

        /** @brief Callback function on system state changes
         *
         * Check if the state is relevant to the BMC and if so, update
         * corresponding BMC object's state
         *
         * @param[in]  msg       - Data associated with subscribed signal
         * @param[in]  userData  - Pointer to this object instance
         * @param[out] retError  - return error data
         *
         */
         static int BmcStateChange(sd_bus_message* msg,
                                   void* userData,
                                   sd_bus_error* retError);

        /** @brief Persistent sdbusplus DBus bus connection. **/
        sdbusplus::bus::bus& bus;

        /** @brief Used to subscribe to dbus system state changes **/
        sdbusplus::server::match::match stateSignal;
};

} // namespace manager
} // namespace state
} // namespace phosphor
