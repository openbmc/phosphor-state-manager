#include "config.h"

#include "chassis_state_manager.hpp"

#include <fmt/format.h>
#include <getopt.h>

#include <sdbusplus/bus.hpp>

#include <cstdlib>
#include <exception>
#include <filesystem>
#include <iostream>

constexpr auto LEGACY_POH_COUNTER_PERSIST_PATH =
    "/var/lib/phosphor-state-manager/POHCounter";
constexpr auto LEGACY_STATE_CHANGE_PERSIST_PATH =
    "/var/lib/phosphor-state-manager/chassisStateChangeTime";

int main(int argc, char** argv)
{
    size_t chassisId = 0;
    int arg;
    int optIndex = 0;
    static struct option longOpts[] = {{"chassis", required_argument, 0, 'c'},
                                       {0, 0, 0, 0}};

    while ((arg = getopt_long(argc, argv, "c:", longOpts, &optIndex)) != -1)
    {
        switch (arg)
        {
            case 'c':
                chassisId = std::stoul(optarg);
                break;
            default:
                break;
        }
    }

    namespace fs = std::filesystem;

    auto bus = sdbusplus::bus::new_default();

    auto chassisBusName =
        std::string{CHASSIS_BUSNAME} + std::to_string(chassisId);
    auto objPathInst = std::string{CHASSIS_OBJPATH} + std::to_string(chassisId);

    if (chassisId == 0)
    {
        // Chassis State Manager was only support single-chassis and there only
        // two file to store persist values(POH and state change time), to
        // support multi-chassis state mamagement, each service access new file
        // paths with prefix 'chassisN', if any legacy persist file is exist,
        // rename it to the new file format of chassis0.

        fs::path legacyPohPath{LEGACY_POH_COUNTER_PERSIST_PATH};
        fs::path legacyStateChangePath{LEGACY_STATE_CHANGE_PERSIST_PATH};
        fs::path newPohPath{fmt::format(POH_COUNTER_PERSIST_PATH, chassisId)};
        fs::path newStateChangePath{
            fmt::format(CHASSIS_STATE_CHANGE_PERSIST_PATH, chassisId)};
        if (fs::exists(legacyPohPath))
        {
            fs::rename(legacyPohPath, newPohPath);
        }
        if (fs::exists(legacyStateChangePath))
        {
            fs::rename(legacyStateChangePath, newStateChangePath);
        }
    }

    // Add sdbusplus ObjectManager.
    sdbusplus::server::manager::manager objManager(bus, objPathInst.c_str());
    phosphor::state::manager::Chassis manager(bus, objPathInst.c_str(),
                                              chassisId);

    // For backwards compatibility, request a busname without chassis id if
    // input id is 0.
    if (chassisId == 0)
    {
        bus.request_name(CHASSIS_BUSNAME);
    }

    bus.request_name(chassisBusName.c_str());
    manager.startPOHCounter();
    return 0;
}
