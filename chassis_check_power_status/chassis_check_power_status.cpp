#include "config.h"

#include "utils.hpp"

#include <getopt.h>
#include <systemd/sd-bus.h>

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/exception.hpp>
#include <sdbusplus/server.hpp>
#include <xyz/openbmc_project/Common/error.hpp>
#include <xyz/openbmc_project/State/Chassis/client.hpp>

#include <iostream>
#include <map>
#include <string>

PHOSPHOR_LOG2_USING;

using namespace phosphor::logging;
using namespace sdbusplus::xyz::openbmc_project::Common::Error;

using ChassisState = sdbusplus::client::xyz::openbmc_project::state::Chassis<>;

int main(int argc, char** argv)
{
    size_t chassisId = 0;
    const auto* objPath = ChassisState::namespace_path::value;
    auto chassisBusName = ChassisState::interface + std::to_string(chassisId);
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

    auto chassisName = std::string(ChassisState::namespace_path::chassis) +
                       std::to_string(chassisId);
    std::string chassisPath =
        sdbusplus::message::object_path(objPath) / chassisName;
    auto bus = sdbusplus::bus::new_default();

    // If the chassis power status is not good, log an error and exit with
    // a non-zero rc so the system does not power on
    auto currentPowerStatus = phosphor::state::manager::utils::getProperty(
        bus, chassisPath, ChassisState::interface, "CurrentPowerStatus");
    if (currentPowerStatus !=
        "xyz.openbmc_project.State.Chassis.PowerStatus.Good")
    {
        error("Chassis power status is not good: {CURRENT_PWR_STATUS}",
              "CURRENT_PWR_STATUS", currentPowerStatus);

        // Generate log telling user why system is not powering on
        const std::string errorMsg =
            "xyz.openbmc_project.State.ChassisPowerBad";
        phosphor::state::manager::utils::createError(
            bus, errorMsg,
            sdbusplus::server::xyz::openbmc_project::logging::Entry::Level::
                Critical);
        return -1;
    }
    // all good
    info("Chassis power status good, start power on");
    return 0;
}
