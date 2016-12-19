#include <iostream>
#include <systemd/sd-bus.h>
#include "BMC_state_manager.hpp"

namespace phosphor
{
namespace state
{
namespace manager
{
using namespace sdbusplus::xyz::openbmc_project::State::server;


void BMC::determineInitialState()
{
    std::string sysState;

    auto method = this->bus.new_method_call("org.openbmc.managers.System",
                                            "/org/openbmc/managers/System",
                                            "org.openbmc.managers.System",
                                            "getSystemState");

    auto reply = this->bus.call(method);

    reply.read(sysState);

    if(sysState == "BMC_READY")
    {
        std::cout << "BMC is BOOTED " << sysState << std::endl;
        currentBMCState(BMCState::Ready);
    }
    else
    {
        std::cout << "BMC is not BOOTED " << sysState << std::endl;
        currentBMCState(BMCState::NotReady);
    }

    return;
}

BMC::Transition BMC::requestedBMCTransition(Transition value)
{
    std::cout << "Setting the RequestedBMCTransition field" << std::endl;
    return sdbusplus::xyz::openbmc_project::State::server::BMC::
            requestedBMCTransition(value);
}

BMC::BMCState BMC::currentBMCState(BMCState value)
{
    std::cout << "Setting the BMCState field" << std::endl;
    return sdbusplus::xyz::openbmc_project::State::server::BMC::
            currentBMCState(value);
}


} // namespace manager
} // namespace state
} // namepsace phosphor

