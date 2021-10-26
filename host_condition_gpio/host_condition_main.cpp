#include "config.h"

#include "host_condition.hpp"

#include <boost/algorithm/string.hpp>
#include <sdbusplus/bus.hpp>

#include <cstdlib>
#include <iostream>

int main(int argc, char** argv)
{
    std::string hostId;

    if (argc == 2)
    {
        hostId = std::string(argv[1]);
    }
    else
    {
        return 0;
    }

    auto bus = sdbusplus::bus::new_default();
    std::string objGroupName = HOST_GPIOS_OBJPATH;
    std::string objPathInst = objGroupName + "/host" + hostId;
    std::string busName = HOST_GPIOS_BUSNAME;

    // Add sdbusplus ObjectManager
    sdbusplus::server::manager::manager objManager(bus, objGroupName.c_str());

    // For now, we only support checking Host0 status
    auto host = std::make_unique<phosphor::condition::Host>(
        bus, objPathInst.c_str(), hostId);

    bus.request_name(busName.c_str());

    while (true)
    {
        bus.process_discard();
        bus.wait();
    }

    return 0;
}
