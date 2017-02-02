#include <iostream>
#include <string>
#include <log.hpp>
#include "bmc_state_manager.hpp"

namespace phosphor
{
namespace state
{
namespace manager
{

// When you see server:: you know we're referencing our base class
namespace server = sdbusplus::xyz::openbmc_project::State::server;

using namespace phosphor::logging;

constexpr auto obmcStandbyTarget = "obmc-standby.target";
constexpr auto signalDone = "done";
constexpr auto activeState = "active";

/* Map a transition to it's systemd target */
const std::map<server::BMC::Transition, const char*> SYSTEMD_TABLE =
{
        {server::BMC::Transition::Reboot, "reboot.target"}
};

constexpr auto SYSTEMD_SERVICE   = "org.freedesktop.systemd1";
constexpr auto SYSTEMD_OBJ_PATH  = "/org/freedesktop/systemd1";
constexpr auto SYSTEMD_INTERFACE = "org.freedesktop.systemd1.Manager";

constexpr auto SYSTEMD_PRP_INTERFACE = "org.freedesktop.DBus.Properties";
constexpr auto SYSTEMD_TGT_PATH      = "/org/freedesktop/systemd1/unit/"
                                       "obmc_2dstandby_2etarget";

void BMC::discoverInitialState()
{
    sdbusplus::message::variant<std::string> currentState;

    auto method = this->bus.new_method_call(SYSTEMD_SERVICE,
                                            SYSTEMD_TGT_PATH,
                                            SYSTEMD_PRP_INTERFACE,
                                            "Get");

    method.append("org.freedesktop.systemd1.Unit", "ActiveState");

    auto result = this->bus.call(method);

    //Is obmc-standby.target active or inactive?
    result.read(currentState);

    if(currentState == activeState)
    {
        log<level::INFO>("Setting the BMCState field",
                         entry("CURRENT_BMC_STATE=%s",
                               "BMC_READY"));
        this->currentBMCState(BMCState::Ready);

        //Unsubscribe so we stop processing all other signals
        method = this->bus.new_method_call(SYSTEMD_SERVICE,
                                           SYSTEMD_OBJ_PATH,
                                           SYSTEMD_INTERFACE,
                                           "Unsubscribe");
        this->bus.call(method);
        this->stateSignal.release();
    }
    else
    {
        log<level::INFO>("Setting the BMCState field",
                         entry("CURRENT_BMC_STATE=%s",
                               "BMC_NOTREADY"));
        this->currentBMCState(BMCState::NotReady);
    }

    return;
}

void BMC::subscribeToSystemdSignals()
{
    auto method = this->bus.new_method_call(SYSTEMD_SERVICE,
                                            SYSTEMD_OBJ_PATH,
                                            SYSTEMD_INTERFACE,
                                            "Subscribe");
    this->bus.call(method);

    return;
}

void BMC::executeTransition(const Transition tranReq)
{
    //Check to make sure it can be found
    auto iter = SYSTEMD_TABLE.find(tranReq);
    if (iter == SYSTEMD_TABLE.end()) return;

    const auto& sysdUnit = iter->second;

    auto method = this->bus.new_method_call(SYSTEMD_SERVICE,
                                            SYSTEMD_OBJ_PATH,
                                            SYSTEMD_INTERFACE,
                                            "StartUnit");

    method.append(sysdUnit, "replace");

    this->bus.call(method);

    return;
}

int BMC::bmcStateChangeSignal(sd_bus_message *msg, void *userData,
                              sd_bus_error *retError)
{
    return static_cast<BMC*>(userData)->bmcStateChange(msg, retError);
}

int BMC::bmcStateChange(sd_bus_message *msg,
                        sd_bus_error *retError)
{
    uint32_t newStateID {};
    sdbusplus::message::object_path newStateObjPath;
    std::string newStateUnit{};
    std::string newStateResult{};

    auto sdPlusMsg = sdbusplus::message::message(msg);
    //Read the msg and populate each variable
    sdPlusMsg.read(newStateID, newStateObjPath, newStateUnit, newStateResult);

    //Caught the signal that indicates the BMC is now BMC_READY
    if((newStateUnit == obmcStandbyTarget) &&
       (newStateResult == signalDone))
    {
        log<level::INFO>("BMC_READY");
        this->currentBMCState(BMCState::Ready);

        //Unsubscribe so we stop processing all other signals
        auto method = this->bus.new_method_call(SYSTEMD_SERVICE,
                                                SYSTEMD_OBJ_PATH,
                                                SYSTEMD_INTERFACE,
                                                "Unsubscribe");
        this->bus.call(method);
        this->stateSignal.release();
    }

    return 0;
}

BMC::Transition BMC::requestedBMCTransition(Transition value)
{
    log<level::INFO>(
            "Setting the RequestedBMCTransition field",
            entry("REQUESTED_BMC_TRANSITION=0x%s",
                  convertForMessage(value).c_str()));

    executeTransition(value);
    return server::BMC::requestedBMCTransition(value);
}

BMC::BMCState BMC::currentBMCState(BMCState value)
{
    log<level::INFO>(
            "Setting the BMCState field",
            entry("CURRENT_BMC_STATE=0x%s",
                  convertForMessage(value).c_str()));

    return server::BMC::currentBMCState(value);
}


} // namespace manager
} // namespace state
} // namepsace phosphor

