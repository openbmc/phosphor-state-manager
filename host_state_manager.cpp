#include <iostream>
#include <systemd/sd-bus.h>
#include "host_state_manager.hpp"

namespace phosphor
{
namespace statemanager
{

using namespace sdbusplus::xyz::openbmc_project::State::server;

Host::Host(
        sdbusplus::bus::bus& bus,
        const char* busName,
        const char* objPath) :
        sdbusplus::server::object::object<
            sdbusplus::xyz::openbmc_project::State::server::Host>(
               bus, objPath),
         _bus(bus),
         _path(objPath) {}

// TODO - Will be rewritten once sdbusplus client bindings are in place
void Host::determineInitialState()
{
    int rc = 0;
    int pgood = -1;
    sd_bus_message *response = NULL;
    sd_bus_error busError = SD_BUS_ERROR_NULL;
    sd_bus *bus = NULL;
    rc = sd_bus_open_system(&bus);
    if(rc < 0)
    {
        std::cerr << "Failed to open system bus" << std::endl;
        goto finish;
    }

    rc = sd_bus_get_property(bus,
                             "org.openbmc.control.Power",
                             "/org/openbmc/control/power0",
                             "org.openbmc.control.Power",
                             "pgood",
                             &busError,
                             &response,
                             "i");
    if(rc < 0)
    {
        std::cerr << "Failed to read pgood property" << std::endl;
        goto finish;
    }

    rc = sd_bus_message_read(response, "i", &pgood);
    if(rc < 0)
    {
        std::cerr << "Failed to extract pgood from msg" << std::endl;
        goto finish;
    }

    if(pgood==1)
    {
        std::cout << "HOST is BOOTED " << pgood << std::endl;
        sdbusplus::xyz::openbmc_project::State::server::Host::
            currentHostState(HostState::Running);
    }
    else
    {
        std::cout << "HOST is not BOOTED " << pgood << std::endl;
        sdbusplus::xyz::openbmc_project::State::server::Host::
            currentHostState(HostState::Off);
    }

finish:
    sd_bus_error_free(&busError);
    response = sd_bus_message_unref(response);
    sd_bus_unref(bus);

    return;
}

void Host::run() noexcept
{
    determineInitialState();

    while(true)
    {
        try
        {
            _bus.process_discard();
            _bus.wait();
        }
        catch (std::exception &e)
        {
            std::cerr << e.what() << std::endl;
        }
    }
}


Host::Transition Host::hostTransition(Transition value)
{
    std::cout << "Someone is setting the HostTransition field" << std::endl;
    sdbusplus::xyz::openbmc_project::State::server::Host::
                hostTransition(value);

    return sdbusplus::xyz::openbmc_project::State::server::Host::
            hostTransition();
}


Host::HostState Host::currentHostState(HostState value)
{
    std::cout << "Someone is being bad and trying to set the HostState field" <<
            std::endl;

    return sdbusplus::xyz::openbmc_project::State::server::Host::
            currentHostState();
}

} // namespace statemanager
} // namepsace phosphor
