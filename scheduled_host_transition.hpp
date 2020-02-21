#pragma once

#include <sdbusplus/bus.hpp>
#include <phosphor-logging/log.hpp>
#include <sdeventplus/event.hpp>
#include <sdeventplus/utility/timer.hpp>
#include <xyz/openbmc_project/State/ScheduledHostTransition/server.hpp>
#include "config.h"

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
    ScheduledHostTransition(sdbusplus::bus::bus& bus, const char* objPath,
                            const sdeventplus::Event& event) :
        ScheduledHostTransitionInherit(bus, objPath, true),
        bus(bus), event(event),
        timer(event, std::bind(&ScheduledHostTransition::callback, this))
    {
    }
    ~ScheduledHostTransition() = default;

    /**
     * @brief Hanlde with scheduled time
     *
     * @param[in] value - The seconds since UTC
     * @return The updated time for transition since UTC
     **/
    uint64_t scheduledTime(uint64_t value) override;

  private:
    /** @brief sdbusplus bus client connection */
    sdbusplus::bus::bus& bus;

    /** @brief sdbusplus event */
    const sdeventplus::Event& event;

    /** @brief Get current time
     *
     *  @return - return current epoch time
     */
    std::chrono::seconds getTime();

    /** @brief Implement host transition
     *
     *  @return - Does not return anything. Error will result in exception
     *            being thrown
     */
    void hostTransition();

    /** @brief Used by the timer to do host transition */
    void callback();

    /** @brief Timer used for host transition with seconds*/
    sdeventplus::utility::Timer<sdeventplus::ClockId::Monotonic> timer;
};
} // namespace manager
} // namespace state
} // namespace phosphor
