#include "drive_state_manager.hpp"

#include <getopt.h>

#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/bus.hpp>

#include <format>
#include <string>

PHOSPHOR_LOG2_USING;

using DriveState = sdbusplus::server::xyz::openbmc_project::state::Drive;

int main(int argc, char** argv)
{
    std::string driveInstance;
    int arg;
    int optIndex = 0;

    static struct option longOpts[] = {
        {"drive", required_argument, nullptr, 'd'}, {nullptr, 0, nullptr, 0}};

    while ((arg = getopt_long(argc, argv, "d:", longOpts, &optIndex)) != -1)
    {
        switch (arg)
        {
            case 'd':
                driveInstance = optarg;
                break;
            default:
                break;
        }
    }

    if (driveInstance.empty())
    {
        error("Drive instance name is required (--drive <name>)");
        return 1;
    }

    auto bus = sdbusplus::bus::new_default();

    const auto* objPath = DriveState::namespace_path::value;
    auto driveBasePath = sdbusplus::message::object_path(objPath) /
                         std::string(DriveState::namespace_path::drive);
    auto driveBusName =
        std::string(DriveState::interface) + "." + driveInstance;
    auto objPathInst = driveBasePath / driveInstance;

    sdbusplus::server::manager_t objManager(bus, driveBasePath.str.c_str());
    phosphor::state::manager::Drive manager(bus, objPathInst.c_str(),
                                            driveInstance);

    bus.request_name(driveBusName.c_str());

    while (true)
    {
        bus.process_discard();
        bus.wait();
    }

    return 0;
}
