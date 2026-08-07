#include "systemd_target_signal.hpp"

#include "utils.hpp"

#include <fnmatch.h>

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/exception.hpp>
#include <sdbusplus/server/manager.hpp>
#include <xyz/openbmc_project/Common/error.hpp>
#include <xyz/openbmc_project/Logging/Create/client.hpp>
#include <xyz/openbmc_project/Logging/Entry/client.hpp>

#include <string>
#include <unordered_map>
#include <variant>

// Helper type for the ListUnits D-Bus reply tuple.
using UnitInfo =
    std::tuple<std::string,            // [0] name
               std::string,            // [1] description
               std::string,            // [2] load state
               std::string,            // [3] active state
               std::string,            // [4] sub state
               std::string,            // [5] following
               sdbusplus::object_path, // [6] object path
               uint32_t,               // [7] job id
               std::string,            // [8] job type
               sdbusplus::object_path  // [9] job path
               >;

namespace phosphor::state::manager
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
    for (const auto& [pattern, target] : this->targetData)
    {
        if (fnmatch(pattern.c_str(), unit.c_str(), 0) != 0)
        {
            continue;
        }

        if (std::find(target.errorsToMonitor.begin(),
                      target.errorsToMonitor.end(), result) !=
            target.errorsToMonitor.end())
        {
            info(
                "Monitored systemd unit has hit an error, unit:{UNIT}, result:{RESULT}",
                "UNIT", unit, "RESULT", result);

            // Generate a BMC dump when a monitored target fails
            utils::createBmcDump(this->bus);

            return (target.errorToLog);
        }
    }

    for (const auto& pattern : this->serviceData)
    {
        if (fnmatch(pattern.c_str(), unit.c_str(), 0) != 0)
        {
            continue;
        }

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
    sdbusplus::object_path objPath;
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

    expandServiceWildcards();

    // Call destructor on match callback since application is now subscribed to
    // systemd signals
    this->systemdNameOwnedChangedSignal.~match();

    // Now that systemd is available, set up immediate-quiesce monitoring
    initImmediateQuiesceMonitoring();

    return;
}

void SystemdTargetLogging::expandServiceWildcards()
{
    // Call ListUnits to get every loaded unit
    std::vector<UnitInfo> units;

    try
    {
        auto method =
            this->bus.new_method_call(SYSTEMD_SERVICE, SYSTEMD_OBJ_PATH,
                                      SYSTEMD_MANAGER_INTERFACE, "ListUnits");
        auto reply = this->bus.call(method);
        reply.read(units);
    }
    catch (const sdbusplus::exception_t& e)
    {
        error("expandServiceWildcards: ListUnits failed: {ERROR}", "ERROR", e);
        return;
    }

    auto installMatch =
        [this](const std::string& unitName, const std::string& objPath) {
            info("Installing PropertiesChanged monitor for service "
                 "{UNIT} at {PATH}",
                 "UNIT", unitName, "PATH", objPath);

            serviceUnitMatches.emplace_back(
                this->bus,
                sdbusplus::bus::match::rules::propertiesChanged(
                    objPath, SYSTEMD_UNIT_INTERFACE),
                [this, unitName](sdbusplus::message_t& m) {
                    serviceUnitPropertiesChanged(unitName, m);
                });
        };

    for (const auto& pattern : this->serviceData)
    {
        const bool isWildcard = (pattern.find('*') != std::string::npos);

        if (isWildcard)
        {
            // Wildcard: scan ListUnits results
            bool matched = false;
            for (const auto& u : units)
            {
                const auto& uName = std::get<0>(u);
                const auto& uPath = std::get<6>(u);
                if (fnmatch(pattern.c_str(), uName.c_str(), 0) == 0)
                {
                    installMatch(uName, static_cast<const std::string&>(uPath));
                    matched = true;
                }
            }
            if (!matched)
            {
                info("expandServiceWildcards: no units matched pattern "
                     "{PATTERN} at init time",
                     "PATTERN", pattern);
            }
        }
        else
        {
            // Resolve concrete name via GetUnit
            try
            {
                auto method = this->bus.new_method_call(
                    SYSTEMD_SERVICE, SYSTEMD_OBJ_PATH,
                    SYSTEMD_MANAGER_INTERFACE, "GetUnit");
                method.append(pattern);
                auto reply = this->bus.call(method);
                sdbusplus::object_path objPath;
                reply.read(objPath);
                installMatch(pattern, static_cast<const std::string&>(objPath));
            }
            catch (const sdbusplus::exception_t& e)
            {
                // Unit not yet loaded — the JobRemoved path will still
                // catch it if it fails after being started later.
                info("expandServiceWildcards: GetUnit({UNIT}) not found at "
                     "init time: {ERROR}",
                     "UNIT", pattern, "ERROR", e);
            }
        }
    }
}

