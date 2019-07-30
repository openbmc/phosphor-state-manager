#include <phosphor-logging/log.hpp>
#include <sdbusplus/bus.hpp>
#include <sdeventplus/event.hpp>

using phosphor::logging::level;
using phosphor::logging::log;

int main(int argc, char* argv[])
{
    log<level::INFO>("Starting systemd target monitor");
    auto bus = sdbusplus::bus::new_default();
    auto event = sdeventplus::Event::get_default();
    bus.attach_event(event.get(), SD_EVENT_PRIORITY_NORMAL);

    // TODO - Process input parameters

    // TODO - Load in json config file(s)

    // TODO - Begin monitoring for systemd unit changes and logging appropriate
    //        errors

    return event.loop();
}
