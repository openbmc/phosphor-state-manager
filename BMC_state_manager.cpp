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
        {BMC::Transition::Reboot,"reboot.target"}
};

/* Map a system state to the BMCState */
const std::map<std::string,server::BMC::BMCState> SYS_BMC_STATE_TABLE = {
        {"BMC_READY",BMC::BMCState::Ready},
        {"BMC_NOTREADY",BMC::BMCState::NotReady}
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

    if(sysState == "BMC_READY")
    {
        std::cout << "BMC is BOOTED " << sysState << std::endl;
        server::BMC::currentBMCState(BMCState::Ready);
    }
    else
    {
        std::cout << "BMC is not BOOTED " << sysState << std::endl;
        server::BMC::currentBMCState(BMCState::NotReady);
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

int BMC::handleSysStateChange(sd_bus_message *msg, void *usrData,
                              sd_bus_error *retError)
{
    const char *newState = nullptr;
    int rc = sd_bus_message_read(msg, "s", &newState);
    if (rc < 0)
    {
        printf("Failed to parse handleSysStateChange signal data");
        return -1;
    }

    auto it = SYS_BMC_STATE_TABLE.find(newState);
    if(it != SYS_BMC_STATE_TABLE.end())
    {
        BMCState gotoState = it->second;
        auto BMCInst = static_cast<BMC*>(usrData);
        BMCInst->server::BMC::currentBMCState(gotoState);
        BMCInst->tranActive = false;
    }
    return 0;
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

