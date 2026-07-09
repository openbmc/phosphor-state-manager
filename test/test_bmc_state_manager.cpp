#include "config.h"

#include "../bmc_state_manager.hpp"

#include <string>

#include <gtest/gtest.h>

namespace phosphor::state::manager
{

class BmcStateManagerTest : public testing::Test
{
  protected:
    static bool isFeatureFlagValid(int flag)
    {
        return (flag == 0 || flag == 1);
    }

    static bool isNonEmptyPath(const std::string& path)
    {
        return !path.empty();
    }

    static bool isAbsolutePath(const std::string& path)
    {
        return !path.empty() && path.front() == '/';
    }
};

TEST_F(BmcStateManagerTest, FirmwareUpdateCheckFeatureFlagIsBoolean)
{
    EXPECT_TRUE(isFeatureFlagValid(CHECK_FWUPDATE_BEFORE_DO_TRANSITION));
}

TEST_F(BmcStateManagerTest, SecureBootPathConfigured)
{
    std::string path = SYSFS_SECURE_BOOT_PATH;
    EXPECT_TRUE(isNonEmptyPath(path));
    EXPECT_TRUE(isAbsolutePath(path));
}

TEST_F(BmcStateManagerTest, AbrImagePathConfigured)
{
    std::string path = SYSFS_ABR_IMAGE_PATH;
    EXPECT_TRUE(isNonEmptyPath(path));
    EXPECT_TRUE(isAbsolutePath(path));
}

TEST_F(BmcStateManagerTest, TpmDevicePathConfigured)
{
    std::string path = SYSFS_TPM_DEVICE_PATH;
    EXPECT_TRUE(isNonEmptyPath(path));
    EXPECT_TRUE(isAbsolutePath(path));
}

TEST_F(BmcStateManagerTest, TpmMeasurementPathConfigured)
{
    std::string path = SYSFS_TPM_MEASUREMENT_PATH;
    EXPECT_TRUE(isNonEmptyPath(path));
    EXPECT_TRUE(isAbsolutePath(path));
}

TEST_F(BmcStateManagerTest, CriticalSysfsPathsAreDistinct)
{
    std::string secureBootPath = SYSFS_SECURE_BOOT_PATH;
    std::string abrImagePath = SYSFS_ABR_IMAGE_PATH;
    std::string tpmDevicePath = SYSFS_TPM_DEVICE_PATH;
    std::string tpmMeasurementPath = SYSFS_TPM_MEASUREMENT_PATH;

    EXPECT_NE(secureBootPath, abrImagePath);
    EXPECT_NE(tpmDevicePath, tpmMeasurementPath);
}

TEST_F(BmcStateManagerTest, TransitionEnumsRemainDistinct)
{
    using Transition = phosphor::state::manager::BMC::Transition;
    EXPECT_NE(Transition::Reboot, Transition::HardReboot);
}

} // namespace phosphor::state::manager
