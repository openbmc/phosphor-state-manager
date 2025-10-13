#include "config.h"

#include "scheduled_host_transition.hpp"

#include <getopt.h>

#include <sdbusplus/bus.hpp>

#include <cstdlib>
#include <exception>
#include <filesystem>

using ScheduledHostTransition =
    sdbusplus::server::xyz::openbmc_project::state::ScheduledHostTransition;

int main(int argc, char** argv)
{
    size_t hostId = 0;

    int arg;
    int optIndex = 0;

    static struct option longOpts[] = {
        {"host", required_argument, nullptr, 'h'}, {nullptr, 0, nullptr, 0}};

    while ((arg = getopt_long(argc, argv, "h:", longOpts, &optIndex)) != -1)
    {
        switch (arg)
        {
            case 'h':
                hostId = std::stoul(optarg);
                break;
            default:
                break;
        }
    }

    namespace fs = std::filesystem;

    // Get a default event loop
    auto event = sdeventplus::Event::get_default();

    // Get a handle to system dbus
    auto bus = sdbusplus::bus::new_default();

    // For now, we only have one instance of the host
    auto objPathInst = std::string{HOST_SCHED_OBJPATH} + std::to_string(hostId);

    // Check SCHEDULED_HOST_TRANSITION_PERSIST_PATH
    auto dir = fs::path(SCHEDULED_HOST_TRANSITION_PERSIST_PATH).parent_path();
    if (!fs::exists(dir))
    {
        fs::create_directories(dir);
    }

    // Add sdbusplus ObjectManager.
    sdbusplus::server::manager_t objManager(bus, objPathInst.c_str());

    phosphor::state::manager::ScheduledHostTransition manager(
        bus, objPathInst.c_str(), hostId, event);

    // For backwards compatibility, request a busname without host id if
    // input id is 0.
    if (hostId == 0)
    {
        bus.request_name(ScheduledHostTransition::interface);
    }

    bus.request_name((std::string{ScheduledHostTransition::interface} +
                      std::to_string(hostId))
                         .c_str());

    // Attach the bus to sd_event to service user requests
    bus.attach_event(event.get(), SD_EVENT_PRIORITY_NORMAL);
    event.loop();

    return 0;
}
