#include <sdbusplus/bus.hpp>
#include <sdbusplus/exception.hpp>
#include <phosphor-logging/log.hpp>
#include <phosphor-logging/elog-errors.hpp>
#include "xyz/openbmc_project/Common/error.hpp"
#include "chassis_state_manager.hpp"
#include <cereal/archives/json.hpp>
#include <fstream>
#include "config.h"
#include <experimental/filesystem>

namespace phosphor
{
namespace state
{
namespace manager
{

// When you see server:: you know we're referencing our base class
namespace server = sdbusplus::xyz::openbmc_project::State::server;

using namespace phosphor::logging;
using sdbusplus::exception::SdBusError;

constexpr auto CHASSIS_STATE_POWEROFF_TGT = "obmc-chassis-poweroff@0.target";
constexpr auto CHASSIS_STATE_HARD_POWEROFF_TGT =
    "obmc-chassis-hard-poweroff@0.target";
constexpr auto CHASSIS_STATE_POWERON_TGT = "obmc-chassis-poweron@0.target";

constexpr auto ACTIVE_STATE = "active";
constexpr auto ACTIVATING_STATE = "activating";

/* Map a transition to it's systemd target */
const std::map<server::Chassis::Transition, std::string> SYSTEMD_TARGET_TABLE =
    {
        // Use the hard off target to ensure we shutdown immediately
        {server::Chassis::Transition::Off, CHASSIS_STATE_HARD_POWEROFF_TGT},
        {server::Chassis::Transition::On, CHASSIS_STATE_POWERON_TGT}};

constexpr auto SYSTEMD_SERVICE = "org.freedesktop.systemd1";
constexpr auto SYSTEMD_OBJ_PATH = "/org/freedesktop/systemd1";
constexpr auto SYSTEMD_INTERFACE = "org.freedesktop.systemd1.Manager";

constexpr auto SYSTEMD_PROPERTY_IFACE = "org.freedesktop.DBus.Properties";
constexpr auto SYSTEMD_INTERFACE_UNIT = "org.freedesktop.systemd1.Unit";

void Chassis::subscribeToSystemdSignals()
{
    auto method = this->bus.new_method_call(SYSTEMD_SERVICE, SYSTEMD_OBJ_PATH,
                                            SYSTEMD_INTERFACE, "Subscribe");
    this->bus.call_noreply(method);

    return;
}

// TODO - Will be rewritten once sdbusplus client bindings are in place
//        and persistent storage design is in place and sdbusplus
//        has read property function
void Chassis::determineInitialState()
{
    sdbusplus::message::variant<int> pgood = -1;
    auto method = this->bus.new_method_call(
        "org.openbmc.control.Power", "/org/openbmc/control/power0",
        "org.freedesktop.DBus.Properties", "Get");

    method.append("org.openbmc.control.Power", "pgood");
    try
    {
        auto reply = this->bus.call(method);
        if (reply.is_method_error())
        {
            log<level::ERR>(
                "Error in response message - could not get initial pgood");
            goto fail;
        }

        try
        {
            reply.read(pgood);
        }
        catch (const SdBusError& e)
        {
            log<level::ERR>("Error in bus response - bad encoding of pgood",
                            entry("ERROR=%s", e.what()),
                            entry("REPLY_SIG=%s", reply.get_signature()));
            goto fail;
        }

        if (pgood == 1)
        {
            log<level::INFO>("Initial Chassis State will be On",
                             entry("CHASSIS_CURRENT_POWER_STATE=%s",
                                   convertForMessage(PowerState::On).c_str()));
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
                if (lastState == PowerState::On)
                {
                    setStateChangeTime();
                }
            }
        }
    }
    catch (const SdBusError& e)
    {
        // It's acceptable for the pgood state service to not be available
        // since it will notify us of the pgood state when it comes up.
        if (e.name() != nullptr &&
            strcmp("org.freedesktop.DBus.Error.ServiceUnknown", e.name()) == 0)
        {
            goto fail;
        }

        // Only log for unexpected error types.
        log<level::ERR>("Error performing call to get pgood",
                        entry("ERROR=%s", e.what()));
        goto fail;
    }

fail:
    log<level::INFO>("Initial Chassis State will be Off",
                     entry("CHASSIS_CURRENT_POWER_STATE=%s",
                           convertForMessage(PowerState::Off).c_str()));
    server::Chassis::currentPowerState(PowerState::Off);
    server::Chassis::requestedPowerTransition(Transition::Off);

