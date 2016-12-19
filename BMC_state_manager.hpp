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
        BMC() = delete;
        ~BMC() = default;
        BMC(const BMC&) = delete;
        BMC& operator=(const BMC&) = delete;
        BMC(BMC&&) = default;
        BMC& operator=(BMC&&) = delete;

        /** @brief Constructs BMC State Manager
         *
         * @param[in] bus       - The Dbus bus object
         * @param[in] busName   - The Dbus name to own
         * @param[in] objPath   - The Dbus object path
         */
        BMC(sdbusplus::bus::bus& bus,
             const char* busName,
             const char* objPath) :
             sdbusplus::server::object::object<
                 sdbusplus::xyz::openbmc_project::State::server::BMC>(
                         bus, objPath),
             bus(bus),
             path(objPath),
             stateSignal(bus,
                         "type='signal',member='GotoSystemState'",
                         handleSysStateChange,
                         this)
        {
            determineInitialState();
        };

        /**
         * @breif Determine intitial BMC state and set internally
         **/
        void determineInitialState();

        /** @brief Set value of BMCTransition **/
        Transition requestedBMCTransition(Transition value) override;

        /** @breif Set value of CurrentBMCState **/
        BMCState currentBMCState(BMCState value) override;

    private:

        /** @breif Execute the transition request
         * This function assumes the state has been validated and the BMC
         * is in an appropriate state for the transition to be started.
         * Currently for the BMC a request to reboot is valid at any time.
         *
         * @param[in] tranReq    - Transition requested
         */
        void executeTransition(const Transition tranReq);

        /** @breif Callback function on system state changes
         *
         * Check if the state is relevant to the BMC and if so, update
         * corresponding BMC object's state
         *
         * @param[in] msg       - Data associated with subscribed signal
         * @param[in] userData  - Pointer to this object instance
         * @param[in] retError  - return error data
         *
         */
         static int handleSysStateChange(sd_bus_message* msg,
                                         void* userData,
                                         sd_bus_error* retError);

        /** @brief Persistent sdbusplus DBus bus connection. **/
        sdbusplus::bus::bus& bus;

        /** @brief Path of the BMC instance */
        std::string path;

        /** @brief Used to subscribe to dbus system state changes **/
        sdbusplus::server::match::match stateSignal;

        /** @breif Indicates whether transition is actively executing **/
        bool tranActive;
};

} // namespace manager
} // namespace state
} // namespace phosphor
