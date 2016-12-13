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

Chassis::Transition Chassis::requestedPowerTransition(Transition value)
{

    log<level::INFO>("Change to Chassis Requested Power State",
                     entry("CHASSIS_REQUESTED_POWER_STATE=0x%.2X",value));
    return server::Chassis::requestedPowerTransition(value);
}

Chassis::PowerState Chassis::currentPowerState(PowerState value)
{
    log<level::INFO>("Change to Chassis Power State",
                     entry("CURRENT_CHASSIS_POWER_STATE=0x%.2X",value));
    return server::Chassis::currentPowerState(value);
}

} // namespace manager
} // namespace state
} // namepsace phosphor
