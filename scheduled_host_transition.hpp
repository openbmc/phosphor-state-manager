#pragma once

#include <sdbusplus/bus.hpp>
#include <phosphor-logging/log.hpp>
#include <xyz/openbmc_project/State/ScheduledHostTransition/server.hpp>

class TestScheduledHostTransition;

namespace phosphor
{
namespace state
{
namespace manager
{

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
        // We deferred this until we could get our property correct
        this->emit_object_added();
    }
    ~ScheduledHostTransition() = default;

    /**
     * @brief Handle with scheduled time
     *
     * @param[in] value - The seconds since UTC
     * @return The updated time for transition since UTC
     **/
    uint64_t scheduledTime(uint64_t value) override;

  private:
    friend class TestScheduledHostTransition;
    /** @brief Get current time
     *
     *  @return - return current epoch time
     */
    std::chrono::seconds getTime();
};
} // namespace manager
} // namespace state
} // namespace phosphor
