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
using InvalidTimeError =
    sdbusplus::xyz::openbmc_project::ScheduledTime::Error::InvalidTime;
using HostTransition =
    sdbusplus::xyz::openbmc_project::State::server::ScheduledHostTransition;

uint64_t ScheduledHostTransition::scheduledTime(uint64_t value)
{
    if (value == 0)
    {
        // 0 means the function Scheduled Host Transition is disabled
        // to do: check timer, stop timer
        log<level::INFO>("The function Scheduled Host Transition is disabled.");
        return HostTransition::scheduledTime(value);
    }
    // Can't use operator "-" directly, since value's type is uint64_t
    auto deltaTime = seconds(value) - getTime();
    if (deltaTime < seconds(0))
    {
        log<level::ERR>("Scheduled time is earlier than current time. Fail to "
                        "do host transition.");
        elog<InvalidTimeError>(
            InvalidTime::REASON("Scheduled time is in the past"));
    }
    else
    {
        // start timer
    }
    return HostTransition::scheduledTime(value);
}

seconds ScheduledHostTransition::getTime()
{
    auto now = system_clock::now();
    return duration_cast<seconds>(now.time_since_epoch());
}

} // namespace manager
} // namespace state
} // namespace phosphor
