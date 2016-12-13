#include <iostream>
#include <systemd/sd-bus.h>
#include <map>
#include <string>
#include <log.hpp>
#include "chassis_state_manager.hpp"

namespace phosphor
{
namespace state
{
namespace manager
{

// When you see server:: you know we're referencing our base class
using namespace sdbusplus::xyz::openbmc_project::State;

using namespace phosphor::logging;


Chassis::Chassis(
        sdbusplus::bus::bus& bus,
        const char* busName,
        const char* objPath) :
        sdbusplus::server::object::object<
            server::Chassis>(
               bus, objPath),
         bus(bus),
         path(objPath)
{}



Chassis::Transition Chassis::requestedPowerTransition(Transition value)
{

    log<level::INFO>("Change to Chassis Requested Power State",
                     entry("CHASSIS_REQUESTED_POWER_STATE=0x%.2X",value));
    return server::Chassis::requestedPowerTransition();
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
