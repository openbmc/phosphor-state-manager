#include <cstdlib>
#include <iostream>
#include <exception>
#include <sdbusplus/bus.hpp>
#include "config.h"
#include "host_state_manager.hpp"

int main(int argc, char *argv[])
{
    auto bus = sdbusplus::bus::new_default();

    auto manager = phosphor::state::manager::Host(bus,
                                                  BUSNAME,
                                                  OBJPATH);
    bus.request_name(BUSNAME);

    while(true)
    {
        bus.process_discard();
        bus.wait();
    }

    exit(EXIT_SUCCESS);
}