void SystemdTargetLogging::initImmediateQuiesceMonitoring()
{
    if (this->immediateQuiesceServiceData.empty())
    {
        return;
    }

    // Guard against duplicate initialization (e.g. if systemd restarts
    // on dbus and subscribeToSystemdSignals is called again)
    if (this->immediateQuiesceMonitoringInitialized)
    {
        return;
    }

    // Call ListUnits once up front so wildcard patterns can be expanded
    // without a separate D-Bus call per pattern.
    std::vector<UnitInfo> units;

    try
    {
        auto method =
            this->bus.new_method_call(SYSTEMD_SERVICE, SYSTEMD_OBJ_PATH,
                                      SYSTEMD_MANAGER_INTERFACE, "ListUnits");
        auto reply = this->bus.call(method);
        reply.read(units);
    }
    catch (const sdbusplus::exception_t& e)
    {
        error("initImmediateQuiesceMonitoring: ListUnits failed: {ERROR}",
              "ERROR", e);
        return;
    }

    // Helper: install a PropertiesChanged match for a resolved unit and check
    // its current ActiveState to close the startup race window.
    auto installAndCheck = [this](const std::string& service,
                                  const sdbusplus::object_path& unitPath) {
        // Install a PropertiesChanged match on this unit's
        // org.freedesktop.systemd1.Unit interface
        auto matchRule = sdbusplus::match_rules::propertiesChanged(
            unitPath.string(), SYSTEMD_UNIT_INTERFACE);

        this->immediateQuiesceMatches.emplace_back(
            this->bus, matchRule,
            [this, svcName = service](sdbusplus::message_t& m) {
                processImmediateQuiesceStateChange(m, svcName);
            });

        // After installing the match, read the current ActiveState to
        // catch services that already failed before we started monitoring.
        // This closes the race where a service crashes before our match
        // is in place -- the PropertiesChanged signal would have been
        // missed, but the state is already "failed".
        auto getMethod = this->bus.new_method_call(SYSTEMD_SERVICE, unitPath,
                                                   PROPERTY_INTERFACE, "Get");
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
    };

    for (const auto& pattern : this->immediateQuiesceServiceData)
    {
        const bool isWildcard = (pattern.find('*') != std::string::npos);

        if (isWildcard)
        {
            // Wildcard: expand against the ListUnits snapshot
            bool matched = false;
            for (const auto& u : units)
            {
                const auto& uName = std::get<0>(u);
                const auto& uPath = std::get<6>(u);
                if (fnmatch(pattern.c_str(), uName.c_str(), 0) == 0)
                {
                    info("Immediate-quiesce wildcard {PATTERN} matched {UNIT}",
                         "PATTERN", pattern, "UNIT", uName);
                    installAndCheck(uName, uPath);
                    matched = true;
                }
            }
            if (!matched)
            {
                info("initImmediateQuiesceMonitoring: no units matched pattern "
                     "{PATTERN} at init time",
                     "PATTERN", pattern);
            }
        }
        else
        {
            // Use LoadUnit to resolve the service name to a unit object path.
            // LoadUnit will load the unit into memory if it isn't already.
            auto method = this->bus.new_method_call(
                SYSTEMD_SERVICE, SYSTEMD_OBJ_PATH, SYSTEMD_MANAGER_INTERFACE,
                "LoadUnit");
            method.append(pattern);

            sdbusplus::object_path unitPath;
            try
            {
                unitPath =
                    this->bus.call(method).unpack<sdbusplus::object_path>();
            }
            catch (const sdbusplus::exception_t& e)
            {
                error("Failed to load unit for immediate-quiesce monitoring, "
                      "unit:{UNIT}, error:{ERROR}",
                      "UNIT", pattern, "ERROR", e);
                continue;
            }

            installAndCheck(pattern, unitPath);
        }
    }

    this->immediateQuiesceMonitoringInitialized = true;
}

void SystemdTargetLogging::processImmediateQuiesceStateChange(
    sdbusplus::message_t& msg, const std::string& unitName)
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

void SystemdTargetLogging::serviceUnitPropertiesChanged(
    const std::string& unitName, sdbusplus::message_t& msg)
{
    std::string interface;
    std::unordered_map<std::string, std::variant<std::string>> changed;

    try
    {
        msg.read(interface, changed);
    }
    catch (const sdbusplus::exception_t& e)
    {
        error("serviceUnitPropertiesChanged: failed to read message: {ERROR}",
              "ERROR", e);
        return;
    }

    auto it = changed.find("ActiveState");
    if (it == changed.end())
    {
        return;
    }

    const auto* activeStatePtr = std::get_if<std::string>(&it->second);
    if (activeStatePtr == nullptr || *activeStatePtr != "failed")
    {
        return;
    }

    info("PropertiesChanged: monitored service {UNIT} entered failed state",
         "UNIT", unitName);

    const std::string errorToLog = processError(unitName, "failed");
    if (!errorToLog.empty())
    {
        logError(errorToLog, "failed", unitName);
    }
}

} // namespace phosphor::state::manager
