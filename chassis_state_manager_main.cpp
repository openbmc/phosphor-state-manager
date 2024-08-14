#include "config.h"

#include "chassis_state_manager.hpp"

#include <getopt.h>

#include <sdbusplus/bus.hpp>

#include <cstdlib>
#include <exception>
#include <filesystem>
#include <format>
#include <iostream>

constexpr auto LEGACY_POH_COUNTER_PERSIST_PATH =
    "/var/lib/phosphor-state-manager/POHCounter";
constexpr auto LEGACY_STATE_CHANGE_PERSIST_PATH =
    "/var/lib/phosphor-state-manager/chassisStateChangeTime";

using ChassisState = sdbusplus::server::xyz::openbmc_project::state::Chassis;

int main(int argc, char** argv)
{
    size_t chassisId = 0;
    int arg;
    int optIndex = 0;
    static struct option longOpts[] = {
        {"chassis", required_argument, nullptr, 'c'}, {nullptr, 0, nullptr, 0}};

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

    auto chassisBusName = ChassisState::interface + std::to_string(chassisId);
    const auto* objPath = ChassisState::namespace_path::value;
    auto chassisName = std::string(ChassisState::namespace_path::chassis) +
                       std::to_string(chassisId);
    std::string objPathInst =
        sdbusplus::message::object_path(objPath) / chassisName;

    if (chassisId == 0)
    {
        // Chassis State Manager was only support single-chassis and there only
        // two file to store persist values(POH and state change time), to
        // support multi-chassis state management, each service access new file
        // paths with prefix 'chassisN', if any legacy persist file is exist,
        // rename it to the new file format of chassis0.

        fs::path legacyPohPath{LEGACY_POH_COUNTER_PERSIST_PATH};
        fs::path legacyStateChangePath{LEGACY_STATE_CHANGE_PERSIST_PATH};
        fs::path newPohPath{std::format(POH_COUNTER_PERSIST_PATH, chassisId)};
        fs::path newStateChangePath{
            std::format(CHASSIS_STATE_CHANGE_PERSIST_PATH, chassisId)};
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
    sdbusplus::server::manager_t objManager(bus, objPath);
    phosphor::state::manager::Chassis manager(bus, objPathInst.c_str(),
                                              chassisId);

    // For backwards compatibility, request a busname without chassis id if
    // input id is 0.
    if (chassisId == 0)
    {
        bus.request_name(ChassisState::interface);
    }

    bus.request_name(chassisBusName.c_str());
    manager.startPOHCounter();
    return 0;
}
