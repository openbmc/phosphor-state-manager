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

    std::string getRequestedTransitionStr(Transition trans)
    {
        return scheduledHostTransition.convertRequestedTransition(trans);
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
    scheduledHostTransition.requestedTransition(Transition::On);
    EXPECT_EQ(scheduledHostTransition.requestedTransition(), Transition::On);
    // set requested transition to be off
    scheduledHostTransition.requestedTransition(Transition::Off);
    EXPECT_EQ(scheduledHostTransition.requestedTransition(), Transition::Off);
}

TEST_F(TestScheduledHostTransition, validRequestedTransition)
{
    EXPECT_EQ(getRequestedTransitionStr(Transition::On),
              "xyz.openbmc_project.State.Host.Transition.On");
    EXPECT_EQ(getRequestedTransitionStr(Transition::Off),
              "xyz.openbmc_project.State.Host.Transition.Off");
}

TEST_F(TestScheduledHostTransition, invalidRequestedTransition)
{
    EXPECT_EQ(getRequestedTransitionStr(Transition::Reboot), "Out of range");
}

} // namespace manager
} // namespace state
} // namespace phosphor
