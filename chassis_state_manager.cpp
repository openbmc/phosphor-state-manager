#include "config.h"

#include "chassis_state_manager.hpp"

#include "utils.hpp"
#include "xyz/openbmc_project/Common/error.hpp"
#include "xyz/openbmc_project/State/Shutdown/Power/error.hpp"

#include <fmt/format.h>

#include <cereal/archives/json.hpp>
#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/exception.hpp>
#include <sdeventplus/event.hpp>
#include <sdeventplus/exception.hpp>

#include <filesystem>
#include <fstream>

namespace phosphor
{
namespace state
{
namespace manager
{

PHOSPHOR_LOG2_USING;

// When you see server:: you know we're referencing our base class
namespace server = sdbusplus::xyz::openbmc_project::State::server;

using namespace phosphor::logging;
using sdbusplus::xyz::openbmc_project::Common::Error::InternalFailure;
using sdbusplus::xyz::openbmc_project::State::Shutdown::Power::Error::Blackout;
using sdbusplus::xyz::openbmc_project::State::Shutdown::Power::Error::Regulator;
constexpr auto CHASSIS_STATE_POWEROFF_TGT_FMT =
    "obmc-chassis-poweroff@{}.target";
constexpr auto CHASSIS_STATE_HARD_POWEROFF_TGT_FMT =
    "obmc-chassis-hard-poweroff@{}.target";
constexpr auto CHASSIS_STATE_POWERON_TGT_FMT = "obmc-chassis-poweron@{}.target";
constexpr auto RESET_HOST_SENSORS_SVC_FMT =
    "phosphor-reset-sensor-states@{}.service";
constexpr auto ACTIVE_STATE = "active";
constexpr auto ACTIVATING_STATE = "activating";

// Details at https://upower.freedesktop.org/docs/Device.html
constexpr uint TYPE_UPS = 3;
constexpr uint STATE_FULLY_CHARGED = 4;
constexpr uint BATTERY_LVL_FULL = 8;

constexpr auto SYSTEMD_SERVICE = "org.freedesktop.systemd1";
constexpr auto SYSTEMD_OBJ_PATH = "/org/freedesktop/systemd1";
constexpr auto SYSTEMD_INTERFACE = "org.freedesktop.systemd1.Manager";

constexpr auto SYSTEMD_PROPERTY_IFACE = "org.freedesktop.DBus.Properties";
constexpr auto SYSTEMD_INTERFACE_UNIT = "org.freedesktop.systemd1.Unit";

constexpr auto MAPPER_BUSNAME = "xyz.openbmc_project.ObjectMapper";
constexpr auto MAPPER_PATH = "/xyz/openbmc_project/object_mapper";
constexpr auto MAPPER_INTERFACE = "xyz.openbmc_project.ObjectMapper";
constexpr auto UPOWER_INTERFACE = "org.freedesktop.UPower.Device";
constexpr auto PROPERTY_INTERFACE = "org.freedesktop.DBus.Properties";

void Chassis::subscribeToSystemdSignals()
{
    try
    {
        auto method = this->bus.new_method_call(
            SYSTEMD_SERVICE, SYSTEMD_OBJ_PATH, SYSTEMD_INTERFACE, "Subscribe");
        this->bus.call(method);
    }
    catch (const sdbusplus::exception::exception& e)
    {
        error("Failed to subscribe to systemd signals: {ERROR}", "ERROR", e);
        elog<InternalFailure>();
    }

    return;
}

void Chassis::createSystemdTargetTable()
{
    systemdTargetTable = {
        // Use the hard off target to ensure we shutdown immediately
        {Transition::Off, fmt::format(CHASSIS_STATE_HARD_POWEROFF_TGT_FMT, id)},
        {Transition::On, fmt::format(CHASSIS_STATE_POWERON_TGT_FMT, id)}};
}

// TODO - Will be rewritten once sdbusplus client bindings are in place
//        and persistent storage design is in place and sdbusplus
//        has read property function
void Chassis::determineInitialState()
{

    // Monitor for any properties changed signals on UPower device path
    uPowerPropChangeSignal = std::make_unique<sdbusplus::bus::match_t>(
        bus,
        sdbusplus::bus::match::rules::propertiesChangedNamespace(
            "/org/freedesktop/UPower", UPOWER_INTERFACE),
        [this](auto& msg) { this->uPowerChangeEvent(msg); });

    determineStatusOfPower();

    std::variant<int> pgood = -1;
    auto method = this->bus.new_method_call(
        "org.openbmc.control.Power", "/org/openbmc/control/power0",
        "org.freedesktop.DBus.Properties", "Get");

    method.append("org.openbmc.control.Power", "pgood");
    try
    {
        auto reply = this->bus.call(method);
        reply.read(pgood);

        if (std::get<int>(pgood) == 1)
        {
            info("Initial Chassis State will be On");
            server::Chassis::currentPowerState(PowerState::On);
            server::Chassis::requestedPowerTransition(Transition::On);
            return;
        }
        else
        {
            // The system is off.  If we think it should be on then
            // we probably lost AC while up, so set a new state
            // change time.
            uint64_t lastTime;
            PowerState lastState;

            if (deserializeStateChangeTime(lastTime, lastState))
            {
                // If power was on before the BMC reboot and the reboot reason
                // was not a pinhole reset, log an error
                if (lastState == PowerState::On)
                {
                    info(
                        "Chassis power was on before the BMC reboot and it is off now");

                    // Reset host sensors since system is off now
                    startUnit(fmt::format(RESET_HOST_SENSORS_SVC_FMT, id));

                    setStateChangeTime();

                    // 0 indicates pinhole reset. 1 is NOT pinhole reset
                    if (phosphor::state::manager::utils::getGpioValue(
                            "reset-cause-pinhole") != 0)
                    {
                        if (standbyVoltageRegulatorFault())
                        {
                            report<Regulator>();
                        }
                        else
                        {
                            report<Blackout>(Entry::Level::Critical);
                        }
                    }
                    else
                    {
                        info("Pinhole reset");
                    }
                }
            }
        }
    }
    catch (const sdbusplus::exception::exception& e)
    {
        // It's acceptable for the pgood state service to not be available
        // since it will notify us of the pgood state when it comes up.
        if (e.name() != nullptr &&
            strcmp("org.freedesktop.DBus.Error.ServiceUnknown", e.name()) == 0)
        {
            goto fail;
        }

        // Only log for unexpected error types.
        error("Error performing call to get pgood: {ERROR}", "ERROR", e);
        goto fail;
    }

fail:
    info("Initial Chassis State will be Off");
    server::Chassis::currentPowerState(PowerState::Off);
    server::Chassis::requestedPowerTransition(Transition::Off);

    return;
}

void Chassis::determineStatusOfPower()
{
    // Default PowerStatus to good
    server::Chassis::currentPowerStatus(PowerStatus::Good);

    // Find all implementations of the UPower interface
    auto mapper = bus.new_method_call(MAPPER_BUSNAME, MAPPER_PATH,
                                      MAPPER_INTERFACE, "GetSubTree");

    mapper.append("/", 0, std::vector<std::string>({UPOWER_INTERFACE}));

    std::map<std::string, std::map<std::string, std::vector<std::string>>>
        mapperResponse;

    try
    {
        auto mapperResponseMsg = bus.call(mapper);
        mapperResponseMsg.read(mapperResponse);
    }
    catch (const sdbusplus::exception::exception& e)
    {
        error("Error in mapper GetSubTree call for UPS: {ERROR}", "ERROR", e);
        throw;
    }

    if (mapperResponse.empty())
    {
        debug("No UPower devices found in system");
    }

    // Iterate through all returned Upower interfaces and look for UPS's
    for (const auto& [path, services] : mapperResponse)
    {
        for (const auto& serviceIter : services)
        {
            const std::string& service = serviceIter.first;

            try
            {
                auto method = bus.new_method_call(service.c_str(), path.c_str(),
                                                  PROPERTY_INTERFACE, "GetAll");
                method.append(UPOWER_INTERFACE);

                auto response = bus.call(method);
                using Property = std::string;
                using Value = std::variant<bool, uint>;
                using PropertyMap = std::map<Property, Value>;
                PropertyMap properties;
                response.read(properties);

                if (std::get<uint>(properties["Type"]) != TYPE_UPS)
                {
                    info("UPower device {OBJ_PATH} is not a UPS device",
                         "OBJ_PATH", path);
                    continue;
                }

                if (std::get<bool>(properties["IsPresent"]) != true)
                {
                    // There is a UPS detected but it is not officially
                    // "present" yet. Monitor it for state change.
                    info("UPower device {OBJ_PATH} is not present", "OBJ_PATH",
                         path);
                    continue;
                }

                if (std::get<uint>(properties["State"]) == STATE_FULLY_CHARGED)
                {
                    info("UPS is fully charged");
                }
                else
                {
                    info("UPS is not fully charged: {UPS_STATE}", "UPS_STATE",
                         std::get<uint>(properties["State"]));
                    server::Chassis::currentPowerStatus(
                        PowerStatus::UninterruptiblePowerSupply);
                    return;
                }

                if (std::get<uint>(properties["BatteryLevel"]) ==
                    BATTERY_LVL_FULL)
                {
                    info("UPS Battery Level is Full");
                    // Only one UPS per system, we've found it and it's all
                    // good so exit function
                    return;
                }
                else
                {
                    info("UPS Battery Level is Low: {UPS_BAT_LEVEL}",
                         "UPS_BAT_LEVEL",
                         std::get<uint>(properties["BatteryLevel"]));
                    server::Chassis::currentPowerStatus(
                        PowerStatus::UninterruptiblePowerSupply);
                    return;
                }
            }
            catch (const sdbusplus::exception::exception& e)
            {
                error("Error reading UPS property, error: {ERROR}, "
                      "service: {SERVICE} path: {PATH}",
                      "ERROR", e, "SERVICE", service, "PATH", path);
                throw;
            }
        }
    }
    return;
}

void Chassis::uPowerChangeEvent(sdbusplus::message::message& msg)
{
    debug("UPS Property Change Event Triggered");
    std::string statusInterface;
    std::map<std::string, std::variant<uint, bool>> msgData;
    msg.read(statusInterface, msgData);

    // If the change is to any of the three properties we are interested in
    // then call determineStatusOfPower() to see if a power status change
    // is needed
    auto propertyMap = msgData.find("IsPresent");
    if (propertyMap != msgData.end())
    {
        info("UPS presence changed to {UPS_PRES_INFO}", "UPS_PRES_INFO",
             std::get<bool>(propertyMap->second));
        determineStatusOfPower();
        return;
    }

    propertyMap = msgData.find("State");
    if (propertyMap != msgData.end())
    {
        info("UPS State changed to {UPS_STATE}", "UPS_STATE",
             std::get<uint>(propertyMap->second));
        determineStatusOfPower();
        return;
    }

    propertyMap = msgData.find("BatteryLevel");
    if (propertyMap != msgData.end())
    {
        info("UPS BatteryLevel changed to {UPS_BAT_LEVEL}", "UPS_BAT_LEVEL",
             std::get<uint>(propertyMap->second));
        determineStatusOfPower();
        return;
    }
    return;
}

void Chassis::startUnit(const std::string& sysdUnit)
{
    auto method = this->bus.new_method_call(SYSTEMD_SERVICE, SYSTEMD_OBJ_PATH,
                                            SYSTEMD_INTERFACE, "StartUnit");

    method.append(sysdUnit);
    method.append("replace");

    this->bus.call_noreply(method);

    return;
}

bool Chassis::stateActive(const std::string& target)
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

int Chassis::sysStateChange(sdbusplus::message::message& msg)
{
    sdbusplus::message::object_path newStateObjPath;
    std::string newStateUnit{};
    std::string newStateResult{};

    // Read the msg and populate each variable
    try
    {
        // newStateID is a throwaway that is needed in order to read the
        // parameters that are useful out of the dbus message
        uint32_t newStateID{};
        msg.read(newStateID, newStateObjPath, newStateUnit, newStateResult);
    }
    catch (const sdbusplus::exception::exception& e)
    {
        error("Error in state change - bad encoding: {ERROR} {REPLY_SIG}",
              "ERROR", e, "REPLY_SIG", msg.get_signature());
        return 0;
    }

    if ((newStateUnit == fmt::format(CHASSIS_STATE_POWEROFF_TGT_FMT, id)) &&
        (newStateResult == "done") &&
        (!stateActive(systemdTargetTable[Transition::On])))
    {
        info("Received signal that power OFF is complete");
        this->currentPowerState(server::Chassis::PowerState::Off);
        this->setStateChangeTime();
    }
    else if ((newStateUnit == systemdTargetTable[Transition::On]) &&
             (newStateResult == "done") &&
             (stateActive(systemdTargetTable[Transition::On])))
    {
        info("Received signal that power ON is complete");
        this->currentPowerState(server::Chassis::PowerState::On);
        this->setStateChangeTime();

        // Remove temporary file which is utilized for scenarios where the
        // BMC is rebooted while the chassis power is still on.
        // This file is used to indicate to chassis related systemd services
        // that the chassis is already on and they should skip running.
        // Once the chassis state is back to on we can clear this file.
        auto size = std::snprintf(nullptr, 0, CHASSIS_ON_FILE, 0);
        size++; // null
        std::unique_ptr<char[]> chassisFile(new char[size]);
        std::snprintf(chassisFile.get(), size, CHASSIS_ON_FILE, 0);
        if (std::filesystem::exists(chassisFile.get()))
        {
            std::filesystem::remove(chassisFile.get());
        }
    }

    return 0;
}

Chassis::Transition Chassis::requestedPowerTransition(Transition value)
{

    info("Change to Chassis Requested Power State: {REQ_POWER_TRAN}",
         "REQ_POWER_TRAN", value);
    startUnit(systemdTargetTable.find(value)->second);
    return server::Chassis::requestedPowerTransition(value);
}

Chassis::PowerState Chassis::currentPowerState(PowerState value)
{
    PowerState chassisPowerState;
    info("Change to Chassis Power State: {CUR_POWER_STATE}", "CUR_POWER_STATE",
         value);

    chassisPowerState = server::Chassis::currentPowerState(value);
    if (chassisPowerState == PowerState::On)
    {
        pohTimer.resetRemaining();
    }
    return chassisPowerState;
}

uint32_t Chassis::pohCounter(uint32_t value)
{
    if (value != pohCounter())
    {
        ChassisInherit::pohCounter(value);
        serializePOH();
    }
    return pohCounter();
}

void Chassis::pohCallback()
{
    if (ChassisInherit::currentPowerState() == PowerState::On)
    {
        pohCounter(pohCounter() + 1);
    }
}

void Chassis::restorePOHCounter()
{
    uint32_t counter;
    if (!deserializePOH(POH_COUNTER_PERSIST_PATH, counter))
    {
        // set to default value
        pohCounter(0);
    }
    else
    {
        pohCounter(counter);
    }
}

fs::path Chassis::serializePOH(const fs::path& path)
{
    std::ofstream os(path.c_str(), std::ios::binary);
    cereal::JSONOutputArchive oarchive(os);
    oarchive(pohCounter());
    return path;
}

bool Chassis::deserializePOH(const fs::path& path, uint32_t& pohCounter)
{
    try
    {
        if (fs::exists(path))
        {
            std::ifstream is(path.c_str(), std::ios::in | std::ios::binary);
            cereal::JSONInputArchive iarchive(is);
            iarchive(pohCounter);
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
    catch (const fs::filesystem_error& e)
    {
        return false;
    }

    return false;
}

void Chassis::startPOHCounter()
{
    auto dir = fs::path(POH_COUNTER_PERSIST_PATH).parent_path();
    fs::create_directories(dir);

    try
    {
        auto event = sdeventplus::Event::get_default();
        bus.attach_event(event.get(), SD_EVENT_PRIORITY_NORMAL);
        event.loop();
    }
    catch (const sdeventplus::SdEventError& e)
    {
        error("Error occurred during the sdeventplus loop: {ERROR}", "ERROR",
              e);
        phosphor::logging::commit<InternalFailure>();
    }
}

void Chassis::serializeStateChangeTime()
{
    fs::path path{CHASSIS_STATE_CHANGE_PERSIST_PATH};
    std::ofstream os(path.c_str(), std::ios::binary);
    cereal::JSONOutputArchive oarchive(os);

    oarchive(ChassisInherit::lastStateChangeTime(),
             ChassisInherit::currentPowerState());
}

bool Chassis::deserializeStateChangeTime(uint64_t& time, PowerState& state)
{
    fs::path path{CHASSIS_STATE_CHANGE_PERSIST_PATH};

    try
    {
        if (fs::exists(path))
        {
            std::ifstream is(path.c_str(), std::ios::in | std::ios::binary);
            cereal::JSONInputArchive iarchive(is);
            iarchive(time, state);
            return true;
        }
    }
    catch (const std::exception& e)
    {
        error("deserialize exception: {ERROR}", "ERROR", e);
        fs::remove(path);
    }

    return false;
}

void Chassis::restoreChassisStateChangeTime()
{
    uint64_t time;
    PowerState state;

    if (!deserializeStateChangeTime(time, state))
    {
        ChassisInherit::lastStateChangeTime(0);
    }
    else
    {
        ChassisInherit::lastStateChangeTime(time);
    }
}

void Chassis::setStateChangeTime()
{
    using namespace std::chrono;
    uint64_t lastTime;
    PowerState lastState;

    auto now =
        duration_cast<milliseconds>(system_clock::now().time_since_epoch())
            .count();

    // If power is on when the BMC is rebooted, this function will get called
    // because sysStateChange() runs.  Since the power state didn't change
    // in this case, neither should the state change time, so check that
    // the power state actually did change here.
    if (deserializeStateChangeTime(lastTime, lastState))
    {
        if (lastState == ChassisInherit::currentPowerState())
        {
            return;
        }
    }

    ChassisInherit::lastStateChangeTime(now);
    serializeStateChangeTime();
}

bool Chassis::standbyVoltageRegulatorFault()
{
    bool regulatorFault = false;

    // find standby voltage regulator fault via gpiog

    auto gpioval = utils::getGpioValue("regulator-standby-faulted");

    if (-1 == gpioval)
    {
        error("Failed reading regulator-standby-faulted GPIO");
    }

    if (1 == gpioval)
    {
        info("Detected standby voltage regulator fault");
        regulatorFault = true;
    }

    return regulatorFault;
}

} // namespace manager
} // namespace state
} // namespace phosphor
