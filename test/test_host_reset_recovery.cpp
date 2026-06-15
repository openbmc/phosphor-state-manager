#include "config.h"

#include <cstdio>
#include <format>
#include <fstream>

#include <gtest/gtest.h>

#undef CHASSIS_ON_FILE
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define CHASSIS_ON_FILE "/tmp/phosphor-state-manager-test-chassis-on-{}.flag"

#define main host_reset_recovery_program_main
// NOLINTNEXTLINE(bugprone-suspicious-include)
#include "../host_reset_recovery.cpp"
#undef main

using namespace phosphor::state::manager;

TEST(HostResetRecovery, ChassisTargetCompleteWhenFileMissing)
{
    auto chassisFile = std::format(CHASSIS_ON_FILE, 0);
    std::remove(chassisFile.c_str());

    EXPECT_TRUE(isChassisTargetComplete());
}

TEST(HostResetRecovery, ChassisTargetNotCompleteWhenFileExists)
{
    auto chassisFile = std::format(CHASSIS_ON_FILE, 0);
    std::ofstream outfile(chassisFile);
    ASSERT_TRUE(outfile.good());
    outfile.close();

    EXPECT_FALSE(isChassisTargetComplete());

    std::remove(chassisFile.c_str());
}
