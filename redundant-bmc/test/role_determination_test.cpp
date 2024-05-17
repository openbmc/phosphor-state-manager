#include "role_determination.hpp"

#include <gtest/gtest.h>

using Role =
    sdbusplus::common::xyz::openbmc_project::state::bmc::Redundancy::Role;

namespace rd = role_determination;

TEST(RoleDeterminationTest, RoleDeterminationTest)
{
    EXPECT_EQ(rd::run(), Role::Passive);
}
