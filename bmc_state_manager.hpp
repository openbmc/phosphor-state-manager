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
         * @param[in] bus       - The Dbus bus object
         * @param[in] busName   - The Dbus name to own
         * @param[in] objPath   - The Dbus object path
         */
        BMC(sdbusplus::bus::bus& bus,
            const char* objPath) :
                sdbusplus::server::object::object<
                    sdbusplus::xyz::openbmc_project::State::server::BMC>(
                        bus, objPath),
                            bus(bus),
                            stateSignal(
                                std::make_unique<
                                decltype(stateSignal)::element_type>(
                                bus,
                                "type='signal',"
                                "member='JobRemoved',"
                                "path='/org/freedesktop/systemd1',"
                                "interface='org.freedesktop.systemd1.Manager'",
                                bmcStateChangeSignal,
                                this))
        {
            subscribeToSystemdSignals();
            discoverInitialState();
        };

        /** @brief Set value of BMCTransition **/
        Transition requestedBMCTransition(Transition value) override;

        /** @brief Set value of CurrentBMCState **/
        BMCState currentBMCState(BMCState value) override;

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

        /** @brief Callback used to direct you into the class
         *
         * @param[in]  msg       - Data associated with subscribed signal
         * @param[in]  userData  - Pointer to this object instance
         * @param[out] retError  - return error data if any
         *
         */
        static int bmcStateChangeSignal(sd_bus_message* msg,
                                        void* userData,
                                        sd_bus_error* retError);

        /** @brief Callback function on bmc state change
         *
         * Check if the state is relevant to the BMC and if so, update
         * corresponding BMC object's state
         *
         * @param[in]  msg       - Data associated with subscribed signal
         * @param[out] retError  - return error data if any
         *
         */
        int bmcStateChange(sd_bus_message* msg,
                           sd_bus_error* retError);

        /** @brief Persistent sdbusplus DBus bus connection. **/
        sdbusplus::bus::bus& bus;

        /** @brief Used to subscribe to dbus system state changes **/
        std::unique_ptr<sdbusplus::server::match::match> stateSignal;
};

} // namespace manager
} // namespace state
} // namespace phosphor
