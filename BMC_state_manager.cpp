#include <iostream>
#include <systemd/sd-bus.h>
#include <map>
#include <log.hpp>
#include "BMC_state_manager.hpp"

namespace phosphor
{
namespace state
{
namespace manager
{

// When you see server:: you know we're referencing our base class
using namespace sdbusplus::xyz::openbmc_project::State;

/* Map a transition to it's systemd target */
const std::map<server::BMC::Transition,std::string> SYSTEMD_TABLE = {
        {server::BMC::Transition::Reboot,"obmc-standby.target"}
};

/* Map a system state to the BMCState */
const std::map<std::string,server::BMC::BMCState> SYS_BMC_STATE_TABLE = {
        {"BMC_READY",server::BMC::BMCState::Ready},
        {"BMC_NOTREADY",server::BMC::BMCState::NotReady}
};

BMC::BMC(
        sdbusplus::bus::bus& bus,
        const char* busName,
        const char* objPath) :
        sdbusplus::server::object::object<
            server::BMC>(
               bus, objPath),
         bus(bus),
         path(objPath),
         stateSignal(bus,
                     "type='signal',member='GotoSystemState'",
                     handleSysStateChange,
                     this),
         tranActive(false)
{
    determineInitialState();
}

// TODO - Will be rewritten once sdbusplus client bindings are in place
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

void BMC::executeTransition(const Transition& tranReq)
{
    tranActive = true;

    std::string sysdUnit = SYSTEMD_TABLE.find(tranReq)->second;

    auto method = this->bus.new_method_call("org.freedesktop.systemd1",
                                            "/org/freedesktop/systemd1",
                                            "org.freedesktop.systemd1.Manager",
                                            "Subscribe");

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
        BMC::BMCState gotoState = it->second;
        BMC* BMCInst = static_cast<BMC*>(usrData);
        BMCInst->currentBMCState(gotoState);

        // Check if we need to start a new transition (i.e. a Reboot)
        BMC::Transition newTran;
        BMCInst->executeTransition(newTran);
    }
    return 0;
}

BMC::Transition BMC::requestedBMCTransition(Transition value)
{
    std::cout << "Someone is setting the RequestedBMCTransition field" <<
        std::endl;
    return server::BMC::requestedBMCTransition(value);
}

BMC::BMCState BMC::currentBMCState(BMCState value)
{
    std::cout << "Someone is being bad and trying to set the BMCState field" <<
            std::endl;

    return server::BMC::currentBMCState(value);
}

} // namespace manager
} // namespace state
} // namepsace phosphor

