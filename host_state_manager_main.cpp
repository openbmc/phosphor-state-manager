#include "host_state_manager.hpp"
#include "config.h"
#include <sdbusplus/bus.hpp>
#include <cstdlib>
#include <iostream>
#include <exception>

int main(int argc, char *argv[])
{
    auto bus = sdbusplus::bus::new_system();
    bus.request_name(BUSNAME);
    auto manager = phosphor::statemanager::Host(bus,
                                                BUSNAME,
                                                OBJPATH);
    manager.run();

    exit(EXIT_SUCCESS);

}
