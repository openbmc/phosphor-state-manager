#include "scheduled_host_transition.hpp"

#include "utils.hpp"
#include "xyz/openbmc_project/State/Host/server.hpp"

#include <sys/timerfd.h>
#include <unistd.h>

#include <cereal/archives/json.hpp>
#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>
#include <phosphor-logging/lg2.hpp>
#include <xyz/openbmc_project/ScheduledTime/error.hpp>
#include <xyz/openbmc_project/State/Host/error.hpp>

#include <chrono>
#include <filesystem>
#include <fstream>

// Need to do this since its not exported outside of the kernel.
// Refer : https://gist.github.com/lethean/446cea944b7441228298
#ifndef TFD_TIMER_CANCEL_ON_SET
#define TFD_TIMER_CANCEL_ON_SET (1 << 1)
#endif

namespace phosphor
{
namespace state
{
namespace manager
{

PHOSPHOR_LOG2_USING;

namespace fs = std::filesystem;

using namespace std::chrono;
using namespace phosphor::logging;
using namespace xyz::openbmc_project::ScheduledTime;
using InvalidTimeError =
    sdbusplus::xyz::openbmc_project::ScheduledTime::Error::InvalidTime;
using HostTransition =
    sdbusplus::server::xyz::openbmc_project::state::ScheduledHostTransition;
using HostState = sdbusplus::server::xyz::openbmc_project::state::Host;

constexpr auto PROPERTY_TRANSITION = "RequestedHostTransition";
constexpr auto PROPERTY_RESTART_CAUSE = "RestartCause";

uint64_t ScheduledHostTransition::scheduledTime(uint64_t value)
{
    info("A scheduled host transition request has been made for {TIME}", "TIME",
         value);
    if (value == 0)
    {
        // 0 means the function Scheduled Host Transition is disabled
        // Stop the timer if it's running
        if (timer.isEnabled())
        {
            timer.setEnabled(false);
            debug(
                "scheduledTime: The function Scheduled Host Transition is disabled.");
        }
    }
    else
    {
        auto deltaTime = seconds(value) - getTime();
        if (deltaTime < seconds(0))
        {
            error(
                "Scheduled time is earlier than current time. Fail to schedule host transition.");
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

void ScheduledHostTransition::hostTransition()
{
    auto hostName = std::string(HostState::namespace_path::host) +
                    std::to_string(id);
    std::string hostPath =
        sdbusplus::message::object_path(HostState::namespace_path::value) /
        hostName;

    auto reqTrans = convertForMessage(HostTransition::scheduledTransition());

    info("Trying to set requestedTransition to {REQUESTED_TRANSITION}",
         "REQUESTED_TRANSITION", reqTrans);

    utils::setProperty(bus, hostPath, HostState::interface, PROPERTY_TRANSITION,
                       reqTrans);

    // Set RestartCause to indicate this transition is occurring due to a
    // scheduled host transition as long as it's not an off request
    if (HostTransition::scheduledTransition() != HostState::Transition::Off)
    {
        info("Set RestartCause to scheduled power on reason");
        auto resCause =
            convertForMessage(HostState::RestartCause::ScheduledPowerOn);
        utils::setProperty(bus, hostPath, HostState::interface,
                           PROPERTY_RESTART_CAUSE, resCause);
    }
}

void ScheduledHostTransition::callback()
{
    // Stop timer, since we need to do host transition once only
    timer.setEnabled(false);
    hostTransition();
    // Set scheduledTime to 0 to disable host transition and update scheduled
    // values
    scheduledTime(0);
}

void ScheduledHostTransition::initialize()
{
    // Subscribe time change event
    // Choose the MAX time that is possible to avoid misfires.
    constexpr itimerspec maxTime = {
        {0, 0},                                     // it_interval
        {system_clock::duration::max().count(), 0}, // it_value
    };

    // Create and operate on a timer that delivers timer expiration
    // notifications via a file descriptor.
    timeFd = timerfd_create(CLOCK_REALTIME, 0);
    if (timeFd == -1)
    {
        auto eno = errno;
        error("Failed to create timerfd, errno: {ERRNO}, rc: {RC}", "ERRNO",
              eno, "RC", timeFd);
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
        error("Failed to set timerfd, errno: {ERRNO}, rc: {RC}", "ERRNO", eno,
              "RC", r);
        throw std::system_error(eno, std::system_category());
    }

    sd_event_source* es;
    // Add a new I/O event source to an event loop. onTimeChange will be called
    // when the event source is triggered.
    r = sd_event_add_io(event.get(), &es, timeFd, EPOLLIN, onTimeChange, this);
    if (r < 0)
    {
        auto eno = errno;
        error("Failed to add event, errno: {ERRNO}, rc: {RC}", "ERRNO", eno,
              "RC", r);
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
        debug(
            "handleTimeUpdates: The function Scheduled Host Transition is disabled.");
        return;
    }

    auto deltaTime = seconds(schedTime) - getTime();
    if (deltaTime <= seconds(0))
    {
        try
        {
            hostTransition();
        }
        catch (const sdbusplus::exception_t& e)
        {
            using BMCNotReady = sdbusplus::error::xyz::openbmc_project::state::
                host::BMCNotReady;
            // If error indicates BMC is not at Ready error then reschedule for
            // 60s later
            if ((e.name() != nullptr) &&
                (e.name() == std::string_view(BMCNotReady::errName)))
            {
                warning(
                    "BMC is not at ready, reschedule transition request for 60s");
                timer.restart(seconds(60));
                return;
            }
            else
            {
                throw;
            }
        }
        // Set scheduledTime to 0 to disable host transition and update
        // scheduled values
        scheduledTime(0);
    }
    else
    {
        // Start a timer to do host transition at scheduled time
        timer.restart(deltaTime);
    }
}

int ScheduledHostTransition::onTimeChange(
    sd_event_source* /* es */, int fd, uint32_t /* revents */, void* userdata)
{
    auto* schedHostTran = static_cast<ScheduledHostTransition*>(userdata);

    std::array<char, 64> time{};

    // We are not interested in the data here.
    // So read until there is no new data here in the FD
    while (read(fd, time.data(), time.max_size()) > 0)
    {}

    debug("BMC system time is changed");
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
    catch (const std::exception& e)
    {
        error("deserialize exception: {ERROR}", "ERROR", e);
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
