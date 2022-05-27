#include "config.h"

#include "host_state_manager.hpp"
#include "settings.hpp"
#include "utils.hpp"
#include "xyz/openbmc_project/Common/error.hpp"
#include "xyz/openbmc_project/Control/Power/RestorePolicy/server.hpp"

#include <fmt/format.h>
#include <getopt.h>
#include <systemd/sd-bus.h>

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/exception.hpp>
#include <sdbusplus/server.hpp>

#include <filesystem>
#include <iostream>
#include <map>
#include <string>

namespace phosphor
{
namespace state
{
namespace manager
{

PHOSPHOR_LOG2_USING;

using namespace phosphor::logging;
using namespace sdbusplus::xyz::openbmc_project::Common::Error;
using namespace sdbusplus::xyz::openbmc_project::Control::Power::server;

} // namespace manager
} // namespace state
} // namespace phosphor

int main(int argc, char** argv)
{
    using namespace phosphor::logging;

    size_t hostId = 0;
    std::string hostPath = "/xyz/openbmc_project/state/host0";
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
                hostPath =
                    std::string("/xyz/openbmc_project/state/host") + optarg;
                break;
            default:
                break;
        }
    }

    auto bus = sdbusplus::bus::new_default();

    using namespace settings;
    HostObjects settings(bus, hostId);

    using namespace phosphor::state::manager;
    namespace server = sdbusplus::xyz::openbmc_project::State::server;

    // This application is only run if chassis power is off

    // If the BMC was rebooted due to a user initiated pinhole reset, do not
    // implement any power restore policies
    auto bmcRebootCause = phosphor::state::manager::utils::getProperty(
        bus, "/xyz/openbmc_project/state/bmc0", BMC_BUSNAME, "LastRebootCause");
    if (bmcRebootCause ==
        "xyz.openbmc_project.State.BMC.RebootCause.PinholeReset")
    {
        info(
            "BMC was reset due to pinhole reset, no power restore policy will be run");
        return 0;
    }
    else if (bmcRebootCause ==
             "xyz.openbmc_project.State.BMC.RebootCause.Watchdog")
    {
        info(
            "BMC was reset due to cold reset, no power restore policy will be run");
        return 0;
    }

    /* The logic here is to first check the one-time PowerRestorePolicy setting.
     * If this property is not the default then look at the persistent
     * user setting in the non one-time object, otherwise honor the one-time
     * setting.
     */
    auto methodOneTime = bus.new_method_call(
        settings.service(settings.powerRestorePolicy, powerRestoreIntf).c_str(),
        settings.powerRestorePolicyOneTime.c_str(),
        "org.freedesktop.DBus.Properties", "Get");
    methodOneTime.append(powerRestoreIntf, "PowerRestorePolicy");

    auto methodUserSetting = bus.new_method_call(
        settings.service(settings.powerRestorePolicy, powerRestoreIntf).c_str(),
        settings.powerRestorePolicy.c_str(), "org.freedesktop.DBus.Properties",
        "Get");
    methodUserSetting.append(powerRestoreIntf, "PowerRestorePolicy");

    std::variant<std::string> result;
    try
    {
        auto reply = bus.call(methodOneTime);
        reply.read(result);
        auto powerPolicy = std::get<std::string>(result);

        if (RestorePolicy::Policy::None ==
            RestorePolicy::convertPolicyFromString(powerPolicy))
        {
            // one_time is set to None so use the customer setting
            info("One time not set, check user setting of power policy");

#ifdef ONLY_RUN_APR_ON_POWER_LOSS
            std::string CHASSIS_LOST_POWER_FMT =
                fmt::format("chassis@{}-lost-power", hostId);
            std::string CHASSIS_LOST_POWER_FILE_FMT =
                "/run/openbmc/" + CHASSIS_LOST_POWER_FMT;
            fs::path chassisPowerLossFile{CHASSIS_LOST_POWER_FILE_FMT};
            if (!fs::exists(chassisPowerLossFile))
            {
                info(
                    "Chassis power was not on prior to BMC reboot so do not run any power policy");
                return 0;
            }
#endif
            auto reply = bus.call(methodUserSetting);
            reply.read(result);
            powerPolicy = std::get<std::string>(result);
        }
        else
        {
            // one_time setting was set so we're going to use it. Reset it
            // to default for next time.
            info("One time set, use it and reset to default");
            phosphor::state::manager::utils::setProperty(
                bus, settings.powerRestorePolicyOneTime.c_str(),
                powerRestoreIntf, "PowerRestorePolicy",
                convertForMessage(RestorePolicy::Policy::None));
        }

        info("Host power is off, processing power policy {POWER_POLICY}",
             "POWER_POLICY", powerPolicy);

        if (RestorePolicy::Policy::AlwaysOn ==
            RestorePolicy::convertPolicyFromString(powerPolicy))
        {
            info("power_policy=ALWAYS_POWER_ON, powering host on");
            phosphor::state::manager::utils::setProperty(
                bus, hostPath, HOST_BUSNAME, "RestartCause",
                convertForMessage(
                    server::Host::RestartCause::PowerPolicyAlwaysOn));
            phosphor::state::manager::utils::setProperty(
                bus, hostPath, HOST_BUSNAME, "RequestedHostTransition",
                convertForMessage(server::Host::Transition::On));
        }
        else if (RestorePolicy::Policy::AlwaysOff ==
                 RestorePolicy::convertPolicyFromString(powerPolicy))
        {
            info("power_policy=ALWAYS_POWER_OFF, set requested state to off");
            // Read last requested state and re-request it to execute it
            auto hostReqState = phosphor::state::manager::utils::getProperty(
                bus, hostPath, HOST_BUSNAME, "RequestedHostTransition");
            if (hostReqState == convertForMessage(server::Host::Transition::On))
            {
                phosphor::state::manager::utils::setProperty(
                    bus, hostPath, HOST_BUSNAME, "RequestedHostTransition",
                    convertForMessage(server::Host::Transition::Off));
            }
        }
        else if (RestorePolicy::Policy::Restore ==
                 RestorePolicy::convertPolicyFromString(powerPolicy))
        {
            info("power_policy=RESTORE, restoring last state");
            // Read last requested state and re-request it to execute it
            auto hostReqState = phosphor::state::manager::utils::getProperty(
                bus, hostPath, HOST_BUSNAME, "RequestedHostTransition");
            if (hostReqState == convertForMessage(server::Host::Transition::On))
            {
                phosphor::state::manager::utils::setProperty(
                    bus, hostPath, HOST_BUSNAME, "RestartCause",
                    convertForMessage(
                        server::Host::RestartCause::PowerPolicyPreviousState));
                phosphor::state::manager::utils::setProperty(
                    bus, hostPath, HOST_BUSNAME, "RequestedHostTransition",
                    hostReqState);
            }
        }
    }
    catch (const sdbusplus::exception::exception& e)
    {
        error("Error in PowerRestorePolicy Get: {ERROR}", "ERROR", e);
        elog<InternalFailure>();
    }

    return 0;
}
