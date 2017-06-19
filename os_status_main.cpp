#include <sdbusplus/server/manager.hpp>
#include <unistd.h>
#include "config.h"
#include "os_status.hpp"


int main(int argc, char**)
{
    auto bus = sdbusplus::bus::new_default();

    // Add sdbusplus ObjectManager.
    sdbusplus::server::manager::manager objManager(bus, OS_STATUS_OBJPATH);

    phosphor::state::os::Status os_status(bus,
            OS_STATUS_OBJPATH);

    bus.request_name(OS_STATUS_BUSNAME);

    while (true)
    {
        bus.process_discard();
        bus.wait();
    }

    exit(EXIT_SUCCESS);

}
