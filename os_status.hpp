#pragma once
#include <tuple>
#include <systemd/sd-bus.h>
#include <sdbusplus/server.hpp>
#include <xyz/openbmc_project/State/OperatingSystem/Status/server.hpp>

namespace phosphor
{
namespace state
{
namespace operatingsystem
{

using Iface = sdbusplus::server::object::object<
    sdbusplus::xyz::openbmc_project::State::OperatingSystem::server::Status>;

class Status : public Iface
{
    public:
        /* Define all of the basic class operations:
         *     Not allowed:
         *         - Default constructor to avoid nullptrs.
         *         - Copy operations due to internal unique_ptr.
         *         - Move operations due to 'this' being registered as the
         *           'context' with sdbus.
         *     Allowed:
         *         - Destructor.
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

        enum class OSStatus
        {
            CBoot,
            PXEBoot,
            DiagBoot,
            CDROMBoot,
            ROMBoot,
            BootComplete,
        };

};

} // namespace OperatingSystem
} // namespace State
} // namespace phosphor

