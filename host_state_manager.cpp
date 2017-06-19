#include <iostream>
#include <map>
#include <string>
#include <systemd/sd-bus.h>
#include <sdbusplus/server.hpp>
#include <phosphor-logging/log.hpp>
#include "host_state_manager.hpp"

namespace phosphor
{
namespace state
{
namespace manager
{

// When you see server:: you know we're referencing our base class
namespace server = sdbusplus::xyz::openbmc_project::State::server;

using namespace phosphor::logging;

// host-shutdown notifies host of shutdown and that leads to host-stop being
// called so initiate a host shutdown with the -shutdown target and consider the
// host shut down when the -stop target is complete
constexpr auto HOST_STATE_SOFT_POWEROFF_TGT = "obmc-host-shutdown@0.target";
constexpr auto HOST_STATE_POWEROFF_TGT = "obmc-host-stop@0.target";
constexpr auto HOST_STATE_POWERON_TGT = "obmc-host-start@0.target";
constexpr auto HOST_STATE_QUIESCE_TGT = "obmc-host-quiesce@0.target";

constexpr auto ACTIVE_STATE = "active";
constexpr auto ACTIVATING_STATE = "activating";

/* Map a transition to it's systemd target */
const std::map<server::Host::Transition,std::string> SYSTEMD_TARGET_TABLE =
{
        {server::Host::Transition::Off, HOST_STATE_SOFT_POWEROFF_TGT},
        {server::Host::Transition::On, HOST_STATE_POWERON_TGT}
};

constexpr auto SYSTEMD_SERVICE   = "org.freedesktop.systemd1";
constexpr auto SYSTEMD_OBJ_PATH  = "/org/freedesktop/systemd1";
constexpr auto SYSTEMD_INTERFACE = "org.freedesktop.systemd1.Manager";

constexpr auto MAPPER_BUSNAME = "xyz.openbmc_project.ObjectMapper";
constexpr auto MAPPER_PATH = "/xyz/openbmc_project/object_mapper";
constexpr auto MAPPER_INTERFACE = "xyz.openbmc_project.ObjectMapper";

constexpr auto REBOOTCOUNTER_SERVICE("org.openbmc.Sensors");
constexpr auto REBOOTCOUNTER_PATH("/org/openbmc/sensors/host/BootCount");
constexpr auto REBOOTCOUNTER_INTERFACE("org.openbmc.SensorValue");

constexpr auto SYSTEMD_PROPERTY_IFACE = "org.freedesktop.DBus.Properties";
constexpr auto SYSTEMD_INTERFACE_UNIT = "org.freedesktop.systemd1.Unit";

/* Map a system state to the HostState */
const std::map<std::string, server::Host::HostState> SYS_HOST_STATE_TABLE = {
        {"HOST_BOOTING", server::Host::HostState::Running},
        {"HOST_POWERED_OFF", server::Host::HostState::Off},
        {"HOST_QUIESCED", server::Host::HostState::Quiesced}
};

void Host::subscribeToSystemdSignals()
{
    auto method = this->bus.new_method_call(SYSTEMD_SERVICE,
                                            SYSTEMD_OBJ_PATH,
                                            SYSTEMD_INTERFACE,
                                            "Subscribe");
    this->bus.call_noreply(method);

    return;
}

void Host::determineInitialState()
{

    if(stateActive(HOST_STATE_POWERON_TGT))
    {
        log<level::INFO>("Initial Host State will be Running",
                         entry("CURRENT_HOST_STATE=%s",
                               convertForMessage(HostState::Running).c_str()));
        server::Host::currentHostState(HostState::Running);
        server::Host::requestedHostTransition(Transition::On);
    }
    else
    {
        log<level::INFO>("Initial Host State will be Off",
                         entry("CURRENT_HOST_STATE=%s",
                               convertForMessage(HostState::Off).c_str()));
        server::Host::currentHostState(HostState::Off);
        server::Host::requestedHostTransition(Transition::Off);
    }

    // Set transition initially to Off
    // TODO - Eventually need to restore this from persistent storage
    server::Host::requestedHostTransition(Transition::Off);

    return;
}

void Host::executeTransition(Transition tranReq)
{
    auto sysdUnit = SYSTEMD_TARGET_TABLE.find(tranReq)->second;

    auto method = this->bus.new_method_call(SYSTEMD_SERVICE,
                                            SYSTEMD_OBJ_PATH,
                                            SYSTEMD_INTERFACE,
                                            "StartUnit");

    method.append(sysdUnit);
    method.append("replace");

    this->bus.call_noreply(method);

    return;
}

bool Host::stateActive(const std::string& target)
{
    sdbusplus::message::variant<std::string> currentState;
    sdbusplus::message::object_path unitTargetPath;

    auto method = this->bus.new_method_call(SYSTEMD_SERVICE,
                                            SYSTEMD_OBJ_PATH,
                                            SYSTEMD_INTERFACE,
                                            "GetUnit");

    method.append(target);
    auto result = this->bus.call(method);

    //Check that the bus call didn't result in an error
    if(result.is_method_error())
    {
        log<level::ERR>("Error in bus call - could not resolve GetUnit for:",
                        entry(" %s", SYSTEMD_INTERFACE));
        return false;
    }

    result.read(unitTargetPath);

    method = this->bus.new_method_call(SYSTEMD_SERVICE,
                                       static_cast<const std::string&>
                                           (unitTargetPath).c_str(),
                                       SYSTEMD_PROPERTY_IFACE,
                                       "Get");

    method.append(SYSTEMD_INTERFACE_UNIT, "ActiveState");
    result = this->bus.call(method);

    //Check that the bus call didn't result in an error
    if(result.is_method_error())
    {
        log<level::ERR>("Error in bus call - could not resolve Get for:",
                        entry(" %s", SYSTEMD_PROPERTY_IFACE));
        return false;
    }

    result.read(currentState);

    if(currentState != ACTIVE_STATE && currentState != ACTIVATING_STATE)
    {
        //False - not active
        return false;
    }
    //True - active
    return true;
}

void Host::setHostbootCount(int bootCount)
{
    auto method = this->bus.new_method_call(REBOOTCOUNTER_SERVICE,
                                            REBOOTCOUNTER_PATH,
                                            REBOOTCOUNTER_INTERFACE,
                                            "setValue");
    sdbusplus::message::variant<int> newParam = bootCount;
    method.append(newParam);
    this->bus.call_noreply(method);
}

bool Host::isAutoReboot()
{
    sdbusplus::message::variant<std::string> autoRebootParam;
    std::string strParam;

    std::string HOST_PATH("/org/openbmc/settings/host0");
    std::string HOST_INTERFACE("org.openbmc.settings.Host");

    auto mapper = this->bus.new_method_call(MAPPER_BUSNAME,
                                            MAPPER_PATH,
                                            MAPPER_INTERFACE,
                                            "GetObject");

    mapper.append(HOST_PATH, std::vector<std::string>({HOST_INTERFACE}));
    auto mapperResponseMsg = this->bus.call(mapper);

    if (mapperResponseMsg.is_method_error())
    {
        log<level::ERR>("Error in mapper call");
        return false;
    }

    std::map<std::string, std::vector<std::string>> mapperResponse;
    mapperResponseMsg.read(mapperResponse);
    if (mapperResponse.empty())
    {
        log<level::ERR>("Error reading mapper response");
        return false;
    }

    const auto& host = mapperResponse.begin()->first;

    auto method = this->bus.new_method_call(host.c_str(),
                                            HOST_PATH.c_str(),
                                            "org.freedesktop.DBus.Properties",
                                            "Get");

    method.append(HOST_INTERFACE.c_str(), "auto_reboot");
    auto reply = this->bus.call(method);

    if (reply.is_method_error())
    {
        log<level::ERR>("Error in auto_reboot Get");
        return false;
    }

    reply.read(autoRebootParam);
    strParam =
        sdbusplus::message::variant_ns::get<std::string>(autoRebootParam);

    if (strParam.empty())
    {
        log<level::ERR>("Error reading auto_reboot response");
        return false;
    }

    sdbusplus::message::variant<int> rebootCounterParam = 0;
    method = this->bus.new_method_call(REBOOTCOUNTER_SERVICE,
                                       REBOOTCOUNTER_PATH,
                                       REBOOTCOUNTER_INTERFACE,
                                       "getValue");
    reply = this->bus.call(method);
    if (reply.is_method_error())
    {
        log<level::ERR>("Error in BOOTCOUNT getValue");
        return false;
    }
    reply.read(rebootCounterParam);

    if (strParam == "yes")
    {
        if( rebootCounterParam > 0)
        {
            // Reduce BOOTCOUNT by 1
            log<level::INFO>("Auto reboot enabled. "
                             "Reducing HOST BOOTCOUNT by 1.");
            Host::setHostbootCount((sdbusplus::message::variant_ns::
                                    get<int>(rebootCounterParam)) - 1);
            return true;
        }
        else if(rebootCounterParam == 0)
        {
            // Reset reboot counter and go to quiesce state
            log<level::INFO>("Auto reboot enabled. "
                             "HOST BOOTCOUNT already set to 0.");
            Host::setHostbootCount(BOOT_COUNT_MAX_ALLOWED);
            return false;
        }
        else
        {
            log<level::INFO>("Auto reboot enabled. "
                             "HOST BOOTCOUNT has an invalid value.");
            return false;
        }
    }
    else
    {
        log<level::INFO>("Auto reboot disabled.");
        return false;
    }
}

void Host::sysStateChange(sdbusplus::message::message& msg)
{
    uint32_t newStateID {};
    sdbusplus::message::object_path newStateObjPath;
    std::string newStateUnit{};
    std::string newStateResult{};

    //Read the msg and populate each variable
    msg.read(newStateID, newStateObjPath, newStateUnit, newStateResult);

    if((newStateUnit == HOST_STATE_POWEROFF_TGT) &&
       (newStateResult == "done") &&
       (!stateActive(HOST_STATE_POWERON_TGT)))
    {
        log<level::INFO>("Received signal that host is off");
        this->currentHostState(server::Host::HostState::Off);

        // Check if we need to start a new transition (i.e. a Reboot)
        if(this->server::Host::requestedHostTransition() ==
               Transition::Reboot)
        {
            log<level::DEBUG>("Reached intermediate state, going to next");
            this->executeTransition(server::Host::Transition::On);
        }
    }
    else if((newStateUnit == HOST_STATE_POWERON_TGT) &&
            (newStateResult == "done") &&
            (stateActive(HOST_STATE_POWERON_TGT)))
     {
         log<level::INFO>("Received signal that host is running");
         this->currentHostState(server::Host::HostState::Running);
     }
     else if((newStateUnit == HOST_STATE_QUIESCE_TGT) &&
             (newStateResult == "done") &&
             (stateActive(HOST_STATE_QUIESCE_TGT)))
     {
         if (Host::isAutoReboot())
         {
             log<level::INFO>("Beginning reboot...");
             Host::requestedHostTransition(server::Host::Transition::Reboot);
         }
         else
         {
             log<level::INFO>("Maintaining quiesce");
             this->currentHostState(server::Host::HostState::Quiesced);
         }

     }
}

Host::Transition Host::requestedHostTransition(Transition value)
{
    log<level::INFO>(
            "Host State transaction request",
            entry("REQUESTED_HOST_TRANSITION=%s",
                  convertForMessage(value).c_str()));

    Transition tranReq = value;
    if(value == server::Host::Transition::Reboot)
    {
        // On reboot requests we just need to do a off if we're on and
        // vice versa.  The handleSysStateChange() code above handles the
        // second part of the reboot
        if(this->server::Host::currentHostState() ==
            server::Host::HostState::Off)
        {
            tranReq = server::Host::Transition::On;
        }
        else
        {
            tranReq = server::Host::Transition::Off;
        }
    }

    executeTransition(tranReq);
    return server::Host::requestedHostTransition(value);
}

Host::HostState Host::currentHostState(HostState value)
{
    log<level::INFO>("Change to Host State",
                     entry("CURRENT_HOST_STATE=%s",
                           convertForMessage(value).c_str()));
    return server::Host::currentHostState(value);
}

} // namespace manager
} // namespace state
} // namepsace phosphor
