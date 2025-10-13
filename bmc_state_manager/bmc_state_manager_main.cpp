#include "config.h"

#include "bmc_state_manager.hpp"

#include <sdbusplus/bus.hpp>

using BMCState = sdbusplus::server::xyz::openbmc_project::state::BMC;

int main()
{
    auto bus = sdbusplus::bus::new_default();

    // For now, we only have one instance of the BMC
    // 0 is for the current instance
    const auto* BMCName = BMCState::namespace_path::bmc;
    const auto* objPath = BMCState::namespace_path::value;
    std::string objPathInst =
        sdbusplus::message::object_path(objPath) / BMCName;

    // Add sdbusplus ObjectManager.
    sdbusplus::server::manager_t objManager(bus, objPath);

    phosphor::state::manager::BMC manager(bus, objPathInst.c_str());

    bus.request_name(BMCState::interface);

    while (true)
    {
        bus.process_discard();
        bus.wait();
    }

    exit(EXIT_SUCCESS);
}
