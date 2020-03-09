#include <cstdlib>
#include <exception>
#include <sdbusplus/bus.hpp>
#include <filesystem>
#include "config.h"
#include "scheduled_host_transition.hpp"

int main()
{
    namespace fs = std::filesystem;

    // Get a default event loop
    auto event = sdeventplus::Event::get_default();

    // Get a handle to system dbus
    auto bus = sdbusplus::bus::new_default();

    // For now, we only have one instance of the host
    auto objPathInst = std::string{HOST_OBJPATH} + '0';

    // Add sdbusplus ObjectManager.
    sdbusplus::server::manager::manager objManager(bus, objPathInst.c_str());

    phosphor::state::manager::ScheduledHostTransition manager(
        bus, objPathInst.c_str(), event);

    auto dir = fs::path(SCHEDULED_HOST_TRANSITION_PERSIST_PATH).parent_path();
    if (!fs::exists(dir))
    {
        fs::create_directories(dir);
    }

    bus.request_name(SCHEDULED_HOST_TRANSITION_BUSNAME);

    // Attach the bus to sd_event to service user requests
    bus.attach_event(event.get(), SD_EVENT_PRIORITY_NORMAL);
    event.loop();

    return 0;
}
