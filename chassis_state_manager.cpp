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
        int instance,
        const char* objPath) :
        sdbusplus::server::object::object<
            server::Chassis>(
               bus, objPath),
         bus(bus),
         instance(instance)
{
    determineInitialState();
}

// TODO - Will be rewritten once sdbusplus client bindings are in place
//        and persistent storage design is in place and sdbusplus
//        has read property function
void Chassis::determineInitialState()
{
    int rc = 0;
    int pgood = -1;
    sd_bus_message *response = nullptr;
    sd_bus *bus = nullptr;
    rc = sd_bus_open_system(&bus);
    if(rc < 0)
    {
        log<level::ERR>("Failed to open system bus");
        goto finish;
    }

    rc = sd_bus_get_property(bus,
                             "org.openbmc.control.Power",
                             "/org/openbmc/control/power0",
                             "org.openbmc.control.Power",
                             "pgood",
                             nullptr,
                             &response,
                             "i");
    if(rc < 0)
    {
        log<level::ERR>("Failed to read pgood property");
        goto finish;
    }

    rc = sd_bus_message_read(response, "i", &pgood);
    if(rc < 0)
    {
        log<level::ERR>("Failed to extract pgood from msg");
        goto finish;
    }

    if(pgood==1)
    {
        log<level::INFO>("Initial Chassis State will be On",
                         entry("CURRENT_CHASSIS_POWER_STATE=0x%.2X",
                               PowerState::On));
        server::Chassis::currentPowerState(PowerState::On);
        server::Chassis::requestedPowerTransition(Transition::On);
    }
    else
    {
        log<level::INFO>("Initial Chassis State will be Off",
                         entry("CURRENT_CHASSIS_POWER_STATE=0x%.2X",
                               PowerState::Off));
        server::Chassis::currentPowerState(PowerState::Off);
        server::Chassis::requestedPowerTransition(Transition::Off);
    }

finish:
    response = sd_bus_message_unref(response);
    sd_bus_unref(bus);

    return;
}

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
