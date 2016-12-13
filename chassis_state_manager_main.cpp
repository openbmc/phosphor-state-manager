#include <cstdlib>
#include <iostream>
#include <exception>
#include <sdbusplus/bus.hpp>
#include "config.h"
#include "chassis_state_manager.hpp"


int main(int argc, char *argv[])
{
    auto bus = sdbusplus::bus::new_default();

    // For now, we only have one instance of the chassis
    std::string objPathInst = {CHASSIS_OBJPATH};
    objPathInst+="0";

    auto manager = phosphor::state::manager::Chassis(bus,
                                                     CHASSIS_BUSNAME,
                                                     objPathInst.c_str());

    bus.request_name(CHASSIS_BUSNAME);

    while(true)
    {
        bus.process_discard();
        bus.wait();
    }

    exit(EXIT_SUCCESS);

}
