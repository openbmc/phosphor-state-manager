#pragma once

#include <sdbusplus/bus.hpp>
#include <phosphor-logging/log.hpp>
#include <sdeventplus/event.hpp>
#include <sdeventplus/utility/timer.hpp>
#include <xyz/openbmc_project/State/ScheduledHostTransition/server.hpp>
#include <chrono>
#include "config.h"

class TestScheduledHostTransition;

namespace phosphor
{
namespace state
{
namespace manager
{

using namespace std::chrono;
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
        // We deferred this until we could get our property correct
        this->emit_object_added();
        initialize();
    }
    ~ScheduledHostTransition();

    /**
     * @brief Handle with scheduled time
     *
     * @param[in] value - The seconds since UTC
     * @return The updated time for transition since UTC
     **/
    uint64_t scheduledTime(uint64_t value) override;

  private:
    friend class TestScheduledHostTransition;

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

    /** @brief The fd for time change event */
    int timeFd = -1;

    /** @brief Initialize timerFd related resource */
    void initialize();

    /** @brief The callback function on system time change
     *
     * @param[in] es - Source of the event
     * @param[in] fd - File descriptor of the timer
     * @param[in] revents - Not used
     * @param[in] userdata - User data pointer
     */
    static int onTimeChange(sd_event_source* es, int fd, uint32_t revents,
                            void* userdata);

    /** @brief The deleter of sd_event_source */
    std::function<void(sd_event_source*)> sdEventSourceDeleter =
        [](sd_event_source* p) {
            if (p)
            {
                sd_event_source_unref(p);
            }
        };
    using SdEventSource =
        std::unique_ptr<sd_event_source, decltype(sdEventSourceDeleter)>;

    /** @brief The event source on system time change */
    SdEventSource timeChangeEventSource{nullptr, sdEventSourceDeleter};

    /** @brief Notified on bmc time is changed*/
    void onBmcTimeChanged();
};
} // namespace manager
} // namespace state
} // namespace phosphor
