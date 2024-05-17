#pragma once
#include <sdbusplus/async/context.hpp>
#include <xyz/openbmc_project/State/BMC/Redundancy/server.hpp>

namespace bmc::redundancy
{

using RedundancyIntf =
    sdbusplus::xyz::openbmc_project::State::BMC::server::Redundancy;
using RedundancyInterface = sdbusplus::server::object_t<RedundancyIntf>;

class Manager
{
  public:
    ~Manager() = default;
    Manager(const Manager&) = delete;
    Manager& operator=(const Manager&) = delete;
    Manager(Manager&&) = delete;
    Manager& operator=(Manager&&) = delete;

    /**
     * @brief Constructor
     *
     * @param cts - The async context object
     */
    explicit Manager(sdbusplus::async::context& ctx);

  private:
    /**
     * @brief Kicks off the Manager startup as a coroutine
     */
    sdbusplus::async::task<> startup();

    /**
     * @brief Determines the BMC role, active or passive
     */
    void determineRole();

    /**
     * @brief The async context object
     */
    sdbusplus::async::context& ctx;

    /**
     * @brief The Redundancy D-Bus interface
     */
    RedundancyInterface redundancyInterface;
};

} // namespace bmc::redundancy
