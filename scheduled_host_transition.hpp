#pragma once

#include <sdbusplus/bus.hpp>
#include <phosphor-logging/log.hpp>
#include <xyz/openbmc_project/State/ScheduledHostTransition/server.hpp>

namespace phosphor
{
namespace state
{
namespace manager
{

using Transition =
    sdbusplus::xyz::openbmc_project::State::server::Host::Transition;
using ScheduledHostTransitionInherit = sdbusplus::server::object::object<
    sdbusplus::xyz::openbmc_project::State::server::ScheduledHostTransition>;

/** @class ScheduledHostTransition
 *  @brief Scheduled host transition implementation.
 *  @details A concrete implementation for
 * xyz.openbmc_project.State.ScheduledHostTransition
 */
class ScheduledHostTransition : public ScheduledHostTransitionInherit
{
  public:
    ScheduledHostTransition(sdbusplus::bus::bus& bus, const char* objPath) :
        ScheduledHostTransitionInherit(bus, objPath, true)
    {
    }
    ~ScheduledHostTransition() = default;

    /**
     * @brief Set value of transition time
     *
     * @param[in] value - The seconds since UTC
     * @return The updated time for transition since UTC
     **/
    uint64_t scheduledTime(uint64_t value) override;

  private:
    /** @brief Get current time
     *
     *  @return - return current epoch time
     */
    std::chrono::seconds getTime();
};
} // namespace manager
} // namespace state
} // namespace phosphor
