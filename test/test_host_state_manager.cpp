#include "config.h"

#include <string>

#include <gtest/gtest.h>

namespace phosphor::state::manager
{

class HostStateManagerTest : public testing::Test
{
  protected:
    // Helper method to validate boot count is in reasonable range
    bool isBootCountValid(uint32_t count) const
    {
        return (count > 0 && count <= 255);
    }

    // Helper method to validate class version is positive
    bool isClassVersionValid(uint32_t version) const
    {
        return (version > 0 && version <= 10);
    }

    // Helper method to validate feature flag is boolean (0 or 1)
    bool isFeatureFlagValid(int flag) const
    {
        return (flag == 0 || flag == 1);
    }

    // Helper method to validate config path contains format placeholder
    bool hasFormatPlaceholder(const std::string& path) const
    {
        return !path.empty() && path.find("{}") != std::string::npos;
    }
};

// Verify BOOT_COUNT_MAX_ALLOWED is properly configured
TEST_F(HostStateManagerTest, BootCountMaxAllowedConfigured)
{
    EXPECT_TRUE(isBootCountValid(BOOT_COUNT_MAX_ALLOWED));
    EXPECT_GT(BOOT_COUNT_MAX_ALLOWED, 0);
    EXPECT_LE(BOOT_COUNT_MAX_ALLOWED, 255);
}

// Verify CLASS_VERSION for serialization support is defined
TEST_F(HostStateManagerTest, ClassVersionConfigured)
{
    EXPECT_TRUE(isClassVersionValid(CLASS_VERSION));
    EXPECT_GT(CLASS_VERSION, 0);
}

// Verify HOST_STATE_PERSIST_PATH is properly configured
TEST_F(HostStateManagerTest, HostStatePersistPathConfigured)
{
    std::string persistPath = HOST_STATE_PERSIST_PATH;
    EXPECT_FALSE(persistPath.empty());
    EXPECT_TRUE(hasFormatPlaceholder(persistPath));
}

// Verify HOST_RUNNING_FILE path is properly configured
TEST_F(HostStateManagerTest, HostRunningFileConfigured)
{
    std::string runningFile = HOST_RUNNING_FILE;
    EXPECT_FALSE(runningFile.empty());
    EXPECT_TRUE(hasFormatPlaceholder(runningFile));
}

// Verify ENABLE_WARM_REBOOT feature flag is boolean
TEST_F(HostStateManagerTest, WarmRebootFeatureFlag)
{
    EXPECT_TRUE(isFeatureFlagValid(ENABLE_WARM_REBOOT));
}

// Verify ENABLE_FORCE_WARM_REBOOT feature flag is boolean
TEST_F(HostStateManagerTest, ForceWarmRebootFeatureFlag)
{
    EXPECT_TRUE(isFeatureFlagValid(ENABLE_FORCE_WARM_REBOOT));
}

// Verify ONLY_ALLOW_BOOT_WHEN_BMC_READY feature flag is boolean
TEST_F(HostStateManagerTest, BootWhenBmcReadyFeatureFlag)
{
    EXPECT_TRUE(isFeatureFlagValid(ONLY_ALLOW_BOOT_WHEN_BMC_READY));
}

// Verify CHECK_FWUPDATE_BEFORE_DO_TRANSITION feature flag is boolean
TEST_F(HostStateManagerTest, FirmwareUpdateCheckFeatureFlag)
{
    EXPECT_TRUE(isFeatureFlagValid(CHECK_FWUPDATE_BEFORE_DO_TRANSITION));
}

// Verify multi-chassis SMP configuration consistency
TEST_F(HostStateManagerTest, MultiChassisConfiguration)
{
    EXPECT_TRUE(isFeatureFlagValid(ENABLE_MULTI_CHASSIS_SMP));

    if (ENABLE_MULTI_CHASSIS_SMP)
    {
        EXPECT_GT(NUM_CHASSIS_SMP, 0);
    }
    else
    {
        EXPECT_EQ(NUM_CHASSIS_SMP, 0);
    }
}

} // namespace phosphor::state::manager
