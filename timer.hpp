#pragma once

#include <memory>
#include <chrono>
#include <functional>
#include <systemd/sd-event.h>

namespace phosphor
{
namespace state
{
namespace manager
{
namespace timer
{

enum Action
{
    OFF = SD_EVENT_OFF,
    ON = SD_EVENT_ON,
    ONESHOT = SD_EVENT_ONESHOT
};
}

/* Need a custom deleter for freeing up sd_event */
struct EventDeleter
{
    void operator()(sd_event* event) const
    {
        event = sd_event_unref(event);
    }
};
using EventPtr = std::unique_ptr<sd_event, EventDeleter>;
//using namespace std;

/* Need a custom deleter for freeing up sd_event_source */
struct EventSourceDeleter
{
    void operator()(sd_event_source* eventSource) const
    {
        eventSource = sd_event_source_unref(eventSource);
    }
};
using EventSourcePtr = std::unique_ptr<sd_event_source, EventSourceDeleter>;

/** @class Timer
 *  @brief Provides a timer source and a mechanism to callback when the timer
 *         expires.
 *
 *  The timer is armed upon construction. The constructor requires a timeout
 *  handler function, the timer expiry duration, and the timer state (one-shot,
 *  reptitive, disabled).
 *  It's possible to change the state of the timer after it's been armed via the
 *  state() API.
 */
class Timer
{
    public:
        Timer() = delete;
        Timer(const Timer&) = delete;
        Timer& operator=(const Timer&) = delete;
        Timer(Timer&&) = delete;
        Timer& operator=(Timer&&) = delete;

        /** @brief Constructs timer object
         *
         *  @param[in] events - sd_event pointer
         *  @param[in] callback - function callback for timer expiry
         *  @param[in] usec - timer duration, in micro seconds
         *  @param[in] action - controls the timer's lifetime
         */
        Timer(EventPtr& event, std::function<void()> userCallback,
              std::chrono::microseconds usec, timer::Action action);

        /** @brief Enables / disables the timer
         *  @param[in] action - controls the timer's lifetime
         */
        int state(timer::Action value)
        {
            action = value;
            return sd_event_source_set_enabled(eventSource.get(), action);
        }

        timer::Action getAction() const
        {
            return action;
        }

        std::chrono::microseconds getDuration() const
        {
            return duration;
        }

    private:
        /** @brief Reference to sd_event unique pointer */
        EventPtr& event;

        /** @brief Source of events */
        EventSourcePtr eventSource;

        /** @brief Optional function to call on timer expiration */
        std::function<void()> callback{};

        /** @brief Duration of the timer */
        std::chrono::microseconds duration{};

        /** @brief whether the timer is enabled/disabled/one-shot */
        timer::Action action = timer::OFF;

        /** @brief Timer expiry handler - invokes callback
        *
        *  @param[in] eventSource - Source of the event
        *  @param[in] usec        - time in micro seconds
        *  @param[in] userData    - User data pointer
        *
        */
        static int timeoutHandler(sd_event_source* eventSource, uint64_t usec,
                                  void* userData);
};
} // namespace manager
} // namespace state
} // namespace phosphor
