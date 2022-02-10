#include "config.h"

#include "host_state_manager.hpp"

#include "host_check.hpp"

#include <stdio.h>
#include <systemd/sd-bus.h>

#include <cereal/archives/json.hpp>
#include <cereal/cereal.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/tuple.hpp>
#include <cereal/types/vector.hpp>
#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/exception.hpp>
#include <sdbusplus/server.hpp>
#include <xyz/openbmc_project/Common/error.hpp>
#include <xyz/openbmc_project/Control/Power/RestorePolicy/server.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <string>

// Register class version with Cereal
CEREAL_CLASS_VERSION(phosphor::state::manager::Host, CLASS_VERSION)

namespace phosphor
{
namespace state
{
namespace manager
{

PHOSPHOR_LOG2_USING;

// When you see server:: or reboot:: you know we're referencing our base class
namespace server = sdbusplus::xyz::openbmc_project::State::server;
namespace reboot = sdbusplus::xyz::openbmc_project::Control::Boot::server;
namespace bootprogress = sdbusplus::xyz::openbmc_project::State::Boot::server;
namespace osstatus =
    sdbusplus::xyz::openbmc_project::State::OperatingSystem::server;
using namespace phosphor::logging;
namespace fs = std::experimental::filesystem;
using sdbusplus::xyz::openbmc_project::Common::Error::InternalFailure;

// host-shutdown notifies host of shutdown and that leads to host-stop being
// called so initiate a host shutdown with the -shutdown target and consider the
// host shut down when the -stop target is complete
std::string HOST_STATE_SOFT_POWEROFF_TGT;
std::string HOST_STATE_POWEROFF_TGT;
std::string HOST_STATE_POWERON_TGT;
std::string HOST_STATE_POWERON_MIN_TGT;
std::string HOST_STATE_REBOOT_TGT;
std::string HOST_STATE_WARM_REBOOT;
std::string HOST_STATE_FORCE_WARM_REBOOT;
std::string HOST_STATE_DIAGNOSTIC_MODE;

std::string HOST_STATE_QUIESCE_TGT;

constexpr auto ACTIVE_STATE = "active";
constexpr auto ACTIVATING_STATE = "activating";

/* Map a transition to it's systemd target */
std::map<server::Host::Transition, std::string> SYSTEMD_TARGET_TABLE;

constexpr auto SYSTEMD_SERVICE = "org.freedesktop.systemd1";
constexpr auto SYSTEMD_OBJ_PATH = "/org/freedesktop/systemd1";
constexpr auto SYSTEMD_INTERFACE = "org.freedesktop.systemd1.Manager";

constexpr auto SYSTEMD_PROPERTY_IFACE = "org.freedesktop.DBus.Properties";
constexpr auto SYSTEMD_INTERFACE_UNIT = "org.freedesktop.systemd1.Unit";

void Host::subscribeToSystemdSignals()
{
    auto method = this->bus.new_method_call(SYSTEMD_SERVICE, SYSTEMD_OBJ_PATH,
                                            SYSTEMD_INTERFACE, "Subscribe");
    try
    {
        this->bus.call_noreply(method);
    }
    catch (const sdbusplus::exception::exception& e)
    {
        error("Failed to subscribe to systemd signals: {ERROR}", "ERROR", e);
        elog<InternalFailure>();
    }
    return;
}

void Host::determineInitialState()
{

    if (stateActive(HOST_STATE_POWERON_MIN_TGT) || isHostRunning())
    {
        info("Initial Host State will be Running");
        server::Host::currentHostState(HostState::Running);
        server::Host::requestedHostTransition(Transition::On);
    }
    else
    {
        info("Initial Host State will be Off");
        server::Host::currentHostState(HostState::Off);
        server::Host::requestedHostTransition(Transition::Off);
    }

    if (!deserialize(HOST_STATE_PERSIST_PATH))
    {
        // set to default value.
        server::Host::requestedHostTransition(Transition::Off);
    }

    return;
}

void Host::createSystemdTargets()
{
    HOST_STATE_SOFT_POWEROFF_TGT = "obmc-host-shutdown@"+ hostId + ".target";
    HOST_STATE_POWEROFF_TGT = "obmc-host-stop@" + hostId + ".target";
    HOST_STATE_POWERON_TGT = "obmc-host-start@" + hostId + ".target";
    HOST_STATE_POWERON_MIN_TGT = "obmc-host-startmin@" + hostId + ".target";
    HOST_STATE_REBOOT_TGT = "obmc-host-reboot@" + hostId + ".target";
    HOST_STATE_WARM_REBOOT = "obmc-host-warm-reboot@" + hostId + ".target";
    HOST_STATE_FORCE_WARM_REBOOT =
        "obmc-host-force-warm-reboot@" + hostId + ".target";
    HOST_STATE_DIAGNOSTIC_MODE =
        "obmc-host-diagnostic-mode@" + hostId + ".target";

    HOST_STATE_QUIESCE_TGT = "obmc-host-quiesce@" + hostId + ".target";

    /* Map a transition to it's systemd target */
    SYSTEMD_TARGET_TABLE = {
        {server::Host::Transition::Off, HOST_STATE_SOFT_POWEROFF_TGT},
        {server::Host::Transition::On, HOST_STATE_POWERON_TGT},
        {server::Host::Transition::Reboot, HOST_STATE_REBOOT_TGT},
    // Some systems do not support a warm reboot so just map the reboot
    // requests to our normal cold reboot in that case
    #if ENABLE_WARM_REBOOT
        {server::Host::Transition::GracefulWarmReboot, HOST_STATE_WARM_REBOOT},
        {server::Host::Transition::ForceWarmReboot, HOST_STATE_FORCE_WARM_REBOOT}};
    #else
        {server::Host::Transition::GracefulWarmReboot, HOST_STATE_REBOOT_TGT},
        {server::Host::Transition::ForceWarmReboot, HOST_STATE_REBOOT_TGT}};
    #endif
}

void Host::executeTransition(Transition tranReq)
{
    auto sysdUnit = SYSTEMD_TARGET_TABLE.find(tranReq)->second;

    auto method = this->bus.new_method_call(SYSTEMD_SERVICE, SYSTEMD_OBJ_PATH,
                                            SYSTEMD_INTERFACE, "StartUnit");

    method.append(sysdUnit);
    method.append("replace");

    this->bus.call_noreply(method);

    return;
}

bool Host::stateActive(const std::string& target)
{
    std::variant<std::string> currentState;
    sdbusplus::message::object_path unitTargetPath;

    auto method = this->bus.new_method_call(SYSTEMD_SERVICE, SYSTEMD_OBJ_PATH,
                                            SYSTEMD_INTERFACE, "GetUnit");

    method.append(target);

    try
    {
        auto result = this->bus.call(method);
        result.read(unitTargetPath);
    }
    catch (const sdbusplus::exception::exception& e)
    {
        error("Error in GetUnit call: {ERROR}", "ERROR", e);
        return false;
    }

    method = this->bus.new_method_call(
        SYSTEMD_SERVICE,
        static_cast<const std::string&>(unitTargetPath).c_str(),
        SYSTEMD_PROPERTY_IFACE, "Get");

    method.append(SYSTEMD_INTERFACE_UNIT, "ActiveState");

    try
    {
        auto result = this->bus.call(method);
        result.read(currentState);
    }
    catch (const sdbusplus::exception::exception& e)
    {
        error("Error in ActiveState Get: {ERROR}", "ERROR", e);
        return false;
    }

    const auto& currentStateStr = std::get<std::string>(currentState);
    return currentStateStr == ACTIVE_STATE ||
           currentStateStr == ACTIVATING_STATE;
}

bool Host::isAutoReboot()
{
    using namespace settings;

    /* The logic here is to first check the one-time AutoReboot setting.
     * If this property is true (the default) then look at the persistent
     * user setting in the non one-time object, otherwise honor the one-time
     * setting and do not auto reboot.
     */
    auto methodOneTime = bus.new_method_call(
        settings.service(settings.autoReboot, autoRebootIntf).c_str(),
        settings.autoRebootOneTime.c_str(), SYSTEMD_PROPERTY_IFACE, "Get");
    methodOneTime.append(autoRebootIntf, "AutoReboot");

    auto methodUserSetting = bus.new_method_call(
        settings.service(settings.autoReboot, autoRebootIntf).c_str(),
        settings.autoReboot.c_str(), SYSTEMD_PROPERTY_IFACE, "Get");
    methodUserSetting.append(autoRebootIntf, "AutoReboot");

    try
    {
        auto reply = bus.call(methodOneTime);
        std::variant<bool> result;
        reply.read(result);
        auto autoReboot = std::get<bool>(result);

        if (!autoReboot)
        {
            info("Auto reboot (one-time) disabled");
            return false;
        }
        else
        {
            // one-time is true so read the user setting
            reply = bus.call(methodUserSetting);
            reply.read(result);
            autoReboot = std::get<bool>(result);
        }

        auto rebootCounterParam = reboot::RebootAttempts::attemptsLeft();

        if (autoReboot)
        {
            if (rebootCounterParam > 0)
            {
                // Reduce BOOTCOUNT by 1
                info(
                    "Auto reboot enabled and boot count at {BOOTCOUNT}, rebooting",
                    "BOOTCOUNT", rebootCounterParam);
                return true;
            }
            else
            {
                // We are at 0 so reset reboot counter and go to quiesce state
                info("Auto reboot enabled but HOST BOOTCOUNT already set to 0");
                attemptsLeft(BOOT_COUNT_MAX_ALLOWED);
                return false;
            }
        }
        else
        {
            info("Auto reboot disabled.");
            return false;
        }
    }
    catch (const sdbusplus::exception::exception& e)
    {
        error("Error in AutoReboot Get, {ERROR}", "ERROR", e);
        return false;
    }
}

void Host::sysStateChangeJobRemoved(sdbusplus::message::message& msg)
{
    uint32_t newStateID{};
    sdbusplus::message::object_path newStateObjPath;
    std::string newStateUnit{};
    std::string newStateResult{};

    // Read the msg and populate each variable
    msg.read(newStateID, newStateObjPath, newStateUnit, newStateResult);

    if ((newStateUnit == HOST_STATE_POWEROFF_TGT) &&
        (newStateResult == "done") &&
        (!stateActive(HOST_STATE_POWERON_MIN_TGT)))
    {
        info("Received signal that host is off");
        this->currentHostState(server::Host::HostState::Off);
        this->bootProgress(bootprogress::Progress::ProgressStages::Unspecified);
        this->operatingSystemState(osstatus::Status::OSStatus::Inactive);
    }
    else if ((newStateUnit == HOST_STATE_POWERON_MIN_TGT) &&
             (newStateResult == "done") &&
             (stateActive(HOST_STATE_POWERON_MIN_TGT)))
    {
        info("Received signal that host is running");
        this->currentHostState(server::Host::HostState::Running);

        // Remove temporary file which is utilized for scenarios where the
        // BMC is rebooted while the host is still up.
        // This file is used to indicate to host related systemd services
        // that the host is already running and they should skip running.
        // Once the host state is back to running we can clear this file.
        auto size = std::snprintf(nullptr, 0, HOST_RUNNING_FILE, 0);
        size++; // null
        std::unique_ptr<char[]> hostFile(new char[size]);
        std::snprintf(hostFile.get(), size, HOST_RUNNING_FILE, 0);
        if (std::filesystem::exists(hostFile.get()))
        {
            std::filesystem::remove(hostFile.get());
        }
    }
    else if ((newStateUnit == HOST_STATE_QUIESCE_TGT) &&
             (newStateResult == "done") &&
             (stateActive(HOST_STATE_QUIESCE_TGT)))
    {
        if (Host::isAutoReboot())
        {
            info("Beginning reboot...");
            Host::requestedHostTransition(server::Host::Transition::Reboot);
        }
        else
        {
            info("Maintaining quiesce");
            this->currentHostState(server::Host::HostState::Quiesced);
        }
    }
}

void Host::sysStateChangeJobNew(sdbusplus::message::message& msg)
{
    uint32_t newStateID{};
    sdbusplus::message::object_path newStateObjPath;
    std::string newStateUnit{};

    // Read the msg and populate each variable
    msg.read(newStateID, newStateObjPath, newStateUnit);

    if (newStateUnit == HOST_STATE_DIAGNOSTIC_MODE)
    {
        info("Received signal that host is in diagnostice mode");
        this->currentHostState(server::Host::HostState::DiagnosticMode);
    }
}

uint32_t Host::decrementRebootCount()
{
    auto rebootCount = reboot::RebootAttempts::attemptsLeft();
    if (rebootCount > 0)
    {
        return (reboot::RebootAttempts::attemptsLeft(rebootCount - 1));
    }
    return rebootCount;
}

fs::path Host::serialize(const fs::path& dir)
{
    std::ofstream os(dir.c_str(), std::ios::binary);
    cereal::JSONOutputArchive oarchive(os);
    oarchive(*this);
    return dir;
}

bool Host::deserialize(const fs::path& path)
{
    try
    {
        if (fs::exists(path))
        {
            std::ifstream is(path.c_str(), std::ios::in | std::ios::binary);
            cereal::JSONInputArchive iarchive(is);
            iarchive(*this);
            return true;
        }
        return false;
    }
    catch (const cereal::Exception& e)
    {
        error("deserialize exception: {ERROR}", "ERROR", e);
        fs::remove(path);
        return false;
    }
}

Host::Transition Host::requestedHostTransition(Transition value)
{
    info("Host state transition request of {REQ}", "REQ", value);
    // If this is not a power off request then we need to
    // decrement the reboot counter.  This code should
    // never prevent a power on, it should just decrement
    // the count to 0.  The quiesce handling is where the
    // check of this count will occur
    if (value != server::Host::Transition::Off)
    {
        decrementRebootCount();
    }

    executeTransition(value);

    auto retVal = server::Host::requestedHostTransition(value);
    serialize();
    return retVal;
}

Host::ProgressStages Host::bootProgress(ProgressStages value)
{
    auto retVal = bootprogress::Progress::bootProgress(value);
    serialize();
    return retVal;
}

Host::OSStatus Host::operatingSystemState(OSStatus value)
{
    auto retVal = osstatus::Status::operatingSystemState(value);
    serialize();
    return retVal;
}

Host::HostState Host::currentHostState(HostState value)
{
    info("Change to Host State: {STATE}", "STATE", value);
    return server::Host::currentHostState(value);
}

} // namespace manager
} // namespace state
} // namespace phosphor
