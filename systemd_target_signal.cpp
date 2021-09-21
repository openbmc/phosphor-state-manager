#include "systemd_target_signal.hpp"

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/exception.hpp>
#include <sdbusplus/server/manager.hpp>
#include <sdeventplus/event.hpp>
#include <xyz/openbmc_project/Common/error.hpp>

namespace phosphor
{
namespace state
{
namespace manager
{

using phosphor::logging::elog;
PHOSPHOR_LOG2_USING;

using sdbusplus::xyz::openbmc_project::Common::Error::InternalFailure;

void SystemdTargetLogging::logError(const std::string& errorLog,
                                    const std::string& result)
{
    auto method = this->bus.new_method_call(
        "xyz.openbmc_project.Logging", "/xyz/openbmc_project/logging",
        "xyz.openbmc_project.Logging.Create", "Create");
    // Signature is ssa{ss}
    method.append(errorLog);
    method.append("xyz.openbmc_project.Logging.Entry.Level.Critical");
    method.append(std::array<std::pair<std::string, std::string>, 1>(
        {std::pair<std::string, std::string>({"SYSTEMD_RESULT", result})}));
    try
    {
        this->bus.call_noreply(method);
    }
    catch (const sdbusplus::exception::exception& e)
    {
        error("Failed to create systemd target error, error:{ERROR}, "
              "result:{RESULT}, exception:{EXCEPTION}",
              "ERROR", errorLog, "RESULT", result, "EXCEPTION", e);
    }
}

const std::string* SystemdTargetLogging::processError(const std::string& unit,
                                                      const std::string& result)
{
    auto targetEntry = this->targetData.find(unit);
    if (targetEntry != this->targetData.end())
    {
        // Check if its result matches any of our monitored errors
        if (std::find(targetEntry->second.errorsToMonitor.begin(),
                      targetEntry->second.errorsToMonitor.end(),
                      result) != targetEntry->second.errorsToMonitor.end())
        {
            info(
                "Monitored systemd unit has hit an error, unit:{UNIT}, result:{RESULT}",
                "UNIT", unit, "RESULT", result);
            return (&targetEntry->second.errorToLog);
        }
    }
    return nullptr;
}

void SystemdTargetLogging::systemdUnitChange(sdbusplus::message::message& msg)
{
    uint32_t id;
    sdbusplus::message::object_path objPath;
    std::string unit{};
    std::string result{};

    msg.read(id, objPath, unit, result);

    // In most cases it will just be success, in which case just return
    if (result != "done")
    {
        const std::string* error = processError(unit, result);

        // If this is a monitored error then log it
        if (error)
        {
            logError(*error, result);
        }
    }
    return;
}

void SystemdTargetLogging::processNameChangeSignal(
    sdbusplus::message::message& msg)
{
    std::string name;      // well-known
    std::string old_owner; // unique-name
    std::string new_owner; // unique-name

    msg.read(name, old_owner, new_owner);

    // Looking for systemd to be on dbus so we can call it
    if (name == "org.freedesktop.systemd1")
    {
        info("org.freedesktop.systemd1 is now on dbus");
        subscribeToSystemdSignals();
    }
    return;
}

void SystemdTargetLogging::subscribeToSystemdSignals()
{
    auto method = this->bus.new_method_call(
        "org.freedesktop.systemd1", "/org/freedesktop/systemd1",
        "org.freedesktop.systemd1.Manager", "Subscribe");

    try
    {
        this->bus.call(method);
    }
    catch (const sdbusplus::exception::exception& e)
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
            error("Failed to subscribe to systemd signals: {ERROR}", "ERR", e);
            elog<InternalFailure>();
        }
        return;
    }

    // Call destructor on match callback since application is now subscribed to
    // systemd signals
    this->systemdNameOwnedChangedSignal.~match();

    return;
}

} // namespace manager
} // namespace state
} // namespace phosphor
