#include <iostream>
#include <map>
#include <string>
#include <systemd/sd-bus.h>
#include <log.hpp>
#include "chassis_state_manager.hpp"

namespace phosphor
{
namespace state
{
namespace manager
{

// When you see server:: you know we're referencing our base class
namespace server = sdbusplus::xyz::openbmc_project::State::server;

using namespace phosphor::logging;

// TODO - Will be rewritten once sdbusplus client bindings are in place
//        and persistent storage design is in place and sdbusplus
//        has read property function
void Chassis::determineInitialState()
{
    sdbusplus::message::variant<int> pgood = -1;
    auto method = this->bus.new_method_call("org.openbmc.control.Power",
                                            "/org/openbmc/control/power0",
                                            "org.freedesktop.DBus.Properties",
                                            "Get");

    method.append("org.openbmc.control.Power", "pgood");
    auto reply = this->bus.call(method);
    reply.read(pgood);

    if(pgood == 1)
    {
        log<level::INFO>("Initial Chassis State will be On",
                         entry("CURRENT_CHASSIS_POWER_STATE=%s",
                               convertForMessage(PowerState::On).c_str()));
        server::Chassis::currentPowerState(PowerState::On);
        server::Chassis::requestedPowerTransition(Transition::On);
    }
    else
    {
        log<level::INFO>("Initial Chassis State will be Off",
                         entry("CURRENT_CHASSIS_POWER_STATE=%s",
                               convertForMessage(PowerState::Off).c_str()));
        server::Chassis::currentPowerState(PowerState::Off);
        server::Chassis::requestedPowerTransition(Transition::Off);
    }

    return;
}

Chassis::Transition Chassis::requestedPowerTransition(Transition value)
{

    log<level::INFO>("Change to Chassis Requested Power State",
                     entry("CHASSIS_REQUESTED_POWER_STATE=%s",
                           convertForMessage(value).c_str()));
    return server::Chassis::requestedPowerTransition(value);
}

Chassis::PowerState Chassis::currentPowerState(PowerState value)
{
    log<level::INFO>("Change to Chassis Power State",
                     entry("CURRENT_CHASSIS_POWER_STATE=%s",
                           convertForMessage(value).c_str()));
    return server::Chassis::currentPowerState(value);
}

} // namespace manager
} // namespace state
} // namepsace phosphor
