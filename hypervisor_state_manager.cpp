#include "config.h"

#include "hypervisor_state_manager.hpp"

#include <fmt/format.h>

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/log.hpp>
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

// When you see server:: or reboot:: you know we're referencing our base class
namespace server = sdbusplus::xyz::openbmc_project::State::server;
using namespace phosphor::logging;
using sdbusplus::exception::SdBusError;

server::Host::Transition Hypervisor::requestedHostTransition(Transition value)
{
    log<level::INFO>(fmt::format("Hypervisor state transition request of {}",
                                 convertForMessage(value))
                         .c_str());

    // Only support the transition to On
    if (value != server::Host::Transition::On)
    {
        log<level::ERR>("Hypervisor state only supports a transition to On");
        // TODO raise appropriate error exception
        return server::Host::Transition::Off;
    }

    // This property is monitored by a separate application which is
    // responsible for propagating the On request to the hypervisor

    return server::Host::requestedHostTransition(value);
}

// TODO - Monitor BootProgress and update hypervisor state to Running if
//        OS is started

server::Host::HostState Hypervisor::currentHostState(HostState value)
{
    log<level::INFO>(
        fmt::format("Change to Hypervisor State: {}", convertForMessage(value))
            .c_str());
    return server::Host::currentHostState(value);
}

void Hypervisor::bootProgressChangeEvent(sdbusplus::message::message& msg)
{
    std::string statusInterface;
    std::map<std::string, std::variant<std::string>> msgData;
    msg.read(statusInterface, msgData);

    auto propertyMap = msgData.find("BootProgress");
    if (propertyMap != msgData.end())
    {
        // Extract the BootProgress
        auto& bootProgress = std::get<std::string>(propertyMap->second);
        log<level::INFO>(
            fmt::format("New BootProgress: {}", bootProgress).c_str());
    }
    return;
}

} // namespace manager
} // namespace state
} // namespace phosphor
