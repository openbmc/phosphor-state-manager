#include "scheduled_host_transition.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/test/sdbus_mock.hpp>
#include <sdeventplus/event.hpp>
#include <xyz/openbmc_project/ScheduledTime/error.hpp>

namespace phosphor
{
namespace state
{
namespace manager
{

using namespace std::chrono;
using InvalidTimeError =
    sdbusplus::xyz::openbmc_project::ScheduledTime::Error::InvalidTime;

class TestScheduledHostTransition : public testing::Test
{
  public:
    sdeventplus::Event event;
    sdbusplus::SdBusMock sdbusMock;
    sdbusplus::bus::bus mockedBus = sdbusplus::get_mocked_new(&sdbusMock);
    ScheduledHostTransition scheduledHostTransition;

    TestScheduledHostTransition() :
        event(sdeventplus::Event::get_default()),
        scheduledHostTransition(mockedBus, "", event)
    {
        // Empty
    }

    seconds getCurrentTime()
    {
        return scheduledHostTransition.getTime();
    }

    bool isTimerEnabled()
    {
        return scheduledHostTransition.timer.isEnabled();
    }
};

TEST_F(TestScheduledHostTransition, disableHostTransition)
{
    EXPECT_EQ(scheduledHostTransition.scheduledTime(0), 0);
    EXPECT_FALSE(isTimerEnabled());
}

TEST_F(TestScheduledHostTransition, invalidScheduledTime)
{
    // scheduled time is 1 min earlier than current time
    uint64_t schTime =
        static_cast<uint64_t>((getCurrentTime() - seconds(60)).count());
    EXPECT_THROW(scheduledHostTransition.scheduledTime(schTime),
                 InvalidTimeError);
}

TEST_F(TestScheduledHostTransition, validScheduledTime)
{
    // scheduled time is 1 min later than current time
    uint64_t schTime =
        static_cast<uint64_t>((getCurrentTime() + seconds(60)).count());
    EXPECT_EQ(scheduledHostTransition.scheduledTime(schTime), schTime);
    EXPECT_TRUE(isTimerEnabled());
}

TEST_F(TestScheduledHostTransition, hostTransitionStatus)
{
    // set requested transition to be on
    scheduledHostTransition.scheduledTransition(Transition::On);
    EXPECT_EQ(scheduledHostTransition.scheduledTransition(), Transition::On);
    // set requested transition to be off
    scheduledHostTransition.scheduledTransition(Transition::Off);
    EXPECT_EQ(scheduledHostTransition.scheduledTransition(), Transition::Off);
}

} // namespace manager
} // namespace state
} // namespace phosphor
