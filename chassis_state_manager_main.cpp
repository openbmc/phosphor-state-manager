#include <cstdlib>
#include <iostream>
#include <exception>
#include <sdbusplus/bus.hpp>
#include "config.h"
#include "chassis_state_manager.hpp"

int main()
{
    auto bus = sdbusplus::bus::new_default();

    // For now, we only have one instance of the chassis
    auto objPathInst = std::string{CHASSIS_OBJPATH} + '0';

    // Add sdbusplus ObjectManager.
    sdbusplus::server::manager::manager objManager(bus, objPathInst.c_str());

    phosphor::state::manager::Chassis manager(bus, CHASSIS_BUSNAME,
                                              objPathInst.c_str());

    bus.request_name(CHASSIS_BUSNAME);
    manager.startPOHCounter();

    return 0;
}
