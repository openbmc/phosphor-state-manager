#include "role_determination.hpp"

namespace role_determination
{
using Redundancy =
    sdbusplus::common::xyz::openbmc_project::state::bmc::Redundancy;

Redundancy::Role run()
{
    using enum Redundancy::Role;
    return Passive;
}

} // namespace role_determination
