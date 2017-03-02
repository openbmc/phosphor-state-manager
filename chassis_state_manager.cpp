#include <sdbusplus/bus.hpp>
#include <phosphor-logging/log.hpp>
#include "chassis_state_manager.hpp"

namespace phosphor
{
namespace state
{
namespace manager
{

// When you see server:: you know we're referencing our base class
namespace server = sdbusplus::xyz::openbmc_project::State::server;

using namespace phosphor::logging;

constexpr auto CHASSIS_STATE_POWEROFF_TGT = "obmc-power-chassis-off@0.target";
constexpr auto CHASSIS_STATE_POWERON_TGT = "obmc-power-chassis-on@0.target";

constexpr auto activeState = "active";

/* Map a transition to it's systemd target */
const std::map<server::Chassis::Transition,std::string> SYSTEMD_TARGET_TABLE =
{
        {server::Chassis::Transition::Off, CHASSIS_STATE_POWEROFF_TGT},
        {server::Chassis::Transition::On, CHASSIS_STATE_POWERON_TGT}
};

constexpr auto SYSTEMD_SERVICE   = "org.freedesktop.systemd1";
constexpr auto SYSTEMD_OBJ_PATH  = "/org/freedesktop/systemd1";
constexpr auto SYSTEMD_INTERFACE = "org.freedesktop.systemd1.Manager";

constexpr auto SYSTEMD_PRP_INTERFACE = "org.freedesktop.DBus.Properties";

void Chassis::subscribeToSystemdSignals()
{
    auto method = this->bus.new_method_call(SYSTEMD_SERVICE,
                                            SYSTEMD_OBJ_PATH,
                                            SYSTEMD_INTERFACE,
                                            "Subscribe");
    this->bus.call_noreply(method);

    return;
}

// TODO - Will be rewritten once sdbusplus client bindings are in place
//        and persistent storage design is in place and sdbusplus
//        has read property function
void Chassis::determineInitialState()
{
    sdbusplus::message::variant<int> pgood = -1;
    auto method = this->bus.new_method_call("org.openbmc.control.Power",
                                            "/org/openbmc/control/power0",
                                            "org.freedesktop.DBus.Properties",
                                            "Get");

    method.append("org.openbmc.control.Power", "pgood");
    auto reply = this->bus.call(method);
    reply.read(pgood);

    if(pgood == 1)
    {
        log<level::INFO>("Initial Chassis State will be On",
                         entry("CHASSIS_CURRENT_POWER_STATE=%s",
                               convertForMessage(PowerState::On).c_str()));
        server::Chassis::currentPowerState(PowerState::On);
        server::Chassis::requestedPowerTransition(Transition::On);
    }
    else
    {
        log<level::INFO>("Initial Chassis State will be Off",
                         entry("CHASSIS_CURRENT_POWER_STATE=%s",
                               convertForMessage(PowerState::Off).c_str()));
        server::Chassis::currentPowerState(PowerState::Off);
        server::Chassis::requestedPowerTransition(Transition::Off);
    }

    return;
}

void Chassis::executeTransition(Transition tranReq)
{
    auto sysdTarget = SYSTEMD_TARGET_TABLE.find(tranReq)->second;

    auto method = this->bus.new_method_call(SYSTEMD_SERVICE,
                                            SYSTEMD_OBJ_PATH,
                                            SYSTEMD_INTERFACE,
                                            "StartUnit");

    method.append(sysdTarget);
    method.append("replace");

    this->bus.call_noreply(method);

    return;
}

bool Chassis::isPoweringOff()
{
    sdbusplus::message::variant<std::string> currentState;
    sdbusplus::message::object_path unitTargetPath;

    auto method = this->bus.new_method_call(SYSTEMD_SERVICE,
                                            SYSTEMD_OBJ_PATH,
                                            SYSTEMD_INTERFACE,
                                            "GetUnit");

    method.append(CHASSIS_STATE_POWERON_TGT);
    auto result = this->bus.call(method);

    //Check that the bus call didn't result in an error
    if(result.is_method_error())
    {
        log<level::ERR>("Error in bus call - could not resolve GetUnit for: ");
        log<level::ERR>(SYSTEMD_INTERFACE);
        return false;
    }

    result.read(unitTargetPath);

    method = this->bus.new_method_call(SYSTEMD_SERVICE,
                                       static_cast<const std::string&>
                                           (unitTargetPath).c_str(),
                                       SYSTEMD_PRP_INTERFACE,
                                       "Get");

    method.append("org.freedesktop.systemd1.Unit", "ActiveState");
    result = this->bus.call(method);

    //Check that the bus call didn't result in an error
    if(result.is_method_error())
    {
        log<level::ERR>("Error in bus call - could not resolve Get for: ");
        log<level::ERR>(SYSTEMD_PRP_INTERFACE);
        return false;
    }

    //Is obmc-chassis-power-off@0.target active or inactive?
    result.read(currentState);

    if(currentState != activeState)
    {
        //True - then its powering down
        return true;
    }
    else
    {
        //False - then its powering up
        return false;
    }

}

int Chassis::sysStateChangeSignal(sd_bus_message *msg, void *userData,
                                  sd_bus_error *retError)
{
    return static_cast<Chassis*>(userData)->sysStateChange(msg, retError);
}

int Chassis::sysStateChange(sd_bus_message* msg,
                            sd_bus_error* retError)
{
    uint32_t newStateID {};
    sdbusplus::message::object_path newStateObjPath;
    std::string newStateUnit{};
    std::string newStateResult{};

    auto sdPlusMsg = sdbusplus::message::message(msg);
    //Read the msg and populate each variable
    sdPlusMsg.read(newStateID, newStateObjPath, newStateUnit, newStateResult);

    if((newStateUnit == CHASSIS_STATE_POWEROFF_TGT) &&
       (newStateResult == "done") &&
       (isPoweringOff() == true))
    {
        log<level::INFO>("Recieved signal that power OFF is complete");
        this->currentPowerState(server::Chassis::PowerState::Off);
    }
    else if((newStateUnit == CHASSIS_STATE_POWERON_TGT) &&
            (newStateResult == "done") &&
            (isPoweringOff() == false))
     {
         log<level::INFO>("Recieved signal that power ON is complete");
         this->currentPowerState(server::Chassis::PowerState::On);
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
    log<level::INFO>("Change to Chassis Power State",
                     entry("CHASSIS_CURRENT_POWER_STATE=%s",
                           convertForMessage(value).c_str()));
    return server::Chassis::currentPowerState(value);
}

} // namespace manager
} // namespace state
} // namepsace phosphor
