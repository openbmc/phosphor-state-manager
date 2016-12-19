#include <iostream>
#include <string>
#include <systemd/sd-bus.h>
#include "BMC_state_manager.hpp"

namespace phosphor
{
namespace state
{
namespace manager
{

// When you see server:: you know we're referencing our base class
namespace server = sdbusplus::xyz::openbmc_project::State::server;

/* Map a transition to it's systemd target */
const std::map<server::BMC::Transition,std::string> SYSTEMD_TABLE =
{
        {BMC::Transition::Reboot,"reboot.target"}
};

constexpr auto SYSTEMD_SERVICE   = "org.freedesktop.systemd1";
constexpr auto SYSTEMD_OBJ_PATH  = "/org/freedesktop/systemd1";
constexpr auto SYSTEMD_INTERFACE = "org.freedesktop.systemd1.Manager";

/* Map a system state to the BMCState */
const std::map<std::string, server::BMC::BMCState> SYS_BMC_STATE_TABLE =
{
        {"BMC_READY", server::BMC::BMCState::Ready},
};

void BMC::subscribeToSystemdSignals()
{
    auto method = this->bus.new_method_call(SYSTEMD_SERVICE,
                                            SYSTEMD_OBJ_PATH,
                                            SYSTEMD_INTERFACE,
                                            "Subscribe");
    this->bus.call(method);

    //Set transition intially to None
    //TODO - Eventually need to restore this from persistent storage
    server::BMC::requestedBMCTransition(Transition::None);
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

int BMC::handleSysStateChange(sd_bus_message *msg, void *usrData,
                              sd_bus_error *retError)
{
    uint32_t newStateID;
    sdbusplus::message::object_path newStateObjPath;
    std::string newStateUnit;
    std::string newStateResult;

    auto sdPlusMsg = sdbusplus::message::message(msg);
    //Read the msg and populate each variable
    sdPlusMsg.read(newStateID, newStateObjPath, newStateUnit, newStateResult);

    //Caught the signal that indicates the BMC is now BMC_READY
    if((newStateUnit == "obmc-standby.target") &&
       (newStateResult == "done"))
    {
        auto it = SYS_BMC_STATE_TABLE.find("BMC_READY");
        if(it != SYS_BMC_STATE_TABLE.end())
        {
            BMC::BMCState gotoState = it->second;
            auto BMCInst = static_cast<BMC*>(usrData);
            BMCInst->currentBMCState(gotoState);
        }
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

