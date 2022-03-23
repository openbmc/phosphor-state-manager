#include "config.h"

#include "host_state_manager.hpp"

#include <fmt/format.h>
#include <getopt.h>

#include <sdbusplus/bus.hpp>

#include <cstdlib>
#include <exception>
#include <filesystem>
#include <iostream>

constexpr auto LEGACY_HOST_STATE_PERSIST_PATH =
    "/var/lib/phosphor-state-manager/requestedHostTransition";

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

    if (hostId == 0)
    {
        // Host State Manager was only support single-host and there only one
        // file to store persist values, to support multi-host state management,
        // each service instance access new file path format with prefix 'hostN'
        // now.For backward compatibility if there is a legacy persist file
        // exist, rename it to the new file format of host0.

        fs::path legacyPath{LEGACY_HOST_STATE_PERSIST_PATH};
        fs::path newPath{fmt::format(HOST_STATE_PERSIST_PATH, hostId)};
        if (fs::exists(legacyPath))
        {
            fs::rename(legacyPath, newPath);
        }
    }

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
