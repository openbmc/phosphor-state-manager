#include <sdbusplus/server/manager.hpp>
#include <unistd.h>
#include "config.h"
#include "boot_count.hpp"


int main()
{
    auto bus = sdbusplus::bus::new_default();

    bus.request_name(BOOT_COUNT_BUSNAME);

    // Add sdbusplus ObjectManager.
    sdbusplus::server::manager::manager objManager(bus, BOOT_COUNT_OBJPATH);

    phosphor::control::boot::RebootAttempts bootCount(bus,
            BOOT_COUNT_OBJPATH);

    while (true)
    {
        bus.process_discard();
        bus.wait();
    }

    exit(EXIT_SUCCESS);

}
