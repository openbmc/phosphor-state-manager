#include "config.h"

#include "hypervisor_state_manager.hpp"

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/exception.hpp>
#include <sdbusplus/server.hpp>

#include <fstream>
#include <iostream>
#include <map>
#include <string>

namespace phosphor
{
namespace state
{
namespace manager
{

PHOSPHOR_LOG2_USING;

// When you see server:: you know we're referencing our base class
namespace server = sdbusplus::xyz::openbmc_project::State::server;
using namespace phosphor::logging;

server::Host::Transition Hypervisor::requestedHostTransition(Transition value)
{
    info("Hypervisor state transition request of {TRAN_REQUEST}",
         "TRAN_REQUEST", value);

    // Only support the transition to On
    if (value != server::Host::Transition::On)
    {
        error("Hypervisor state only supports a transition to On");
        // TODO raise appropriate error exception
        return server::Host::Transition::Off;
    }

    // This property is monitored by a separate application (for example PLDM)
    // which is responsible for propagating the On request to the hypervisor

    return server::Host::requestedHostTransition(value);
}

server::Host::HostState Hypervisor::currentHostState(HostState value)
{
    // Only log a message if this has changed since last
    if (value != server::Host::currentHostState())
    {
        info("Change to Hypervisor State: {HYP_STATE}", "HYP_STATE", value);
    }
    return server::Host::currentHostState(value);
}

server::Host::HostState Hypervisor::currentHostState()
{
    return server::Host::currentHostState();
}

void Hypervisor::updateCurrentHostState(std::string& bootProgress)
{
    debug("New BootProgress: {BOOTPROGRESS}", "BOOTPROGRESS", bootProgress);

    if (bootProgress == "xyz.openbmc_project.State.Boot.Progress."
                        "ProgressStages.SystemInitComplete")
    {
        currentHostState(server::Host::HostState::Standby);
    }
    else if (bootProgress == "xyz.openbmc_project.State.Boot.Progress."
                             "ProgressStages.OSRunning")
    {
        currentHostState(server::Host::HostState::Running);
    }
    else if (bootProgress == "xyz.openbmc_project.State.Boot.Progress."
                             "ProgressStages.Unspecified")
    {
        // Unspecified is set when the system is powered off so
        // set the state to off and reset the requested host state
        // back to its default
        currentHostState(server::Host::HostState::Off);
        server::Host::requestedHostTransition(server::Host::Transition::Off);
    }
    else
    {
        // BootProgress changed and it is not one of the above so
        // set hypervisor state to off
        currentHostState(server::Host::HostState::Off);
    }
}

void Hypervisor::bootProgressChangeEvent(sdbusplus::message_t& msg)
{
    std::string statusInterface;
    std::map<std::string, std::variant<std::string>> msgData;
    msg.read(statusInterface, msgData);

    auto propertyMap = msgData.find("BootProgress");
    if (propertyMap != msgData.end())
    {
        // Extract the BootProgress
        auto& bootProgress = std::get<std::string>(propertyMap->second);
        updateCurrentHostState(bootProgress);
    }
}

} // namespace manager
} // namespace state
} // namespace phosphor
