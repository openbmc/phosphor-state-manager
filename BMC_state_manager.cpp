#include <iostream>
#include <string>
#include <log.hpp>
#include "BMC_state_manager.hpp"

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

/* Map a transition to it's systemd target */
const std::map<server::BMC::Transition, std::string> SYSTEMD_TABLE =
{
        {server::BMC::Transition::Reboot, "reboot.target"}
};

constexpr auto SYSTEMD_SERVICE   = "org.freedesktop.systemd1";
constexpr auto SYSTEMD_OBJ_PATH  = "/org/freedesktop/systemd1";
constexpr auto SYSTEMD_INTERFACE = "org.freedesktop.systemd1.Manager";

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
    auto sysdUnit = SYSTEMD_TABLE.find(tranReq)->second;

    auto method = this->bus.new_method_call(SYSTEMD_SERVICE,
                                            SYSTEMD_OBJ_PATH,
                                            SYSTEMD_INTERFACE,
                                            "StartUnit");

    method.append(sysdUnit);
    method.append("replace");

    this->bus.call(method);

    return;
}

int BMC::handleSysStateChange(sd_bus_message *msg, void *userData,
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
        auto BMCInst = static_cast<BMC*>(userData);
        BMCInst->currentBMCState(BMCState::Ready);

        //Unsubscribe so we stop processing all other signals
        auto method = BMCInst->bus.new_method_call(SYSTEMD_SERVICE,
                                                   SYSTEMD_OBJ_PATH,
                                                   SYSTEMD_INTERFACE,
                                                   "Unsubscribe");
        BMCInst->bus.call(method);
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

