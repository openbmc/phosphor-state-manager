#pragma once

#include <xyz/openbmc_project/State/BMC/Redundancy/common.hpp>

namespace role_determination
{

/**
 * @brief Determines if this BMC should claim the Active or Passive role.
 *
 * @return The role
 */
sdbusplus::common::xyz::openbmc_project::state::bmc::Redundancy::Role run();

} // namespace role_determination
