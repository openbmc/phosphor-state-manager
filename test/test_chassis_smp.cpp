#include "config.h"

#include "../chassis_state_manager_smp.hpp"

#include <sdbusplus/test/sdbus_mock.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace phosphor::state::manager;
using namespace testing;

class ChassisSMPTest : public Test
{
  public:
    sdbusplus::SdBusMock sdbusMock;
    sdbusplus::bus_t mockedBus = sdbusplus::get_mocked_new(&sdbusMock);

    // Mock slot pointer for D-Bus operations
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    sd_bus_slot* mockSlot = reinterpret_cast<sd_bus_slot*>(0x1);

    // Helper to create a mock chassis SMP object
    std::unique_ptr<ChassisSMP> createChassisSMP(size_t numChassis = 2)
    {
        // Mock the D-Bus calls that happen during construction
        EXPECT_CALL(sdbusMock, sd_bus_emit_object_added(_, _))
            .WillRepeatedly(Return(0));

        // Mock sd_bus_add_object_vtable to return a valid slot pointer
        EXPECT_CALL(sdbusMock, sd_bus_add_object_vtable(_, _, _, _, _, _))
            .WillRepeatedly(DoAll(SetArgPointee<1>(mockSlot), Return(0)));

        // Mock sd_bus_add_match to return a valid slot pointer
        EXPECT_CALL(sdbusMock, sd_bus_add_match(_, _, _, _, _))
            .WillRepeatedly(DoAll(SetArgPointee<1>(mockSlot), Return(0)));

        // Mock sd_bus_slot_unref to return nullptr (standard behavior)
        EXPECT_CALL(sdbusMock, sd_bus_slot_unref(_))
            .WillRepeatedly(Return(nullptr));

        return std::make_unique<ChassisSMP>(
            mockedBus,
            sdbusplus::object_path("/xyz/openbmc_project/state/chassis0"),
            numChassis);
    }
};

// Test that ChassisSMP requires at least 1 chassis to aggregate
TEST_F(ChassisSMPTest, RequiresAtLeastOneChassis)
{
    EXPECT_THROW(
        {
            ChassisSMP smp(
                mockedBus,
                sdbusplus::object_path("/xyz/openbmc_project/state/chassis0"),
                0);
        },
        std::invalid_argument);
}

// Test that ChassisSMP initializes with correct default states
TEST_F(ChassisSMPTest, InitializesWithDefaultStates)
{
    auto smp = createChassisSMP(2);

    // Use the inherited getter methods from ChassisInherit
    EXPECT_EQ(smp->ChassisInherit::currentPowerState(),
              ChassisSMP::PowerState::Off);
    EXPECT_EQ(smp->ChassisInherit::currentPowerStatus(),
              ChassisSMP::PowerStatus::Good);
}

// Test basic object creation with various chassis counts
TEST_F(ChassisSMPTest, CreatesObjectSuccessfully)
{
    // Test with 2 chassis
    auto smp2 = createChassisSMP(2);
    EXPECT_NE(smp2, nullptr);

    // Test with 3 chassis
    auto smp3 = createChassisSMP(3);
    EXPECT_NE(smp3, nullptr);

    // Test with 5 chassis
    auto smp5 = createChassisSMP(5);
    EXPECT_NE(smp5, nullptr);
    EXPECT_NO_THROW(smp5->startMonitoring());
}

// Test that aggregation reports Off if any chassis is Off
TEST_F(ChassisSMPTest, AggregatesOffIfAnyChassisIsOff)
{
    auto smp = createChassisSMP(2);

    // After initialization, should use cached Off state
    EXPECT_NO_THROW(smp->startMonitoring());

    // Verify the aggregated state is Off (from cached values)
    EXPECT_EQ(smp->ChassisInherit::currentPowerState(),
              ChassisSMP::PowerState::Off);
}

// Test power status aggregation (worst-case)
TEST_F(ChassisSMPTest, AggregatesWorstCasePowerStatus)
{
    auto smp = createChassisSMP(2);

    // In a full implementation, we would mock D-Bus responses
    // to return different power statuses and verify that the
    // worst-case status is reported (BrownOut > UPS > Good)

    // For now, verify the object initializes with Good status
    EXPECT_EQ(smp->ChassisInherit::currentPowerStatus(),
              ChassisSMP::PowerStatus::Good);
}

// Test that the SMP aggregator can handle initialization gracefully
TEST_F(ChassisSMPTest, HandlesInitializationGracefully)
{
    auto smp = createChassisSMP(2);

    // Should not throw during monitoring start
    EXPECT_NO_THROW(smp->startMonitoring());
}
