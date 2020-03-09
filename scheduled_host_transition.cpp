#include "scheduled_host_transition.hpp"

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>
#include <phosphor-logging/log.hpp>
#include <xyz/openbmc_project/ScheduledTime/error.hpp>
#include <cereal/archives/json.hpp>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <sys/timerfd.h>
#include <unistd.h>

// Need to do this since its not exported outside of the kernel.
// Refer : https://gist.github.com/lethean/446cea944b7441228298
#ifndef TFD_TIMER_CANCEL_ON_SET
#define TFD_TIMER_CANCEL_ON_SET (1 << 1)
#endif

// Needed to make sure timerfd does not misfire even though we set CANCEL_ON_SET
#define TIME_T_MAX (time_t)((1UL << ((sizeof(time_t) << 3) - 1)) - 1)

namespace phosphor
{
namespace state
{
namespace manager
{

namespace fs = std::filesystem;

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

        log<level::INFO>("scheduledTime: The function Scheduled Host "
                         "Transition is disabled.");
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

    // Set scheduledTime
    HostTransition::scheduledTime(value);
    // Store scheduled values
    serializeScheduledValues();

    return value;
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
    // Update scheduled values
    serializeScheduledValues();
}

void ScheduledHostTransition::initialize()
{
    // Subscribe time change event
    // Choose the MAX time that is possible to avoid mis fires.
    constexpr itimerspec maxTime = {
        {0, 0},          // it_interval
        {TIME_T_MAX, 0}, // it_value
    };

    // Create and operate on a timer that delivers timer expiration
    // notifications via a file descriptor.
    timeFd = timerfd_create(CLOCK_REALTIME, 0);
    if (timeFd == -1)
    {
        auto eno = errno;
        log<level::ERR>("Failed to create timerfd", entry("ERRNO=%d", eno),
                        entry("RC=%d", timeFd));
        throw std::system_error(eno, std::system_category());
    }

    // Starts the timer referred to by the file descriptor fd.
    // If TFD_TIMER_CANCEL_ON_SET is specified along with TFD_TIMER_ABSTIME
    // and the clock for this timer is CLOCK_REALTIME, then mark this timer
    // as cancelable if the real-time clock undergoes a discontinuous change.
    // In this way, we can monitor whether BMC time is changed or not.
    auto r = timerfd_settime(
        timeFd, TFD_TIMER_ABSTIME | TFD_TIMER_CANCEL_ON_SET, &maxTime, nullptr);
    if (r != 0)
    {
        auto eno = errno;
        log<level::ERR>("Failed to set timerfd", entry("ERRNO=%d", eno),
                        entry("RC=%d", r));
        throw std::system_error(eno, std::system_category());
    }

    sd_event_source* es;
    // Add a new I/O event source to an event loop. onTimeChange will be called
    // when the event source is triggered.
    r = sd_event_add_io(event.get(), &es, timeFd, EPOLLIN, onTimeChange, this);
    if (r < 0)
    {
        auto eno = errno;
        log<level::ERR>("Failed to add event", entry("ERRNO=%d", eno),
                        entry("RC=%d", r));
        throw std::system_error(eno, std::system_category());
    }
    timeChangeEventSource.reset(es);
}

ScheduledHostTransition::~ScheduledHostTransition()
{
    close(timeFd);
}

void ScheduledHostTransition::handleTimeUpdates()
{
    // Stop the timer if it's running.
    // Don't return directly when timer is stopped, because the timer is always
    // disabled after the BMC is rebooted.
    if (timer.isEnabled())
    {
        timer.setEnabled(false);
    }

    // Get scheduled time
    auto schedTime = HostTransition::scheduledTime();

    if (schedTime == 0)
    {
        log<level::INFO>("handleTimeUpdates: The function Scheduled Host "
                         "Transition is disabled.");
        return;
    }

    auto deltaTime = seconds(schedTime) - getTime();
    if (deltaTime <= seconds(0))
    {
        // When BMC time is changed to be later than scheduled time, check the
        // state of host transition to decide whether need to do host transition
        hostTransition();
        // Set scheduledTime to 0 to disable host transition
        HostTransition::scheduledTime(0);
        // Update scheduled values
        serializeScheduledValues();
    }
    else
    {
        // Start a timer to do host transition at scheduled time
        timer.restart(deltaTime);
    }
}

int ScheduledHostTransition::onTimeChange(sd_event_source* /* es */, int fd,
                                          uint32_t /* revents */,
                                          void* userdata)
{
    auto schedHostTran = static_cast<ScheduledHostTransition*>(userdata);

    std::array<char, 64> time{};

    // We are not interested in the data here.
    // So read until there is no new data here in the FD
    while (read(fd, time.data(), time.max_size()) > 0)
        ;

    log<level::INFO>("BMC system time is changed");
    schedHostTran->handleTimeUpdates();

    return 0;
}

void ScheduledHostTransition::serializeScheduledValues()
{
    fs::path path{SCHEDULED_HOST_TRANSITION_PERSIST_PATH};
    std::ofstream os(path.c_str(), std::ios::binary);
    cereal::JSONOutputArchive oarchive(os);

    oarchive(HostTransition::scheduledTime(),
             HostTransition::scheduledTransition());
}

bool ScheduledHostTransition::deserializeScheduledValues(uint64_t& time,
                                                         Transition& trans)
{
    fs::path path{SCHEDULED_HOST_TRANSITION_PERSIST_PATH};

    try
    {
        if (fs::exists(path))
        {
            std::ifstream is(path.c_str(), std::ios::in | std::ios::binary);
            cereal::JSONInputArchive iarchive(is);
            iarchive(time, trans);
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

void ScheduledHostTransition::restoreScheduledValues()
{
    uint64_t time;
    Transition trans;
    if (!deserializeScheduledValues(time, trans))
    {
        // set to default value
        HostTransition::scheduledTime(0);
        HostTransition::scheduledTransition(Transition::On);
    }
    else
    {
        HostTransition::scheduledTime(time);
        HostTransition::scheduledTransition(trans);
        // Rebooting BMC is something like the BMC time is changed,
        // so go on with the same process as BMC time changed.
        handleTimeUpdates();
    }
}

} // namespace manager
} // namespace state
} // namespace phosphor
