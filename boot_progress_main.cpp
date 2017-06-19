#include <unistd.h>
#include <sdbusplus/server/manager.hpp>
#include "config.h"
#include "boot_progress.hpp"


int main()
{
    auto bus = sdbusplus::bus::new_default();

    bus.request_name(BOOT_PROGRESS_BUSNAME);

    // Add sdbusplus ObjectManager.
    sdbusplus::server::manager::manager objManager(bus,
            BOOT_PROGRESS_OBJPATH);

    phosphor::state::boot::Progress bootProgress(bus,
            BOOT_PROGRESS_OBJPATH);

    while (true)
    {
        bus.process_discard();
        bus.wait();
    }

    exit(EXIT_SUCCESS);

}
