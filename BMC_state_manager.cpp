#include <iostream>
#include <systemd/sd-bus.h>
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

/* Map a transition to it's systemd target */
const std::map<server::BMC::Transition,std::string> SYSTEMD_TABLE = {
        {server::BMC::Transition::Reboot,"reboot.target"}
};

/* Map a system state to the BMCState */
const std::map<std::string, server::BMC::BMCState> SYS_BMC_STATE_TABLE = {
        {"BMC_READY", server::BMC::BMCState::Ready},
};

void BMC::determineInitialState()
{
    std::string sysState;

    auto method = this->bus.new_method_call("org.openbmc.managers.System",
                                            "/org/openbmc/managers/System",
                                            "org.openbmc.managers.System",
                                            "getSystemState");

    auto reply = this->bus.call(method);

    reply.read(sysState);

    if(sysState == "HOST_BOOTED")
    {
        log<level::INFO>("Initial BMC State will be Ready",
                         entry("CURRENT_BMC_STATE=0x%.2X",
                               BMCState::Ready));
        currentBMCState(BMCState::Ready);
    }
    else
    {
        log<level::INFO>("Initial BMC State will be NotReady",
                         entry("CURRENT_BMC_STATE=0x%.2X",
                               BMCState::NotReady));
        currentBMCState(BMCState::NotReady);
    }

    //Set transition intially to None
    //TODO - Eventually need to restore this from persistent storage
    server::BMC::requestedBMCTransition(Transition::None);
    return;
}

void BMC::executeTransition(const Transition tranReq)
{
    tranActive = true;

    std::string sysdUnit = SYSTEMD_TABLE.find(tranReq)->second;

    auto method = this->bus.new_method_call("org.freedesktop.systemd1",
                                            "/org/freedesktop/systemd1",
                                            "org.freedesktop.systemd1.Manager",
                                            "StartUnit");

    method.append(sysdUnit);
    method.append("replace");

    this->bus.call(method);
    return;
}

int BMC::handleSysStateChange(sd_bus_message *msg, void *usrData,
                              sd_bus_error *retError)
{
    const char *newState = nullptr;
    auto sdPlusMsg = sdbusplus::message::message(msg);
    sdPlusMsg.read(newState);


    auto it = SYS_BMC_STATE_TABLE.find(newState);
    if(it != SYS_BMC_STATE_TABLE.end())
    {
        BMC::BMCState gotoState = it->second;
        auto BMCInst = static_cast<BMC*>(usrData);
        BMCInst->currentBMCState(gotoState);
        BMCInst->tranActive = false;
    }
    return 0;
}

BMC::Transition BMC::requestedBMCTransition(Transition value)
{
    log<level::INFO>(
            "Setting the RequestedBMCTransition field",
            entry("REQUESTED_BMC_TRANSITION=0x%.2X", value));
    executeTransition(value);
    return server::BMC::requestedBMCTransition(value);
}

BMC::BMCState BMC::currentBMCState(BMCState value)
{
    log<level::INFO>(
            "Change to BMC State",
            entry("CURRENT_BMC_STATE=0x%.2X", value));
    return server::BMC::currentBMCState(value);
}


} // namespace manager
} // namespace state
} // namepsace phosphor

