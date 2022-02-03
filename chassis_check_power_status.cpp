#include "config.h"

#include "utils.hpp"

#include <getopt.h>
#include <systemd/sd-bus.h>

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/exception.hpp>
#include <sdbusplus/server.hpp>

#include <iostream>
#include <map>
#include <string>

PHOSPHOR_LOG2_USING;

using namespace phosphor::logging;
using namespace sdbusplus::xyz::openbmc_project::Common::Error;

int main(int argc, char** argv)
{
    std::string chassisPath = "/xyz/openbmc_project/state/chassis0";
    int arg;
    int optIndex = 0;

    static struct option longOpts[] = {{"chassis", required_argument, 0, 'c'},
                                       {0, 0, 0, 0}};

    while ((arg = getopt_long(argc, argv, "c:", longOpts, &optIndex)) != -1)
    {
        switch (arg)
        {
            case 'c':
                chassisPath =
                    std::string("/xyz/openbmc_project/state/chassis") + optarg;
                break;
            default:
                break;
        }
    }

    auto bus = sdbusplus::bus::new_default();

    // If the chassis power status is not good, log an error and exit with
    // a non-zero rc so the system does not power on
    auto currentPowerStatus = phosphor::state::manager::utils::getProperty(
        bus, chassisPath, CHASSIS_BUSNAME, "CurrentPowerStatus");
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
            sdbusplus::xyz::openbmc_project::Logging::server::Entry::Level::
                Critical);
        return -1;
    }
    // all good
    info("Chassis power status good, start power on");
    return 0;
}
