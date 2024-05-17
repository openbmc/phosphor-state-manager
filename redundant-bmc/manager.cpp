#include "manager.hpp"

#include "role_determination.hpp"

#include <phosphor-logging/lg2.hpp>

namespace bmc::redundancy
{

Manager::Manager(sdbusplus::async::context& ctx) :
    ctx(ctx),
    redundancyInterface(ctx.get_bus(), RedundancyInterface::instance_path)
{
    ctx.spawn(startup());
}

sdbusplus::async::task<> Manager::startup()
{
    determineRole();
    co_return;
}

void Manager::determineRole()
{
    auto role = role_determination::run();

    lg2::info("Role Determined: {ROLE}", "ROLE", role);

    redundancyInterface.role(role);
}

} // namespace bmc::redundancy
