#pragma once
#include <sdbusplus/server.hpp>
#include <xyz/openbmc_project/State/OperatingSystem/Status/server.hpp>

namespace phosphor
{
namespace state
{
namespace os
{

using Iface = sdbusplus::server::object::object<
    sdbusplus::xyz::openbmc_project::State::OperatingSystem::server::Status>;

/** class Status
 *  @brief implementation of operating system status
 */
class Status : public Iface
{
    public:
        /* Define all of the basic class operations:
         */
        Status() = delete;
        Status(const Status&) = delete;
        Status& operator=(const Status&) = delete;
        Status(Status&&) = delete;
        Status& operator=(Status&&) = delete;
        virtual ~Status() = default;

        /** @brief Constructor to put object onto bus at a dbus path.
         *  @param[in] bus - Bus to attach to.
         *  @param[in] path - Path to attach at.
         */
        Status(sdbusplus::bus::bus& bus, const char* path):Iface(bus,path)
        { }

};

} // namespace os
} // namespace state
} // namespace phosphor

