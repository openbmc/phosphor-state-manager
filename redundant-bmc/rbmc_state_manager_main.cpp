#include <sdbusplus/async.hpp>
#include <sdbusplus/server.hpp>

int main()
{
    sdbusplus::async::context ctx;
    sdbusplus::server::manager_t objMgr{ctx, "/xyz/openbmc_project"};

    ctx.spawn([](sdbusplus::async::context& ctx) -> sdbusplus::async::task<> {
        ctx.request_name("xyz.openbmc_project.Test");
        co_return;
    }(ctx));

    ctx.run();

    return 0;
}
