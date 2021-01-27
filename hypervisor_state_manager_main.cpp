#include "config.h"

#include "hypervisor_state_manager.hpp"

#include <sdbusplus/bus.hpp>

#include <cstdlib>

int main()
{
    auto bus = sdbusplus::bus::new_default();

    // For now, we only have one instance of the hypervisor
    auto objPathInst = std::string{HYPERVISOR_OBJPATH} + '0';

    // Add sdbusplus ObjectManager.
    sdbusplus::server::manager::manager objManager(bus, objPathInst.c_str());

    phosphor::state::manager::Hypervisor manager(bus, objPathInst.c_str());

    bus.request_name(HYPERVISOR_BUSNAME);

    while (true)
    {
        bus.process_discard();
        bus.wait();
    }
    return 0;
}
