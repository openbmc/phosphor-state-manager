#include <cstdlib>
#include <iostream>
#include <exception>
#include <sdbusplus/bus.hpp>
#include "config.h"
#include "scheduled_host_transition.hpp"

int main()
{
    // Get a default event loop
    auto event = sdeventplus::Event::get_default();

    // Get a handle to system dbus
    auto bus = sdbusplus::bus::new_default();

    // Add sdbusplus ObjectManager.
    sdbusplus::server::manager::manager objManager(
        bus, SCHEDULED_HOST_TRANSITION_OBJPATH);

    phosphor::state::manager::ScheduledHostTransition manager(
        bus, SCHEDULED_HOST_TRANSITION_OBJPATH, event);

    bus.request_name(SCHEDULED_HOST_TRANSITION_BUSNAME);

    // Attach the bus to sd_event to service user requests
    bus.attach_event(event.get(), SD_EVENT_PRIORITY_NORMAL);
    event.loop();

    return 0;
}
