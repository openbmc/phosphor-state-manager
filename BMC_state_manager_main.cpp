#include "BMC_state_manager.hpp"
#include "config.h"
#include <sdbusplus/bus.hpp>
#include <cstdlib>
#include <iostream>
#include <exception>

int main(int argc, char *argv[])
{
    auto bus = sdbusplus::bus::new_default();
    int instance = 0; //Instance of the BMC

    std::string objPathInst = {BMC_OBJPATH};
    objPathInst += std::to_string(instance);

    auto manager = phosphor::state::manager::BMC(bus,
                                                 BMC_BUSNAME,
                                                 objPathInst.c_str());
    bus.request_name(BMC_BUSNAME);

    while(true)
    {
        bus.process_discard();
        bus.wait();
    }

    exit(EXIT_SUCCESS);

}
