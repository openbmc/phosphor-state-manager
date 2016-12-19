#include "BMC_state_manager.hpp"
#include "config.h"
#include <sdbusplus/bus.hpp>
#include <cstdlib>
#include <iostream>
#include <exception>

int main(int argc, char *argv[])
{
    auto bus = sdbusplus::bus::new_default();

    // For now, we only have one instance of the BMC
    std::string objPathInst = {OBJPATH};
    objPathInst+="0";
    auto manager = phosphor::state::manager::BMC(bus,
                                                 BUSNAME,
                                                 objPathInst.c_str());
    bus.request_name(BUSNAME);
    while(true)
    {
        bus.process_discard();
        bus.wait();
    }

    exit(EXIT_SUCCESS);

}
