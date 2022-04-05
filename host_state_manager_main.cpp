#include "config.h"

#include "host_state_manager.hpp"

#include <getopt.h>

#include <sdbusplus/bus.hpp>

#include <cstdlib>
#include <exception>
#include <filesystem>
#include <iostream>

int main(int argc, char** argv)
{
    size_t hostId = 0;

    int arg;
    int optIndex = 0;

    static struct option longOpts[] = {{"host", required_argument, 0, 'h'},
                                       {0, 0, 0, 0}};

    while ((arg = getopt_long(argc, argv, "h:", longOpts, &optIndex)) != -1)
    {
        switch (arg)
        {
            case 'h':
                hostId = std::stoul(optarg);
                break;
            default:
                break;
        }
    }

    namespace fs = std::filesystem;

    auto bus = sdbusplus::bus::new_default();

    auto hostBusName = std::string{HOST_BUSNAME} + std::to_string(hostId);
    auto objPathInst = std::string{HOST_OBJPATH} + std::to_string(hostId);

    // Add sdbusplus ObjectManager.
    sdbusplus::server::manager::manager objManager(bus, objPathInst.c_str());

    phosphor::state::manager::Host manager(bus, objPathInst.c_str(), hostId);

    auto dir = fs::path(HOST_STATE_PERSIST_PATH).parent_path();
    fs::create_directories(dir);

    // For backwards compatibility, request a busname without host id if
    // input id is 0.
    if (hostId == 0)
    {
        bus.request_name(HOST_BUSNAME);
    }

    bus.request_name(hostBusName.c_str());

    while (true)
    {
        bus.process_discard();
        bus.wait();
    }
    return 0;
}
