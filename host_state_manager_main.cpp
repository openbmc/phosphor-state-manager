#include "host_state_manager.hpp"
#include "config.h"
#include <sdbusplus/bus.hpp>
#include <cstdlib>
#include <iostream>
#include <exception>

int main(int argc, char *argv[])
{
    auto bus = sdbusplus::bus::new_system();

    auto manager = phosphor::statemanager::Host(bus,
                                                BUSNAME,
                                                OBJPATH);

    bus.request_name(BUSNAME);

    while(true)
    {
        try
        {
            bus.process_discard();
            bus.wait();
        }
        catch (std::exception &e)
        {
            std::cerr << e.what() << std::endl;
        }
    }

    exit(EXIT_SUCCESS);

}
