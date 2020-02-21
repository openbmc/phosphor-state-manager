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
using FailedError =
    sdbusplus::xyz::openbmc_project::ScheduledTime::Error::InvalidTime;
using HostTransition =
    sdbusplus::xyz::openbmc_project::State::server::ScheduledHostTransition;

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

    bool getTimerStatus()
    {
        return scheduledHostTransition.timer.isEnabled();
    }

    void hostTransition()
    {
        scheduledHostTransition.hostTransition();
    }
};

TEST_F(TestScheduledHostTransition, disableHostTransition)
{
    EXPECT_EQ(scheduledHostTransition.scheduledTime(0), 0);
    EXPECT_FALSE(getTimerStatus());
}

TEST_F(TestScheduledHostTransition, invalidScheduledTime)
{
    // scheduled time is 1 min earlier than current time
    uint64_t schTime = (uint64_t)(getCurrentTime() - seconds(60)).count();
    EXPECT_THROW(scheduledHostTransition.scheduledTime(schTime), FailedError);
}

TEST_F(TestScheduledHostTransition, validScheduledTime)
{
    // scheduled time is 1 min later than current time
    uint64_t schTime = (uint64_t)(getCurrentTime() + seconds(60)).count();
    EXPECT_EQ(scheduledHostTransition.scheduledTime(schTime), 60ULL);
    EXPECT_TRUE(getTimerStatus());
}

TEST_F(TestScheduledHostTransition, hostTransitionStatus)
{
    // set requested transition to be on
    scheduledHostTransition.requestedTransition(Transition::On);
    EXPECT_EQ(scheduledHostTransition.requestedTransition(), Transition::On);
    // set requested transition to be off
    scheduledHostTransition.requestedTransition(Transition::Off);
    EXPECT_EQ(scheduledHostTransition.requestedTransition(), Transition::Off);
}

} // namespace manager
} // namespace state
} // namespace phosphor
