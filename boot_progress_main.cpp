#include <sdbusplus/bus.hpp>
#include "config.h"
#include "boot_progress.hpp"


int main(int argc, char**)
{
    auto bus = sdbusplus::bus::new_default();

    // Add sdbusplus ObjectManager.
    sdbusplus::server::manager::manager objManager(bus, BOOT_PROGRESS_BUSNAME);

    phosphor::state::boot::Progress boot_progress(bus,
                                          BOOT_PROGRESS_OBJPATH);

    bus.request_name(BOOT_PROGRESS_BUSNAME);

    while(true)
    {
        bus.process_discard();
        bus.wait();
    }

    exit(EXIT_SUCCESS);

}
