#include "manager.hpp"

#include <sdbusplus/async.hpp>

int main()
{
    sdbusplus::async::context ctx;
    sdbusplus::server::manager_t objMgr{
        ctx, bmc::redundancy::RedundancyInterface::instance_path};

    bmc::redundancy::Manager manager{ctx};

    ctx.spawn([](sdbusplus::async::context& ctx) -> sdbusplus::async::task<> {
        ctx.request_name(bmc::redundancy::RedundancyInterface::default_service);
        co_return;
    }(ctx));

    ctx.run();

    return 0;
}
