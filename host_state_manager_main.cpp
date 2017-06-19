#include <cstdlib>
#include <iostream>
#include <exception>
#include <sdbusplus/bus.hpp>
#include "config.h"
#include "host_state_manager.hpp"
#include <xyz/openbmc_project/State/Boot/Progress/server.hpp>
#include <xyz/openbmc_project/Control/Boot/RebootAttempts/server.hpp>
#include <xyz/openbmc_project/State/OperatingSystem/Status/server.hpp>

int main(int argc, char *argv[])
{
    auto bus = sdbusplus::bus::new_default();

    using BootProgress = sdbusplus::xyz::openbmc_project::State::
                         Boot::server::Progress;
    using BootCount = sdbusplus::xyz::openbmc_project::Control::
                         Boot::server::RebootAttempts;
    using OsStatus = sdbusplus::xyz::openbmc_project::State::
                         OperatingSystem::server::Status;

    // For now, we only have one instance of the host
    auto objPathInst = std::string{HOST_OBJPATH} + '0';

    // Add sdbusplus ObjectManager.
    sdbusplus::server::manager::manager objManager(bus, objPathInst.c_str());

    phosphor::state::manager::Host manager(bus,
                                           HOST_BUSNAME,
                                           objPathInst.c_str());

    bus.request_name(HOST_BUSNAME);

    //Boot progress
    BootProgress bootProgress(bus, BOOT_PROGRESS_OBJPATH);

    //Boot Count
    sdbusplus::server::manager::manager BootCountObjManager(bus,
            BOOT_COUNT_OBJPATH);

    BootCount bootCount(bus, BOOT_COUNT_OBJPATH);

    //set boot count to initial default value
    bootCount.attemptsLeft(BOOT_COUNT_MAX_ALLOWED);

    //OperatingSystem Status
    OsStatus osStatus(bus, OS_STATUS_OBJPATH);

    while(true)
    {
        bus.process_discard();
        bus.wait();
    }
    return 0;
}
