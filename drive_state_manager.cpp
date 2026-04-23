#include "drive_state_manager.hpp"

#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/exception.hpp>
#include <xyz/openbmc_project/Common/error.hpp>

#include <format>
#include <string>
#include <tuple>
#include <variant>
#include <vector>

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

constexpr auto HOST_STATE_INTERFACE = "xyz.openbmc_project.State.Host";
constexpr auto HOST_STATE_PATH_FMT = "/xyz/openbmc_project/state/host{}";
constexpr auto HOST_STATE_BUSNAME_FMT = "xyz.openbmc_project.State.Host{}";

using AssocDefTuple = std::tuple<std::string, std::string, std::string>;

Drive::Drive(sdbusplus::bus_t& bus, const char* objPath,
             const std::string& instanceName, const std::string& inventoryPath,
             std::optional<size_t> hostId) :
    DriveInherit(bus, objPath, DriveInherit::action::defer_emit), bus(bus),
    systemdSignals(
        bus,
        sdbusRule::type::signal() + sdbusRule::member("JobRemoved") +
            sdbusRule::path(SYSTEMD_OBJ_PATH) +
            sdbusRule::interface(SYSTEMD_MANAGER_INTERFACE),
        [this](sdbusplus::message_t& m) { sysStateChangeJobRemoved(m); }),
    instanceName(instanceName), inventoryPath(inventoryPath), hostId(hostId)
{
    utils::subscribeToSystemdSignals(bus);
    createSystemdTargetTable();

    // Publish association to inventory object
    std::vector<AssocDefTuple> assocDefs;
    assocDefs.emplace_back("inventory", "state", inventoryPath);
    associations(assocDefs);

    setupHostStateMonitoring();
    determineInitialState();
    this->emit_object_added();
}

void Drive::createSystemdTargetTable()
{
    systemdTargetTable = {
        {Transition::On,
         std::format(DRIVE_STATE_POWERON_TGT_FMT, instanceName)},
        {Transition::Off,
         std::format(DRIVE_STATE_POWEROFF_TGT_FMT, instanceName)},
        {Transition::Reboot,
         std::format(DRIVE_STATE_REBOOT_TGT_FMT, instanceName)},
        {Transition::HardReboot,
         std::format(DRIVE_STATE_HARD_REBOOT_TGT_FMT, instanceName)},
        {Transition::Powercycle,
         std::format(DRIVE_STATE_POWERCYCLE_TGT_FMT, instanceName)}};
}

void Drive::setupHostStateMonitoring()
{
    if (!hostId.has_value())
    {
        // No host association; drive is host-independent
        hostRunning = true;
        return;
    }

    auto hostPath = std::format(HOST_STATE_PATH_FMT, *hostId);

    info("Drive {DRIVE} associated with host {HOSTID}, monitoring {PATH}",
         "DRIVE", instanceName, "HOSTID", *hostId, "PATH", hostPath);

    // Watch for host state property changes
    hostStateMatch = std::make_unique<sdbusplus::bus::match_t>(
        bus, sdbusRule::propertiesChanged(hostPath, HOST_STATE_INTERFACE),
        [this](sdbusplus::message_t& msg) {
            std::string interface;
            std::map<std::string, std::variant<std::string>> properties;
            msg.read(interface, properties);

            auto findState = properties.find("CurrentHostState");
            if (findState == properties.end())
            {
                return;
            }

            const auto& stateStr = std::get<std::string>(findState->second);
            hostRunning = stateStr.ends_with(".Running");

            info("Host {HOSTID} state changed, hostRunning={RUNNING}", "HOSTID",
                 *hostId, "RUNNING", hostRunning);
        });

    // Watch for host state manager restart (NameOwnerChanged)
    auto hostBusName = std::format(HOST_STATE_BUSNAME_FMT, *hostId);
    hostOwnerMatch = std::make_unique<sdbusplus::bus::match_t>(
        bus, sdbusRule::nameOwnerChanged() + sdbusRule::argN(0, hostBusName),
        [this](sdbusplus::message_t& msg) {
            std::string name;
            std::string oldOwner;
            std::string newOwner;
            msg.read(name, oldOwner, newOwner);

            if (!newOwner.empty())
            {
                // Host state manager restarted; re-read state
                info("Host state manager reappeared, re-reading state");
                readHostState();
            }
            else
            {
                // Host state manager gone; assume host is off for safety
                hostRunning = false;
            }
        });

    // Initial read of current host state
    readHostState();
}

void Drive::readHostState()
{
    if (!hostId.has_value())
    {
        return;
    }

    auto hostPath = std::format(HOST_STATE_PATH_FMT, *hostId);

    try
    {
        auto currentState = utils::getProperty(
            bus, hostPath, HOST_STATE_INTERFACE, "CurrentHostState");
        hostRunning = currentState.ends_with(".Running");
    }
    catch (const sdbusplus::exception_t& e)
    {
        // Host state manager may not be available yet; default to off
        info("Unable to read host state: {ERROR}", "ERROR", e);
        hostRunning = false;
    }
}

bool Drive::isHostRunning() const
{
    if (!hostId.has_value())
    {
        // No host association means host-independent
        return true;
    }
    return hostRunning;
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
        // No marker target is active. This is expected on first boot or
        // after a BMC reset when no drive transition has been triggered yet.
        // The state will update to Ready or Offline when the platform
        // starts the poweron/poweroff targets.
        info("Drive {DRIVE} has no active marker target, state is Unknown",
             "DRIVE", instanceName);
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

    // Host dependency check: reject transitions when host is off
    if (!isHostRunning())
    {
        error(
            "Drive {DRIVE} transition {TRANSITION} rejected: host {HOSTID} is off",
            "DRIVE", instanceName, "TRANSITION", value, "HOSTID",
            hostId.value_or(0));
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
