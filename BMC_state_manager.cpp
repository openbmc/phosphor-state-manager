#include <iostream>
#include <systemd/sd-bus.h>
#include "BMC_state_manager.hpp"

namespace phosphor
{
namespace state
{
namespace manager
{
using namespace sdbusplus::xyz::openbmc_project::State;

/* Map a transition to it's systemd target */
const std::map<server::BMC::Transition,std::string> SYSTEMD_TABLE = {
        {server::BMC::Transition::Reboot,"reboot.target"}
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
        std::cout << "BMC is READY " << sysState << std::endl;
        currentBMCState(BMCState::Ready);
    }
    else
    {
        std::cout << "BMC is not READY " << sysState << std::endl;
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

    auto reply = this->bus.call(method);
    return;
}

BMC::Transition BMC::requestedBMCTransition(Transition value)
{
    std::cout << "Setting the RequestedBMCTransition field" << std::endl;
    executeTransition(value);
    std::cout << "......SUCCESS" << std::endl;
    return server::BMC::requestedBMCTransition(value);
}

BMC::BMCState BMC::currentBMCState(BMCState value)
{
    std::cout << "Setting the BMCState field" << std::endl;
    return server::BMC::currentBMCState(value);
}


} // namespace manager
} // namespace state
} // namepsace phosphor

