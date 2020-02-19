#include "scheduled_host_transition.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/test/sdbus_mock.hpp>
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

class TestScheduledHostTransition : public testing::Test
{
  public:
    sdbusplus::SdBusMock sdbusMock;
    sdbusplus::bus::bus mockedBus = sdbusplus::get_mocked_new(&sdbusMock);
    ScheduledHostTransition scheduledHostTransition;

    TestScheduledHostTransition() : scheduledHostTransition(mockedBus, "")
    {
        // Empty
    }

    seconds getCurrentTime()
    {
        return scheduledHostTransition.getTime();
    }
};

TEST_F(TestScheduledHostTransition, disableHostTransition)
{
    EXPECT_EQ(scheduledHostTransition.scheduledTime(0), 0);
}

TEST_F(TestScheduledHostTransition, invalidScheduledTime)
{
    // scheduled time is 1 min earlier than current time
    uint64_t schTime = (uint64_t)(getCurrentTime() - seconds(60)).count();
    EXPECT_THROW(scheduledHostTransition.scheduledTime(schTime), FailedError);
}

} // namespace manager
} // namespace state
} // namespace phosphor
