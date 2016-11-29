#include "host_state_manager.hpp"
#include <sdbusplus/bus.hpp>
#include <cstdlib>
#include <iostream>
#include <exception>

int main(int argc, char *argv[])
{
    const char *busName = "xyz.openbmc_project.State.Host";
    const char *objPath = "/xyz/openbmc_project/state/host";

    try {
            auto bus = sdbusplus::bus::new_system();
            bus.request_name(busName);
            auto manager = phosphor::statemanager::Host(
                bus,
                busName,
                objPath);
        manager.run();
        exit(EXIT_SUCCESS);
    }
    catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
    }
    exit(EXIT_FAILURE);
}
