#include <cstdlib>
#include <iostream>
#include <exception>
#include <sdbusplus/bus.hpp>
#include "config.h"
#include "host_state_manager.hpp"


int main(int argc, char *argv[])
{
    auto bus = sdbusplus::bus::new_default();

    // For now, we only have one instance of the host
    std::string objPathInst = {HOST_OBJPATH};
    objPathInst+="0";

    auto manager = phosphor::state::manager::Host(bus,
                                                  HOST_BUSNAME,
                                                  objPathInst.c_str());

    bus.request_name(HOST_BUSNAME);

    while(true)
    {
        bus.process_discard();
        bus.wait();
    }

    exit(EXIT_SUCCESS);

}
