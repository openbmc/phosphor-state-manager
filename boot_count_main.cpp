#include <sdbusplus/server/manager.hpp>
#include <unistd.h>
#include "config.h"
#include "boot_count.hpp"


int main()
{
    auto bus = sdbusplus::bus::new_default();

    // Add sdbusplus ObjectManager.
    sdbusplus::server::manager::manager objManager(bus, BOOT_COUNT_OBJPATH );

    phosphor::Control::Boot::RebootAttempts boot_count(bus,
                                          BOOT_COUNT_OBJPATH);

    bus.request_name(BOOT_COUNT_BUSNAME);

    while(true)
    {
        bus.process_discard();
        bus.wait();
    }

    exit(EXIT_SUCCESS);

}
