#include "drive_state_manager.hpp"

#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/exception.hpp>
#include <xyz/openbmc_project/Common/error.hpp>

#include <format>
#include <string>
#include <tuple>
#include <variant>

namespace phosphor
{
namespace state
{
namespace manager
{

PHOSPHOR_LOG2_USING;

namespace server = sdbusplus::server::xyz::openbmc_project::state;

constexpr auto DRIVE_STATE_POWERON_TGT_FMT = "obmc-drive-poweron@{}.target";
constexpr auto DRIVE_STATE_POWEROFF_TGT_FMT = "obmc-drive-poweroff@{}.target";
constexpr auto DRIVE_STATE_REBOOT_TGT_FMT = "obmc-drive-reboot@{}.target";
constexpr auto DRIVE_STATE_HARD_REBOOT_TGT_FMT =
    "obmc-drive-hard-reboot@{}.target";
constexpr auto DRIVE_STATE_POWERCYCLE_TGT_FMT =
    "obmc-drive-powercycle@{}.target";
constexpr auto DRIVE_STATE_POWERED_ON_TGT_FMT =
    "obmc-drive-powered-on@{}.target";
constexpr auto DRIVE_STATE_POWERED_OFF_TGT_FMT =
    "obmc-drive-powered-off@{}.target";

constexpr auto ACTIVE_STATE = "active";
constexpr auto ACTIVATING_STATE = "activating";

void Drive::createSystemdTargetTable()
{
    systemdTargetTable = {
        {Transition::Reboot,
         std::format(DRIVE_STATE_REBOOT_TGT_FMT, instanceName)},
        {Transition::HardReboot,
         std::format(DRIVE_STATE_HARD_REBOOT_TGT_FMT, instanceName)},
        {Transition::Powercycle,
         std::format(DRIVE_STATE_POWERCYCLE_TGT_FMT, instanceName)}};
}

void Drive::determineInitialState()
{
    if (stateActive(std::format(DRIVE_STATE_POWERED_OFF_TGT_FMT, instanceName)))
    {
        server::Drive::currentDriveState(DriveState::Offline, true);
    }
    else if (stateActive(
                 std::format(DRIVE_STATE_POWERED_ON_TGT_FMT, instanceName)))
    {
        server::Drive::currentDriveState(DriveState::Ready, true);
    }
    else
    {
        server::Drive::currentDriveState(DriveState::Unknown, true);
    }

    server::Drive::requestedDriveTransition(Transition::None, true);
}

void Drive::startUnit(const std::string& sysdUnit)
{
    auto method = bus.new_method_call(SYSTEMD_SERVICE, SYSTEMD_OBJ_PATH,
                                      SYSTEMD_MANAGER_INTERFACE, "StartUnit");

    method.append(sysdUnit);
    method.append("replace");

    bus.call_noreply(method);
}

bool Drive::stateActive(const std::string& target)
{
    sdbusplus::message::object_path unitTargetPath;

    auto method = bus.new_method_call(SYSTEMD_SERVICE, SYSTEMD_OBJ_PATH,
                                      SYSTEMD_MANAGER_INTERFACE, "GetUnit");
    method.append(target);

    try
    {
        auto result = bus.call(method);
        unitTargetPath = result.unpack<sdbusplus::message::object_path>();
    }
    catch (const sdbusplus::exception_t& e)
    {
        error("Error in GetUnit call: {ERROR}", "ERROR", e);
        return false;
    }

    method = bus.new_method_call(
        SYSTEMD_SERVICE,
        static_cast<const std::string&>(unitTargetPath).c_str(),
        PROPERTY_INTERFACE, "Get");
    method.append(SYSTEMD_UNIT_INTERFACE, "ActiveState");

    std::string currentStateStr;
    try
    {
        auto result = bus.call(method);
        currentStateStr =
            std::get<std::string>(result.unpack<std::variant<std::string>>());
    }
    catch (const sdbusplus::exception_t& e)
    {
        error("Error in ActiveState Get: {ERROR}", "ERROR", e);
        return false;
    }

    return currentStateStr == ACTIVE_STATE ||
           currentStateStr == ACTIVATING_STATE;
}

void Drive::executeTransition(Transition value)
{
    auto it = systemdTargetTable.find(value);
    if (it == systemdTargetTable.end())
    {
        info("Unsupported drive transition requested: {TRANSITION}",
             "TRANSITION", value);
        throw sdbusplus::xyz::openbmc_project::Common::Error::NotAllowed();
    }

    startUnit(it->second);
}

void Drive::sysStateChangeJobRemoved(sdbusplus::message_t& msg)
{
    uint32_t newStateID{};
    sdbusplus::message::object_path newStateObjPath;
    std::string newStateUnit{};
    std::string newStateResult{};

    try
    {
        std::tie(newStateID, newStateObjPath, newStateUnit, newStateResult) =
            msg.unpack<uint32_t, sdbusplus::message::object_path, std::string,
                       std::string>();
    }
    catch (const sdbusplus::exception_t& e)
    {
        error("Error in drive state change - bad encoding: "
              "{ERROR} {REPLY_SIG}",
              "ERROR", e, "REPLY_SIG", msg.get_signature());
        return;
    }

    if ((newStateUnit ==
         std::format(DRIVE_STATE_POWERON_TGT_FMT, instanceName)) &&
        newStateResult == "done")
    {
        currentDriveState(DriveState::Ready);
        server::Drive::requestedDriveTransition(Transition::None, true);
        return;
    }

    if ((newStateUnit ==
         std::format(DRIVE_STATE_POWEROFF_TGT_FMT, instanceName)) &&
        newStateResult == "done")
    {
        currentDriveState(DriveState::Offline);
        server::Drive::requestedDriveTransition(Transition::None, true);
        return;
    }
}

Drive::Transition Drive::requestedDriveTransition(Transition value)
{
    if (value == Transition::None)
    {
        return server::Drive::requestedDriveTransition(value);
    }

    if (value == Transition::NotSupported)
    {
        throw sdbusplus::xyz::openbmc_project::Common::Error::NotAllowed();
    }

    executeTransition(value);
    currentDriveState(DriveState::NotReady);
    return server::Drive::requestedDriveTransition(value);
}

Drive::DriveState Drive::currentDriveState(DriveState value)
{
    info("Change to Drive {DRIVE_INSTANCE} State: {STATE}", "DRIVE_INSTANCE",
         instanceName, "STATE", value);
    return server::Drive::currentDriveState(value);
}

} // namespace manager
} // namespace state
} // namespace phosphor
