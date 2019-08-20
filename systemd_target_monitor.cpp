#include <sdbusplus/bus.hpp>
#include <sdeventplus/event.hpp>

int main(int argc, char* argv[])
{
    auto bus = sdbusplus::bus::new_default();
    auto event = sdeventplus::Event::get_default();
    bus.attach_event(event.get(), SD_EVENT_PRIORITY_NORMAL);

    // TODO - Process input parameters

    // TODO - Load in json config file(s)

    // TODO - Begin monitoring for systemd unit changes and logging appropriate
    //        errors

    return event.loop();
}
