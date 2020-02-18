#include <cstdlib>
#include <iostream>
#include <exception>
#include <sdbusplus/bus.hpp>
#include "config.h"
#include "scheduled_host_transition.hpp"

int main(int argc, char *argv[])
{
    auto bus = sdbusplus::bus::new_default();

    // Add sdbusplus ObjectManager.
    sdbusplus::server::manager::manager objManager(
        bus, SCHEDULED_HOST_TRANSITION_OBJPATH);

    phosphor::state::manager::ScheduledHostTransition manager(
        bus, SCHEDULED_HOST_TRANSITION_OBJPATH);

    bus.request_name(SCHEDULED_HOST_TRANSITION_BUSNAME);

    while (true)
    {
        bus.process_discard();
        bus.wait();
    }
    return 0;
}
