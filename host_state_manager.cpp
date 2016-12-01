#include <iostream>
#include <systemd/sd-bus.h>
#include "host_state_manager.hpp"

namespace phosphor
{
namespace state
{
namespace manager
{

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
        sdbusplus::xyz::openbmc_project::State::server::Host::
            currentHostState(HostState::Running);
    }
    else
    {
        std::cout << "HOST is not BOOTED " << sysState << std::endl;
        sdbusplus::xyz::openbmc_project::State::server::Host::
            currentHostState(HostState::Off);
    }

    // Set transition initially to Off
    // TODO - Eventually need to restore this from persistent storage
    sdbusplus::xyz::openbmc_project::State::server::Host::
                requestedHostTransition(Transition::Off);

    return;
}

Host::Transition Host::requestedHostTransition(Transition value)
{
    std::cout << "Someone is setting the RequestedHostTransition field" <<
            std::endl;
    return sdbusplus::xyz::openbmc_project::State::server::Host::
            requestedHostTransition(value);
}


Host::HostState Host::currentHostState(HostState value)
{
    std::cout << "Someone is being bad and trying to set the HostState field" <<
            std::endl;

    return sdbusplus::xyz::openbmc_project::State::server::Host::
            currentHostState();
}

} // namespace manager
} // namespace state
} // namepsace phosphor
