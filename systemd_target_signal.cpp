#include "systemd_target_signal.hpp"

#include "utils.hpp"

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/exception.hpp>
#include <sdbusplus/server/manager.hpp>
#include <xyz/openbmc_project/Common/error.hpp>
#include <xyz/openbmc_project/Logging/Create/client.hpp>
#include <xyz/openbmc_project/Logging/Entry/client.hpp>

#include <string>
#include <variant>

namespace phosphor
{
namespace state
{
namespace manager
{

using phosphor::logging::elog;
PHOSPHOR_LOG2_USING;

using sdbusplus::xyz::openbmc_project::Common::Error::InternalFailure;

using LoggingCreate =
    sdbusplus::client::xyz::openbmc_project::logging::Create<>;
using LoggingEntry = sdbusplus::client::xyz::openbmc_project::logging::Entry<>;

constexpr auto CRITICAL_SERVICE_ERROR =
    "xyz.openbmc_project.State.Error.CriticalServiceFailure";

void SystemdTargetLogging::startBmcQuiesceTarget()
{
    auto method =
        this->bus.new_method_call(SYSTEMD_SERVICE, SYSTEMD_OBJ_PATH,
                                  SYSTEMD_MANAGER_INTERFACE, "StartUnit");

    // TODO: Enhance when needed to support multiple-bmc instance systems
    method.append("obmc-bmc-service-quiesce@0.target");
    method.append("replace");
    try
    {
        this->bus.call_noreply(method);
    }
    catch (const sdbusplus::exception_t& e)
    {
        error("Failed to start BMC quiesce target, exception:{ERROR}", "ERROR",
              e);
        // just continue, this is error path anyway so we're just doing what
        // we can
    }

    return;
}

void SystemdTargetLogging::logError(const std::string& errorLog,
                                    const std::string& result,
                                    const std::string& unit)
{
    auto method = this->bus.new_method_call(
        LoggingCreate::default_service, LoggingCreate::instance_path,
        LoggingCreate::interface, "Create");
    // Signature is ssa{ss}
    method.append(
        errorLog, LoggingEntry::Level::Critical,
        std::array<std::pair<std::string, std::string>, 2>(
            {std::pair<std::string, std::string>({"SYSTEMD_RESULT", result}),
             std::pair<std::string, std::string>({"SYSTEMD_UNIT", unit})}));
    try
    {
        this->bus.call_noreply(method);
    }
    catch (const sdbusplus::exception_t& e)
    {
        error("Failed to create systemd target error, error:{ERROR_MSG}, "
              "result:{RESULT}, exception:{ERROR}",
              "ERROR_MSG", errorLog, "RESULT", result, "ERROR", e);
    }
}

std::string SystemdTargetLogging::processError(const std::string& unit,
                                               const std::string& result)
{
    auto targetEntry = this->targetData.find(unit);
    if (targetEntry != this->targetData.end())
    {
        // Check if its result matches any of our monitored errors
        if (std::find(targetEntry->second.errorsToMonitor.begin(),
                      targetEntry->second.errorsToMonitor.end(), result) !=
            targetEntry->second.errorsToMonitor.end())
        {
            info(
                "Monitored systemd unit has hit an error, unit:{UNIT}, result:{RESULT}",
                "UNIT", unit, "RESULT", result);

            // Generate a BMC dump when a monitored target fails
            utils::createBmcDump(this->bus);
            return (targetEntry->second.errorToLog);
        }
    }

    // Check if it's in our list of services to monitor
    if (std::find(this->serviceData.begin(), this->serviceData.end(), unit) !=
        this->serviceData.end())
    {
        if (result == "failed")
        {
            info(
                "Monitored systemd service has hit an error, unit:{UNIT}, result:{RESULT}",
                "UNIT", unit, "RESULT", result);

            // Generate a BMC dump when a critical service fails
            utils::createBmcDump(this->bus);
            // Enter BMC Quiesce when a critical service fails
            startBmcQuiesceTarget();
            return std::string{CRITICAL_SERVICE_ERROR};
        }
    }

    return (std::string{});
}

void SystemdTargetLogging::systemdUnitChange(sdbusplus::message_t& msg)
{
    uint32_t id;
    sdbusplus::message::object_path objPath;
    std::string unit{};
    std::string result{};

    msg.read(id, objPath, unit, result);

    // In most cases it will just be success, in which case just return
    if (result != "done")
    {
        const std::string error = processError(unit, result);

        // If this is a monitored error then log it
        if (!error.empty())
        {
            logError(error, result, unit);
        }
    }
    return;
}

void SystemdTargetLogging::processNameChangeSignal(sdbusplus::message_t& msg)
{
    std::string name;      // well-known
    std::string old_owner; // unique-name
    std::string new_owner; // unique-name

    msg.read(name, old_owner, new_owner);

    // Looking for systemd to be on dbus so we can call it
    if (name == SYSTEMD_SERVICE)
    {
        info("org.freedesktop.systemd1 is now on dbus");
        subscribeToSystemdSignals();
    }
    return;
}

void SystemdTargetLogging::subscribeToSystemdSignals()
{
    auto method =
        this->bus.new_method_call(SYSTEMD_SERVICE, SYSTEMD_OBJ_PATH,
                                  SYSTEMD_MANAGER_INTERFACE, "Subscribe");

    try
    {
        this->bus.call(method);
    }
    catch (const sdbusplus::exception_t& e)
    {
        // If error indicates systemd is not on dbus yet then do nothing.
        // The systemdNameChangeSignals callback will detect when it is on
        // dbus and then call this function again
        const std::string noDbus("org.freedesktop.DBus.Error.ServiceUnknown");
        if (noDbus == e.name())
        {
            info("org.freedesktop.systemd1 not on dbus yet");
        }
        else
        {
            error("Failed to subscribe to systemd signals: {ERROR}", "ERROR",
                  e);
            elog<InternalFailure>();
        }
        return;
    }

    // Call destructor on match callback since application is now subscribed to
    // systemd signals
    this->systemdNameOwnedChangedSignal.~match();

    // Now that systemd is available, set up state-change monitoring
    initStateChangeMonitoring();

    return;
}

void SystemdTargetLogging::initStateChangeMonitoring()
{
    if (this->stateChangeServiceData.empty())
    {
        return;
    }

    // Guard against duplicate initialization (e.g. if systemd restarts
    // on dbus and subscribeToSystemdSignals is called again)
    if (this->stateChangeMonitoringInitialized)
    {
        return;
    }

    for (const auto& service : this->stateChangeServiceData)
    {
        // Use LoadUnit to resolve the service name to a unit object path.
        // LoadUnit will load the unit into memory if it isn't already.
        auto method =
            this->bus.new_method_call(SYSTEMD_SERVICE, SYSTEMD_OBJ_PATH,
                                      SYSTEMD_MANAGER_INTERFACE, "LoadUnit");
        method.append(service);

        sdbusplus::message::object_path unitPath;
        try
        {
            unitPath = this->bus.call(method)
                           .unpack<sdbusplus::message::object_path>();
        }
        catch (const sdbusplus::exception_t& e)
        {
            error("Failed to load unit for state-change monitoring, "
                  "unit:{UNIT}, error:{ERROR}",
                  "UNIT", service, "ERROR", e);
            continue;
        }

        // Install a PropertiesChanged match on this unit's
        // org.freedesktop.systemd1.Unit interface
        auto matchRule = sdbusplus::bus::match::rules::propertiesChanged(
            unitPath.str, SYSTEMD_UNIT_INTERFACE);

        this->stateChangeMatches.emplace_back(
            this->bus, matchRule,
            [this, svcName = service](sdbusplus::message_t& m) {
                processStateChange(m, svcName);
            });

        // After installing the match, read the current ActiveState to
        // catch services that already failed before we started monitoring.
        // This closes the race where a service crashes before our match
        // is in place — the PropertiesChanged signal would have been
        // missed, but the state is already "failed".
        auto getMethod = this->bus.new_method_call(
            SYSTEMD_SERVICE, unitPath.str.c_str(), PROPERTY_INTERFACE, "Get");
        getMethod.append(SYSTEMD_UNIT_INTERFACE, "ActiveState");

        try
        {
            auto currentState =
                this->bus.call(getMethod).unpack<std::variant<std::string>>();
            const auto* stateStr = std::get_if<std::string>(&currentState);
            if (stateStr != nullptr && *stateStr == "failed")
            {
                info("Immediate-quiesce service already in failed state "
                     "at monitor startup, unit:{UNIT}, result:{RESULT}",
                     "UNIT", service, "RESULT", *stateStr);
                utils::createBmcDump(this->bus);
                logError(CRITICAL_SERVICE_ERROR, *stateStr, service);
                startBmcQuiesceTarget();
            }
        }
        catch (const sdbusplus::exception_t& e)
        {
            error("Failed to read current ActiveState for unit:{UNIT}, "
                  "error:{ERROR}",
                  "UNIT", service, "ERROR", e);
        }
    }

    this->stateChangeMonitoringInitialized = true;
}

void SystemdTargetLogging::processStateChange(sdbusplus::message_t& msg,
                                              const std::string& unitName)
{
    // PropertiesChanged carries all changed properties. systemd unit
    // properties include various types, so the variant must be wide enough
    // to deserialize the entire signal even though we only inspect
    // ActiveState (string).
    using PropVariant = std::variant<std::string, bool, uint32_t, uint64_t,
                                     int32_t, int64_t, double>;
    using PropMap = std::map<std::string, PropVariant>;

    std::string interface;
    PropMap changedProperties;

    try
    {
        msg.read(interface, changedProperties);
    }
    catch (const sdbusplus::exception_t& e)
    {
        error("Failed to read PropertiesChanged signal for unit:{UNIT}, "
              "error:{ERROR}",
              "UNIT", unitName, "ERROR", e);
        return;
    }

    auto it = changedProperties.find("ActiveState");
    if (it == changedProperties.end())
    {
        return;
    }

    const auto* activeStatePtr = std::get_if<std::string>(&it->second);
    if (activeStatePtr == nullptr || *activeStatePtr != "failed")
    {
        return;
    }

    info("Monitored immediate-quiesce service has hit an error, "
         "unit:{UNIT}, result:{RESULT}",
         "UNIT", unitName, "RESULT", *activeStatePtr);

    // Generate a BMC dump when an immediate-quiesce service fails
    utils::createBmcDump(this->bus);

    // Log the error
    logError(CRITICAL_SERVICE_ERROR, *activeStatePtr, unitName);

    // Enter BMC Quiesce
    startBmcQuiesceTarget();
}

} // namespace manager
} // namespace state
} // namespace phosphor
