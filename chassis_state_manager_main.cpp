#include "config.h"

#include "chassis_state_manager.hpp"

#include <getopt.h>

#include <sdbusplus/bus.hpp>

#include <cstdlib>
#include <exception>
#include <iostream>

int main(int argc, char** argv)
{
    size_t chassisId = 0;
    int arg;
    int optIndex = 0;
    static struct option longOpts[] = {{"chassis", required_argument, 0, 'c'},
                                       {0, 0, 0, 0}};

    while ((arg = getopt_long(argc, argv, "c:", longOpts, &optIndex)) != -1)
    {
        switch (arg)
        {
            case 'c':
                chassisId = std::stoul(optarg);
                break;
            default:
                break;
        }
    }

    auto bus = sdbusplus::bus::new_default();

    auto chassisBusName =
        std::string{CHASSIS_BUSNAME} + std::to_string(chassisId);
    auto objPathInst = std::string{CHASSIS_OBJPATH} + std::to_string(chassisId);

    // Add sdbusplus ObjectManager.
    sdbusplus::server::manager::manager objManager(bus, objPathInst.c_str());
    phosphor::state::manager::Chassis manager(bus, objPathInst.c_str(),
                                              chassisId);
    bus.request_name(chassisBusName.c_str());
    manager.startPOHCounter();
    return 0;
}
