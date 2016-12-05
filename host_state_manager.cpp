#include <iostream>
#include <systemd/sd-bus.h>
#include "host_state_manager.hpp"

namespace phosphor
{
namespace state
{
namespace manager
{

// When you see server:: you know we're referencing our base class
using namespace sdbusplus::xyz::openbmc_project::State;

// TODO - Will be rewritten once sdbusplus client bindings are in place
//        and persistent storage design is in place
void Host::determineInitialState()
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
        std::cout << "HOST is BOOTED " << sysState << std::endl;
        server::Host::currentHostState(HostState::Running);
    }
    else
    {
        std::cout << "HOST is not BOOTED " << sysState << std::endl;
        server::Host::currentHostState(HostState::Off);
    }

    // Set transition initially to Off
    // TODO - Eventually need to restore this from persistent storage
    server::Host::requestedHostTransition(Transition::Off);

    return;
}

bool Host::verifyValidTransition(const Transition &tranReq,
                                 const HostState &curState,
                                 bool tranActive)
{
    bool valid = {false};

    // Make sure we're not in process of a transition
    if (tranActive)
    {
        std::cerr << "Busy, currently executing transition" << std::endl;
        return valid;
    }

    /* Valid Transitions
     *
     * CurrentHostState    RequestedHostTransition
     * Off              -> On
     * Running          -> Off
     * Running          -> Reboot
     */

    switch (tranReq)
    {
        case Transition::Off:
        {
            if(curState == HostState::Off)
            {
                std::cout << "Already at requested Off state" << std::endl;
            }
            else if(curState == HostState::Running)
            {
                valid = true;
            }
            break;
        }
        case Transition::On:
        {
            if(curState == HostState::Running)
            {
                std::cout << "Already at requested On state" << std::endl;
            }
            else if(curState == HostState::Off)
            {
                valid = true;
            }
            break;
        }
        case Transition::Reboot:
        {
            if(curState == HostState::Off)
            {
                std::cout << "Can not request reboot in off state" << std::endl;
            }
            else if(curState == HostState::Running)
            {
                valid = true;
            }
            break;
        }
    }

    return valid;
}

Host::Transition Host::requestedHostTransition(Transition value)
{
    std::cout << "Someone is setting the RequestedHostTransition field"
            << std::endl;
    return server::Host::requestedHostTransition(value);
}

Host::HostState Host::currentHostState(HostState value)
{
    std::cout << "Someone is being bad and trying to set the HostState field"
            << std::endl;

    return server::Host::currentHostState();
}

} // namespace manager
} // namespace state
} // namepsace phosphor
