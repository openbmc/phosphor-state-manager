#include "config.h"

#include "host_state_manager.hpp"

#include <getopt.h>

#include <sdbusplus/bus.hpp>

#include <cstdlib>
#include <exception>
#include <filesystem>
#include <format>
#include <iostream>

constexpr auto LEGACY_HOST_STATE_PERSIST_PATH =
    "/var/lib/phosphor-state-manager/requestedHostTransition";

using HostState = sdbusplus::server::xyz::openbmc_project::state::Host;

int main(int argc, char** argv)
{
    size_t hostId = 0;

    int arg;
    int optIndex = 0;

    static struct option longOpts[] = {
        {"host", required_argument, nullptr, 'h'}, {nullptr, 0, nullptr, 0}};

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

    auto hostBusName = HostState::interface + std::to_string(hostId);
    auto hostName = std::string(HostState::namespace_path::host) +
                    std::to_string(hostId);
    const auto* objPath = HostState::namespace_path::value;
    std::string objPathInst =
        sdbusplus::message::object_path(objPath) / hostName;

    if (hostId == 0)
    {
        // Host State Manager was only support single-host and there only one
        // file to store persist values, to support multi-host state management,
        // each service instance access new file path format with prefix 'hostN'
        // now.For backward compatibility if there is a legacy persist file
        // exist, rename it to the new file format of host0.

        fs::path legacyPath{LEGACY_HOST_STATE_PERSIST_PATH};
        fs::path newPath{std::format(HOST_STATE_PERSIST_PATH, hostId)};
        if (fs::exists(legacyPath))
        {
            fs::rename(legacyPath, newPath);
        }
    }

    // Add sdbusplus ObjectManager.
    sdbusplus::server::manager_t objManager(bus, objPath);

    phosphor::state::manager::Host manager(bus, objPathInst.c_str(), hostId);

    auto dir = fs::path(HOST_STATE_PERSIST_PATH).parent_path();
    fs::create_directories(dir);

    // For backwards compatibility, request a busname without host id if
    // input id is 0.
    if (hostId == 0)
    {
        bus.request_name(HostState::interface);
    }

    bus.request_name(hostBusName.c_str());

    while (true)
    {
        bus.process_discard();
        bus.wait();
    }
    return 0;
}