    return;
}

void Chassis::executeTransition(Transition tranReq)
{
    auto sysdTarget = SYSTEMD_TARGET_TABLE.find(tranReq)->second;

    auto method = this->bus.new_method_call(SYSTEMD_SERVICE, SYSTEMD_OBJ_PATH,
                                            SYSTEMD_INTERFACE, "StartUnit");

    method.append(sysdTarget);
    method.append("replace");

    this->bus.call_noreply(method);

    return;
}

bool Chassis::stateActive(const std::string& target)
{
    sdbusplus::message::variant<std::string> currentState;
    sdbusplus::message::object_path unitTargetPath;

    auto method = this->bus.new_method_call(SYSTEMD_SERVICE, SYSTEMD_OBJ_PATH,
                                            SYSTEMD_INTERFACE, "GetUnit");

    method.append(target);
    auto result = this->bus.call(method);

    // Check that the bus call didn't result in an error
    if (result.is_method_error())
    {
        log<level::ERR>("Error in bus call - could not resolve GetUnit for:",
                        entry(" %s", SYSTEMD_INTERFACE));
        return false;
    }

    try
    {
        result.read(unitTargetPath);
    }
    catch (const SdBusError& e)
    {
        log<level::ERR>("Error in bus response - bad encoding for GetUnit",
                        entry("ERROR=%s", e.what()),
                        entry("REPLY_SIG=%s", result.get_signature()));
        return false;
    }

    method = this->bus.new_method_call(
        SYSTEMD_SERVICE,
        static_cast<const std::string&>(unitTargetPath).c_str(),
        SYSTEMD_PROPERTY_IFACE, "Get");

    method.append(SYSTEMD_INTERFACE_UNIT, "ActiveState");
    result = this->bus.call(method);

    // Check that the bus call didn't result in an error
    if (result.is_method_error())
    {
        log<level::ERR>("Error in bus call - could not resolve Get for:",
                        entry(" %s", SYSTEMD_PROPERTY_IFACE));
        return false;
    }

    result.read(currentState);

    if (currentState != ACTIVE_STATE && currentState != ACTIVATING_STATE)
    {
        // False - not active
        return false;
    }
    // True - active
    return true;
}

int Chassis::sysStateChange(sdbusplus::message::message& msg)
{
    uint32_t newStateID{};
    sdbusplus::message::object_path newStateObjPath;
    std::string newStateUnit{};
    std::string newStateResult{};

    // Read the msg and populate each variable
    try
    {
        msg.read(newStateID, newStateObjPath, newStateUnit, newStateResult);
    }
    catch (const SdBusError& e)
    {
        log<level::ERR>("Error in state change - bad encoding",
                        entry("ERROR=%s", e.what()),
                        entry("REPLY_SIG=%s", msg.get_signature()));
        return 0;
    }

    if ((newStateUnit == CHASSIS_STATE_POWEROFF_TGT) &&
        (newStateResult == "done") && (!stateActive(CHASSIS_STATE_POWERON_TGT)))
    {
        log<level::INFO>("Received signal that power OFF is complete");
        this->currentPowerState(server::Chassis::PowerState::Off);
        this->setStateChangeTime();
    }
    else if ((newStateUnit == CHASSIS_STATE_POWERON_TGT) &&
             (newStateResult == "done") &&
             (stateActive(CHASSIS_STATE_POWERON_TGT)))
    {
        log<level::INFO>("Received signal that power ON is complete");
        this->currentPowerState(server::Chassis::PowerState::On);
        this->setStateChangeTime();
    }

    return 0;
}

Chassis::Transition Chassis::requestedPowerTransition(Transition value)
{

    log<level::INFO>("Change to Chassis Requested Power State",
                     entry("CHASSIS_REQUESTED_POWER_STATE=%s",
                           convertForMessage(value).c_str()));
    executeTransition(value);
    return server::Chassis::requestedPowerTransition(value);
}

Chassis::PowerState Chassis::currentPowerState(PowerState value)
{
    PowerState chassisPowerState;
    log<level::INFO>("Change to Chassis Power State",
                     entry("CHASSIS_CURRENT_POWER_STATE=%s",
                           convertForMessage(value).c_str()));

    chassisPowerState = server::Chassis::currentPowerState(value);
    if (chassisPowerState == PowerState::On)
    {
        timer->state(timer::ON);
    }
    else
    {
        timer->state(timer::OFF);
    }
    return chassisPowerState;
}

uint32_t Chassis::pOHCounter(uint32_t value)
{
    if (value != pOHCounter())
    {
        ChassisInherit::pOHCounter(value);
        serializePOH();
    }
    return pOHCounter();
}

void Chassis::restorePOHCounter()
{
    uint32_t counter;
    if (!deserializePOH(POH_COUNTER_PERSIST_PATH, counter))
    {
        // set to default value
        pOHCounter(0);
    }
    else
    {
        pOHCounter(counter);
    }
}

fs::path Chassis::serializePOH(const fs::path& path)
{
    std::ofstream os(path.c_str(), std::ios::binary);
    cereal::JSONOutputArchive oarchive(os);
    oarchive(pOHCounter());
    return path;
}

bool Chassis::deserializePOH(const fs::path& path, uint32_t& pOHCounter)
{
    try
    {
        if (fs::exists(path))
        {
            std::ifstream is(path.c_str(), std::ios::in | std::ios::binary);
            cereal::JSONInputArchive iarchive(is);
            iarchive(pOHCounter);
            return true;
        }
        return false;
    }
    catch (cereal::Exception& e)
    {
        log<level::ERR>(e.what());
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
    using namespace std::chrono_literals;
    using namespace phosphor::logging;
    using namespace sdbusplus::xyz::openbmc_project::Common::Error;

    auto dir = fs::path(POH_COUNTER_PERSIST_PATH).parent_path();
    fs::create_directories(dir);

    sd_event* event = nullptr;
    auto r = sd_event_default(&event);
    if (r < 0)
    {
        log<level::ERR>("Error creating a default sd_event handler");
        throw;
    }

    phosphor::state::manager::EventPtr eventP{event};
    event = nullptr;

    auto callback = [&]() {
        if (ChassisInherit::currentPowerState() == PowerState::On)
        {
            pOHCounter(pOHCounter() + 1);
        }
    };

    try
    {
        timer = std::make_unique<phosphor::state::manager::Timer>(
            eventP, callback, std::chrono::seconds(POH::hour),
            phosphor::state::manager::timer::ON);
        bus.attach_event(eventP.get(), SD_EVENT_PRIORITY_NORMAL);
        r = sd_event_loop(eventP.get());
        if (r < 0)
        {
            log<level::ERR>("Error occurred during the sd_event_loop",
                            entry("RC=%d", r));
            elog<InternalFailure>();
        }
    }
    catch (InternalFailure& e)
    {
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
    catch (std::exception& e)
    {
        log<level::ERR>(e.what());
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

} // namespace manager
} // namespace state
} // namepsace phosphor
