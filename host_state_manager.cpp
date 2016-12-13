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
namespace server = sdbusplus::xyz::openbmc_project::State::server;

using namespace phosphor::logging;

/* Map a transition to it's systemd target */
const std::map<server::Host::Transition,std::string> SYSTEMD_TARGET_TABLE =
{
        {server::Host::Transition::Off, "obmc-chassis-stop@0.target"},
        {server::Host::Transition::On, "obmc-chassis-start@0.target"}
};

constexpr auto SYSTEMD_SERVICE   = "org.freedesktop.systemd1";
constexpr auto SYSTEMD_OBJ_PATH  = "/org/freedesktop/systemd1";
constexpr auto SYSTEMD_INTERFACE = "org.freedesktop.systemd1.Manager";

constexpr auto SYSTEM_SERVICE   = "org.openbmc.managers.System";
constexpr auto SYSTEM_OBJ_PATH  = "/org/openbmc/managers/System";
constexpr auto SYSTEM_INTERFACE = SYSTEM_SERVICE;

/* Map a system state to the HostState */
/* TODO:Issue 774 - Use systemd target signals to control host states */
const std::map<std::string, server::Host::HostState> SYS_HOST_STATE_TABLE = {
        {"HOST_BOOTING", server::Host::HostState::Running},
        {"HOST_POWERED_OFF", server::Host::HostState::Off}
};

// TODO - Will be rewritten once sdbusplus client bindings are in place
//        and persistent storage design is in place
void Host::determineInitialState()
{
    std::string sysState;

    auto method = this->bus.new_method_call(SYSTEM_SERVICE,
                                            SYSTEM_OBJ_PATH,
                                            SYSTEM_INTERFACE,
                                            "getSystemState");

    auto reply = this->bus.call(method);

    reply.read(sysState);

    if(sysState == "HOST_BOOTED")
    {
        log<level::INFO>("Initial Host State will be Running",
                         entry("CURRENT_HOST_STATE=0x%.2X",
                               HostState::Running));
        currentHostState(HostState::Running);
        requestedHostTransition(Transition::On);
    }
    else
    {
        log<level::INFO>("Initial Host State will be Off",
                         entry("CURRENT_HOST_STATE=0x%.2X", HostState::Off));
        currentHostState(HostState::Off);
        requestedHostTransition(Transition::Off);
    }

    // Set transition initially to Off
    // TODO - Eventually need to restore this from persistent storage
    server::Host::requestedHostTransition(Transition::Off);

    return;
}

void Host::executeTransition(Transition tranReq)
{
    auto sysdUnit = SYSTEMD_TARGET_TABLE.find(tranReq)->second;

    auto method = this->bus.new_method_call(SYSTEMD_SERVICE,
                                            SYSTEMD_OBJ_PATH,
                                            SYSTEMD_INTERFACE,
                                            "StartUnit");

    method.append(sysdUnit);
    method.append("replace");

    this->bus.call(method);

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
        hostInst->currentHostState(gotoState);

        // Check if we need to start a new transition (i.e. a Reboot)
        if((gotoState == server::Host::HostState::Off) &&
           (hostInst->server::Host::requestedHostTransition() ==
               Transition::Reboot))
        {
            log<level::DEBUG>("Reached intermediate state, going to next");
            hostInst->executeTransition(server::Host::Transition::On);
        }
        else
        {
            log<level::DEBUG>("Reached final state");
        }
    }

    return 0;
}

Host::Transition Host::requestedHostTransition(Transition value)
{
    log<level::INFO>(
            "Host State transaction request",
            entry("REQUESTED_HOST_TRANSITION=0x%.2X",value));

    Transition tranReq = value;
    if(value == server::Host::Transition::Reboot)
    {
        // On reboot requests we just need to do a off if we're on and
        // vice versa.  The handleSysStateChange() code above handles the
        // second part of the reboot
        if(this->server::Host::currentHostState() ==
            server::Host::HostState::Off)
        {
            tranReq = server::Host::Transition::On;
        }
        else
        {
            tranReq = server::Host::Transition::Off;
        }
    }

    executeTransition(tranReq);
    return server::Host::requestedHostTransition(value);
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
