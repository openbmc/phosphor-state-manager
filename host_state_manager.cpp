#include <iostream>
#include <map>
#include <string>
#include <systemd/sd-bus.h>
#include <log.hpp>
#include "host_state_manager.hpp"

namespace phosphor
{
namespace state
{
namespace manager
{

// When you see server:: you know we're referencing our base class
using namespace sdbusplus::xyz::openbmc_project::State;

using namespace phosphor::logging;

/* Map a transition to it's systemd target */
const std::map<Host::Transition, std::string> SYSTEMD_TABLE = {
        {Host::Transition::Off,"obmc-chassis-stop@0.target"},
        {Host::Transition::On,"obmc-chassis-start@0.target"}
};


const constexpr char* SYSTEMD_SERVICE   = "org.openbmc.managers.System";
const constexpr char* SYSTEMD_OBJ_PATH  = "/org/openbmc/managers/System";
const constexpr char* SYSTEMD_INTERFACE = SYSTEMD_SERVICE;

/* Map a system state to the HostState */
const std::map<std::string, Host::HostState> SYS_HOST_STATE_TABLE = {
        {"HOST_BOOTING",Host::HostState::Running},
        {"HOST_POWERED_OFF",Host::HostState::Off}
};

// TODO - Will be rewritten once sdbusplus client bindings are in place
//        and persistent storage design is in place
void Host::determineInitialState()
{
    std::string sysState;

    auto method = this->bus.new_method_call(SYSTEMD_SERVICE,
                                            SYSTEMD_OBJ_PATH,
                                            SYSTEMD_INTERFACE,
                                            "getSystemState");

    auto reply = this->bus.call(method);

    reply.read(sysState);

    if(sysState == "HOST_BOOTED")
    {
        log<level::INFO>("Initial Host State will be Running",
                         entry("CURRENT_HOST_STATE=0x%.2X",HostState::Running));
        server::Host::currentHostState(HostState::Running);
        server::Host::requestedHostTransition(Transition::On);
    }
    else
    {
        log<level::INFO>("Initial Host State will be Off",
                         entry("CURRENT_HOST_STATE=0x%.2X",HostState::Off));
        server::Host::currentHostState(HostState::Off);
        server::Host::requestedHostTransition(Transition::Off);
    }

    // Set transition initially to Off
    // TODO - Eventually need to restore this from persistent storage
    server::Host::requestedHostTransition(Transition::Off);

    return;
}

bool Host::verifyValidTransition(const Transition tranReq,
                                 const HostState curState,
                                 bool tranActive,
                                 Transition& tranTarget)
{
    bool valid = {false};

    // If we already have a transition active, it can only be valid if
    // we're doing a reboot (which is a power off, then on transition)
    if ((tranActive) && (tranReq != Transition::Reboot))
    {
        return valid;
    }

    /* Valid Transitions
     *
     * tranActive    CurrentHostState    RequestedHostTransition
     * false         Off              -> On
     * false         Running          -> Off
     * false         Running          -> Reboot (Off)
     * true          Off              -> Reboot (On)
     */

    switch (tranReq)
    {
        case Transition::Off:
        {
            if(curState == HostState::Running)
            {
                valid = true;
                tranTarget = tranReq;
            }
            break;
        }
        case Transition::On:
        {
            if(curState == HostState::Off)
            {
                valid = true;
                tranTarget = tranReq;
            }
            break;
        }
        case Transition::Reboot:
        {
            if((curState == HostState::Off) && (tranActive))
            {
                valid = true;
                tranTarget = Transition::On;
            }
            else if((curState == HostState::Running) && (!tranActive))
            {
                valid = true;
                tranTarget = Transition::Off;
            }
            break;
        }
    }

    return valid;
}

void Host::executeTransition(const Transition tranReq)
{
    tranActive = true;

    std::string sysdUnit = SYSTEMD_TABLE.find(tranReq)->second;

    auto method = this->bus.new_method_call(SYSTEMD_SERVICE,
                                            SYSTEMD_OBJ_PATH,
                                            SYSTEMD_INTERFACE,
                                            "StartUnit");

    method.append(sysdUnit);
    method.append("replace");

    auto reply = this->bus.call(method);

    return;
}

int Host::handleSysStateChange(sd_bus_message *msg, void *usrData,
                               sd_bus_error *retError)
{
    const char *newState = nullptr;
    auto sdPlusMsg = sdbusplus::message::message(msg);
    sdPlusMsg.read(newState);

    auto it = SYS_HOST_STATE_TABLE.find(newState);
    if(it != SYS_HOST_STATE_TABLE.end())
    {
        // This is a state change we're interested in so process it
        Host::HostState gotoState = it->second;
        // Grab the host object instance from the userData
        auto hostInst = static_cast<Host*>(usrData);
        hostInst->server::Host::currentHostState(gotoState);

        // Check if we need to start a new transition (i.e. a Reboot)
        Host::Transition newTran;
        if (verifyValidTransition(hostInst->server::Host::
                                      requestedHostTransition(),
                                  hostInst->server::Host::currentHostState(),
                                  hostInst->tranActive,
                                  newTran))
        {
            log<level::DEBUG>("Reached intermediate state, going to next");
            hostInst->executeTransition(newTran);
        }
        else
        {
            log<level::DEBUG>("Reached final state");
            hostInst->tranActive = false;
        }
    }

    return 0;
}

Host::Transition Host::requestedHostTransition(Transition value)
{
    Transition tranReq;
    if(verifyValidTransition(value,
                             server::Host::currentHostState(),
                             tranActive,
                             tranReq))
    {
        // Note that the requested transition (value) and the transition we
        // execute (tranReq) are not always the same.  An example is a Reboot
        // transition request.  In this case we transition to off, and then to
        // on.
        log<level::INFO>(
                "Valid Host State transaction request",
                entry("REQUESTED_HOST_TRANSITION=0x%.2X",value));

        executeTransition(tranReq);

        return server::Host::requestedHostTransition(value);
    }

    log<level::NOTICE>("Not a valid transaction request",
                       entry("INVALID_REQUESTED_HOST_TRANSITION=0x%.2X",value));

    return server::Host::requestedHostTransition();
}

Host::HostState Host::currentHostState(HostState value)
{
    log<level::INFO>("Change to Host State",
                     entry("CURRENT_HOST_STATE=0x%.2X",value));
    return server::Host::currentHostState(value);
}

} // namespace manager
} // namespace state
} // namepsace phosphor
