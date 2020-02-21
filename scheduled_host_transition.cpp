#include "scheduled_host_transition.hpp"

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>
#include <phosphor-logging/log.hpp>
#include <xyz/openbmc_project/ScheduledTime/error.hpp>
#include <chrono>
#include <iostream>

namespace phosphor
{
namespace state
{
namespace manager
{

using namespace std::chrono;
using namespace phosphor::logging;
using namespace xyz::openbmc_project::ScheduledTime;
using FailedError =
    sdbusplus::xyz::openbmc_project::ScheduledTime::Error::InvalidTime;
using sdbusplus::exception::SdBusError;
using HostTransition =
    sdbusplus::xyz::openbmc_project::State::server::ScheduledHostTransition;

constexpr auto MAPPER_BUSNAME = "xyz.openbmc_project.ObjectMapper";
constexpr auto MAPPER_PATH = "/xyz/openbmc_project/object_mapper";
constexpr auto MAPPER_INTERFACE = "xyz.openbmc_project.ObjectMapper";
constexpr auto PROPERTY_INTERFACE = "org.freedesktop.DBus.Properties";

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
        // Set scheduledTime to 0 to disable host transition
        HostTransition::scheduledTime(0);

        log<level::INFO>("The function Scheduled Host Transition is disabled.");
        return HostTransition::scheduledTime(value);
    }
    // Can't use operator "-" directly, since value's type is uint64_t
    auto deltaTime = seconds(value) - getTime();
    if (deltaTime < seconds(0))
    {
        // Set scheduledTime to 0 to disable host transition
        HostTransition::scheduledTime(0);

        log<level::ERR>("Scheduled time is earlier than current time. Fail to "
                        "do host transition.");
        elog<FailedError>(InvalidTime::REASON("Scheduled time is in the past"));
    }
    else
    {
        timer.restart(deltaTime);
    }
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

    std::map<std::string, std::vector<std::string>> mapperResponse;
    try
    {
        auto mapperResponseMsg = bus.call(mapper);

        mapperResponseMsg.read(mapperResponse);
        if (mapperResponse.empty())
        {
            log<level::ERR>("Error reading mapper response",
                            entry("PATH=%s", path.c_str()),
                            entry("INTERFACE=%s", interface.c_str()));
            throw std::runtime_error("Error reading mapper response");
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

std::string getProperty(sdbusplus::bus::bus& bus, std::string path,
                        std::string interface, std::string propertyName)
{
    sdbusplus::message::variant<std::string> property;
    std::string service = getService(bus, path, interface);

    auto method = bus.new_method_call(service.c_str(), path.c_str(),
                                      PROPERTY_INTERFACE, "Get");

    method.append(interface, propertyName);

    try
    {
        auto reply = bus.call(method);
        reply.read(property);
    }
    catch (const SdBusError& e)
    {
        log<level::ERR>("Error in property Get", entry("ERROR=%s", e.what()),
                        entry("PROPERTY=%s", propertyName.c_str()));
        throw;
    }

    if (sdbusplus::message::variant_ns::get<std::string>(property).empty())
    {
        log<level::ERR>("Error reading property response",
                        entry("PROPERTY=%s", propertyName.c_str()));
        throw std::runtime_error("Error reading property response");
    }

    return sdbusplus::message::variant_ns::get<std::string>(property);
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
    std::string hostPath = "/xyz/openbmc_project/state/host0";

    // Scheduled Host Transition only support two states: On and Off
    std::string reqState =
        (HostTransition::requestedTransition() == Transition::On) ? "On"
                                                                  : "Off";
    auto reqTrans = "xyz.openbmc_project.State.Host.Transition." + reqState;
    auto currTrans =
        getProperty(bus, hostPath, HOST_BUSNAME, "RequestedHostTransition");

    if (reqTrans != currTrans)
    {
        setProperty(bus, hostPath, HOST_BUSNAME, "RequestedHostTransition",
                    reqTrans);
    }
    else
    {
        log<level::INFO>(
            "The state of host transition is as requestedTransition already.");
    }
}

void ScheduledHostTransition::callback()
{
    hostTransition();
    // Stop timer, since we need to do host transition once only
    timer.setEnabled(false);
    // Set scheduledTime to 0 to disable host transition
    HostTransition::scheduledTime(0);
}

} // namespace manager
} // namespace state
} // namespace phosphor
