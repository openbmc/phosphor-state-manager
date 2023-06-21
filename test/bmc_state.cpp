#include "config.h"

#include <bmc_state_manager.hpp>

#include <gtest/gtest.h>

using namespace phosphor::state::manager;

TEST(rebootCause, BasicPaths)
{
    // Defaults on a normal reboot
    uint64_t bootReason = 0;
    int phrGpioVal = 1;
    bool chassisACLoss = false;

    auto rebootReason = phosphor::state::manager::BMC::getRebootCause(
        bootReason, phrGpioVal, chassisACLoss);
    EXPECT_EQ(rebootReason, BMC::RebootCause::Unknown);

    // WDIOF_EXTERN1
    bootReason = WDIOF_EXTERN1;
    phrGpioVal = 1;
    chassisACLoss = false;

    rebootReason = phosphor::state::manager::BMC::getRebootCause(
        bootReason, phrGpioVal, chassisACLoss);
    EXPECT_EQ(rebootReason, BMC::RebootCause::Watchdog);

    // WDIOF_CARDRESET
    bootReason = WDIOF_CARDRESET;
    phrGpioVal = 1;
    chassisACLoss = false;

    rebootReason = phosphor::state::manager::BMC::getRebootCause(
        bootReason, phrGpioVal, chassisACLoss);
    EXPECT_EQ(rebootReason, BMC::RebootCause::POR);

    // Pinhole Reset
    bootReason = 0;
    phrGpioVal = 0;
    chassisACLoss = false;

    rebootReason = phosphor::state::manager::BMC::getRebootCause(
        bootReason, phrGpioVal, chassisACLoss);
    EXPECT_EQ(rebootReason, BMC::RebootCause::PinholeReset);

    // AC Loss
    bootReason = 0;
    phrGpioVal = 1;
    chassisACLoss = true;

    rebootReason = phosphor::state::manager::BMC::getRebootCause(
        bootReason, phrGpioVal, chassisACLoss);
    EXPECT_EQ(rebootReason, BMC::RebootCause::POR);
}

TEST(rebootCause, ComplexPaths)
{
    // Pinhole set but also AC Loss
    uint64_t bootReason = 0;
    int phrGpioVal = 0;
    bool chassisACLoss = true;

    auto rebootReason = phosphor::state::manager::BMC::getRebootCause(
        bootReason, phrGpioVal, chassisACLoss);
    EXPECT_EQ(rebootReason, BMC::RebootCause::PinholeReset);
}
