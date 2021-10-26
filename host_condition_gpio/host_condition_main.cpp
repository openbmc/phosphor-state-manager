#include "config.h"

#include "host_condition.hpp"

#include <boost/algorithm/string.hpp>
#include <sdbusplus/bus.hpp>

#include <cstdlib>
#include <iostream>

int main()
{
    auto bus = sdbusplus::bus::new_default();
    std::string objGroupName = HOST_GPIOS_OBJPATH;
    std::string objPathInst = objGroupName + "/host0";
    std::string busName = HOST_GPIOS_BUSNAME;

    // Add sdbusplus ObjectManager
    sdbusplus::server::manager::manager objManager(bus, objGroupName.c_str());

    // For now, we only support checking Host0 status
    auto host =
        std::make_unique<phosphor::condition::Host>(bus, objPathInst.c_str());

    bus.request_name(busName.c_str());

    while (true)
    {
        bus.process_discard();
        bus.wait();
    }

    return 0;
}
