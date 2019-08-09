#include <phosphor-logging/log.hpp>
#include <sdbusplus/server/manager.hpp>
#include <sdeventplus/event.hpp>
#include <systemd_target_signal.hpp>

namespace phosphor
{
namespace state
{
namespace manager
{

using phosphor::logging::entry;
using phosphor::logging::level;
using phosphor::logging::log;

void SystemdTargetLogging::logError(const std::string& error,
                                    const std::string& result)
{
    auto method = this->bus.new_method_call(
        "xyz.openbmc_project.Logging", "/xyz/openbmc_project/logging",
        "xyz.openbmc_project.Logging.Create", "Create");
    // Signature is ssa{ss}
    method.append(error);
    method.append("xyz.openbmc_project.Logging.Entry.Level.Critical");
    method.append(std::array<std::pair<std::string, std::string>, 1>(
        {std::pair<std::string, std::string>({"SYSTEMD_RESULT", result})}));
    try
    {
        this->bus.call_noreply(method);
    }
    catch (const SdBusError& e)
    {
        log<level::ERR>("Failed to create systemd target error",
                        entry("ERROR=%s", error.c_str()),
                        entry("RESULT=%s", result.c_str()));
    }
}

std::string* SystemdTargetLogging::processError(const std::string& unit,
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
            log<level::INFO>("Monitored systemd unit has hit an error",
                             entry("UNIT=%s", unit.c_str()),
                             entry("RESULT=%s", result.c_str()));
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
        std::string* error = processError(unit, result);

        // If this is a monitored error then log it
        if (error)
        {
            logError(*error, result);
        }
    }
    return;
}

void SystemdTargetLogging::subscribeToSystemdSignals()
{
    auto method = this->bus.new_method_call(
        "org.freedesktop.systemd1", "/org/freedesktop/systemd1",
        "org.freedesktop.systemd1.Manager", "Subscribe");
    this->bus.call_noreply(method);
    return;
}

} // namespace manager
} // namespace state
} // namespace phosphor
