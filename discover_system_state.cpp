#include "config.h"

#include "host_state_manager.hpp"
#include "settings.hpp"
#include "utils.hpp"
#include "xyz/openbmc_project/Common/error.hpp"
#include "xyz/openbmc_project/Control/Power/RestorePolicy/server.hpp"

#include <getopt.h>
#include <systemd/sd-bus.h>

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/exception.hpp>
#include <sdbusplus/server.hpp>
#include <xyz/openbmc_project/State/BMC/client.hpp>
#include <xyz/openbmc_project/State/Host/client.hpp>

#include <filesystem>
#include <iostream>
#include <map>
#include <string>
#include <thread>

namespace phosphor
{
namespace state
{
namespace manager
{

PHOSPHOR_LOG2_USING;

using namespace phosphor::logging;
using namespace sdbusplus::xyz::openbmc_project::Common::Error;
using namespace sdbusplus::server::xyz::openbmc_project::control::power;
using HostState = sdbusplus::client::xyz::openbmc_project::state::Host<>;
using BMCState = sdbusplus::client::xyz::openbmc_project::state::BMC<>;

} // namespace manager
} // namespace state
} // namespace phosphor

int main(int argc, char** argv)
{
    using namespace phosphor::logging;

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

    using Host = sdbusplus::client::xyz::openbmc_project::state::Host<>;
    std::string hostPath =
        std::string(Host::namespace_path::value) + "/" +
        std::string(Host::namespace_path::host) + std::to_string(hostId);

    auto bus = sdbusplus::bus::new_default();

    using namespace settings;
    HostObjects settings(bus, hostId);

    using namespace phosphor::state::manager;
    namespace server = sdbusplus::server::xyz::openbmc_project::state;

    // This application is only run if chassis power is off

    // If the BMC was rebooted due to a user initiated pinhole reset, do not
    // implement any power restore policies
    using BMC = sdbusplus::client::xyz::openbmc_project::state::BMC<>;
    auto bmcPath = sdbusplus::message::object_path(BMC::namespace_path::value) /
                   BMC::namespace_path::bmc;

    auto bmcRebootCause =
        sdbusplus::message::convert_from_string<BMC::RebootCause>(
            phosphor::state::manager::utils::getProperty(
                bus, bmcPath.str, BMCState::interface, "LastRebootCause"));

    if (bmcRebootCause == BMC::RebootCause::PinholeReset)
    {
        info(
            "BMC was reset due to pinhole reset, no power restore policy will be run");
        return 0;
    }
    else if (bmcRebootCause == BMC::RebootCause::Watchdog)
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
                bus, settings.powerRestorePolicyOneTime, powerRestoreIntf,
                "PowerRestorePolicy",
                convertForMessage(RestorePolicy::Policy::None));
        }

        auto methodUserSettingDelay = bus.new_method_call(
            settings.service(settings.powerRestorePolicy, powerRestoreIntf)
                .c_str(),
            settings.powerRestorePolicy.c_str(),
            "org.freedesktop.DBus.Properties", "Get");

        methodUserSettingDelay.append(powerRestoreIntf, "PowerRestoreDelay");

        std::variant<uint64_t> restoreDelay;

        auto delayResult = bus.call(methodUserSettingDelay);
        delayResult.read(restoreDelay);
        auto powerRestoreDelayUsec =
            std::chrono::microseconds(std::get<uint64_t>(restoreDelay));
        auto powerRestoreDelaySec =
            std::chrono::duration_cast<std::chrono::seconds>(
                powerRestoreDelayUsec);

        info("Host power is off, processing power policy {POWER_POLICY}",
             "POWER_POLICY", powerPolicy);

        if (RestorePolicy::Policy::AlwaysOn ==
            RestorePolicy::convertPolicyFromString(powerPolicy))
        {
            info(
                "power_policy=ALWAYS_POWER_ON, powering host on ({DELAY}s delay)",
                "DELAY", powerRestoreDelaySec.count());
            utils::waitBmcReady(bus, powerRestoreDelaySec);
            phosphor::state::manager::utils::setProperty(
                bus, hostPath, HostState::interface, "RestartCause",
                convertForMessage(
                    server::Host::RestartCause::PowerPolicyAlwaysOn));
            phosphor::state::manager::utils::setProperty(
                bus, hostPath, HostState::interface, "RequestedHostTransition",
                convertForMessage(server::Host::Transition::On));
        }
        // Always execute power on if AlwaysOn is set, otherwise check config
        // option (and AC loss status) on whether to execute other policy
        // settings
#if ONLY_RUN_APR_ON_POWER_LOSS
        else if (!phosphor::state::manager::utils::checkACLoss(hostId))
        {
            info(
                "Chassis power was not on prior to BMC reboot so do not run any further power policy");
            return 0;
        }
#endif
        else if (RestorePolicy::Policy::AlwaysOff ==
                 RestorePolicy::convertPolicyFromString(powerPolicy))
        {
            info(
                "power_policy=ALWAYS_POWER_OFF, set requested state to off ({DELAY}s delay)",
                "DELAY", powerRestoreDelaySec.count());
            utils::waitBmcReady(bus, powerRestoreDelaySec);
            // Read last requested state and re-request it to execute it
            auto hostReqState = phosphor::state::manager::utils::getProperty(
                bus, hostPath, HostState::interface, "RequestedHostTransition");
            if (hostReqState !=
                convertForMessage(server::Host::Transition::Off))
            {
                phosphor::state::manager::utils::setProperty(
                    bus, hostPath, HostState::interface,
                    "RequestedHostTransition",
                    convertForMessage(server::Host::Transition::Off));
            }
        }
        else if (RestorePolicy::Policy::Restore ==
                 RestorePolicy::convertPolicyFromString(powerPolicy))
        {
            info("power_policy=RESTORE, restoring last state ({DELAY}s delay)",
                 "DELAY", powerRestoreDelaySec.count());
            utils::waitBmcReady(bus, powerRestoreDelaySec);
            // Read last requested state and re-request it to execute it
            auto hostReqState = phosphor::state::manager::utils::getProperty(
                bus, hostPath, HostState::interface, "RequestedHostTransition");

            // As long as the host transition is not 'Off' power on host state.
            if (hostReqState !=
                convertForMessage(server::Host::Transition::Off))
            {
                phosphor::state::manager::utils::setProperty(
                    bus, hostPath, HostState::interface, "RestartCause",
                    convertForMessage(
                        server::Host::RestartCause::PowerPolicyPreviousState));
                phosphor::state::manager::utils::setProperty(
                    bus, hostPath, HostState::interface,
                    "RequestedHostTransition",
                    convertForMessage(server::Host::Transition::On));
            }
        }
    }
    catch (const sdbusplus::exception_t& e)
    {
        error("Error in PowerRestorePolicy Get: {ERROR}", "ERROR", e);
        elog<InternalFailure>();
    }

    return 0;
}
