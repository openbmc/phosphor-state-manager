#include "scheduled_host_transition.hpp"

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>
#include <phosphor-logging/log.hpp>
#include <xyz/openbmc_project/ScheduledTime/error.hpp>
#include <chrono>

namespace phosphor
{
namespace state
{
namespace manager
{

using namespace std::chrono;
using namespace phosphor::logging;
using namespace xyz::openbmc_project::ScheduledTime;
using sdbusplus::exception::SdBusError;
using InvalidTimeError =
    sdbusplus::xyz::openbmc_project::ScheduledTime::Error::InvalidTime;
using HostTransition =
    sdbusplus::xyz::openbmc_project::State::server::ScheduledHostTransition;

constexpr auto MAPPER_BUSNAME = "xyz.openbmc_project.ObjectMapper";
constexpr auto MAPPER_PATH = "/xyz/openbmc_project/object_mapper";
constexpr auto MAPPER_INTERFACE = "xyz.openbmc_project.ObjectMapper";
constexpr auto PROPERTY_INTERFACE = "org.freedesktop.DBus.Properties";
constexpr auto PROPERTY_TRANSITION = "RequestedHostTransition";

uint64_t ScheduledHostTransition::scheduledTime(uint64_t value)
{
    if (value == 0)
    {
        // 0 means the function Scheduled Host Transition is disabled
        // Stop the timer if it's running
        if (timer.isEnabled())
        {
            timer.setEnabled(false);
        }

        log<level::INFO>("The function Scheduled Host Transition is disabled.");
    }
    else
    {
        auto deltaTime = seconds(value) - getTime();
        if (deltaTime < seconds(0))
        {
            log<level::ERR>(
                "Scheduled time is earlier than current time. Fail to "
                "schedule host transition.");
            elog<InvalidTimeError>(
                InvalidTime::REASON("Scheduled time is in the past"));
        }
        else
        {
            // Start a timer to do host transition at scheduled time
            timer.restart(deltaTime);
        }
    }

    // Set and return the scheduled time
    return HostTransition::scheduledTime(value);
}

seconds ScheduledHostTransition::getTime()
{
    auto now = system_clock::now();
    return duration_cast<seconds>(now.time_since_epoch());
}

std::string getService(sdbusplus::bus::bus& bus, std::string path,
                       std::string interface)
{
    auto mapper = bus.new_method_call(MAPPER_BUSNAME, MAPPER_PATH,
                                      MAPPER_INTERFACE, "GetObject");

    mapper.append(path, std::vector<std::string>({interface}));

    std::vector<std::pair<std::string, std::vector<std::string>>>
        mapperResponse;

    try
    {
        auto mapperResponseMsg = bus.call(mapper);

        mapperResponseMsg.read(mapperResponse);
        if (mapperResponse.empty())
        {
            log<level::ERR>("Error no matching service",
                            entry("PATH=%s", path.c_str()),
                            entry("INTERFACE=%s", interface.c_str()));
            throw std::runtime_error("Error no matching service");
        }
    }
    catch (const SdBusError& e)
    {
        log<level::ERR>("Error in mapper call", entry("ERROR=%s", e.what()),
                        entry("PATH=%s", path.c_str()),
                        entry("INTERFACE=%s", interface.c_str()));
        throw;
    }

    return mapperResponse.begin()->first;
}

void setProperty(sdbusplus::bus::bus& bus, const std::string& path,
                 const std::string& interface, const std::string& property,
                 const std::string& value)
{
    sdbusplus::message::variant<std::string> variantValue = value;
    std::string service = getService(bus, path, interface);

    auto method = bus.new_method_call(service.c_str(), path.c_str(),
                                      PROPERTY_INTERFACE, "Set");

    method.append(interface, property, variantValue);
    bus.call_noreply(method);

    return;
}

void ScheduledHostTransition::hostTransition()
{
    auto hostPath = std::string{HOST_OBJPATH} + '0';

    auto reqTrans = convertForMessage(HostTransition::scheduledTransition());

    setProperty(bus, hostPath, HOST_BUSNAME, PROPERTY_TRANSITION, reqTrans);

    log<level::INFO>("Set requestedTransition",
                     entry("REQUESTED_TRANSITION=%s", reqTrans.c_str()));
}

void ScheduledHostTransition::callback()
{
    // Stop timer, since we need to do host transition once only
    timer.setEnabled(false);
    hostTransition();
    // Set scheduledTime to 0 to disable host transition
    HostTransition::scheduledTime(0);
}

} // namespace manager
} // namespace state
} // namespace phosphor
