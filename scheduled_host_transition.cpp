#include "scheduled_host_transition.hpp"

#include <xyz/openbmc_project/ScheduledTime/error.hpp>

namespace phosphor
{
namespace state
{
namespace manager
{

using HostTransition =
    sdbusplus::xyz::openbmc_project::State::server::ScheduledHostTransition;

uint64_t ScheduledHostTransition::scheduledTime(uint64_t value)
{
    return HostTransition::scheduledTime(value);
}

} // namespace manager
} // namespace state
} // namespace phosphor
