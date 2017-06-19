#pragma once
#include <tuple>
#include <systemd/sd-bus.h>
#include <sdbusplus/server.hpp>
#include <xyz/openbmc_project/State/Boot/Progress/server.hpp>

namespace phosphor
{
namespace state
{
namespace boot
{

using Iface = sdbusplus::server::object::object<
    sdbusplus::xyz::openbmc_project::State::Boot::server::Progress>;

/** class Progress
 *  @brief implementation of boot progress
 */
class Progress : public Iface
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
        Progress() = delete;
        Progress(const Progress&) = delete;
        Progress& operator=(const Progress&) = delete;
        Progress(Progress&&) = delete;
        Progress& operator=(Progress&&) = delete;
        virtual ~Progress() = default;

        /** @brief Constructor to put object onto bus at a dbus path.
         *  @param[in] bus - Bus to attach to.
         *  @param[in] path - Path to attach at.
         */
        Progress(sdbusplus::bus::bus& bus, const char* path) : Iface(bus,path)
        { }

        enum class ProgressStages
        {
            Unspecified,
            MemoryInit,
            SecondaryProcInit,
            OSStart,
            MotherboardInit,
        };
};

} // namespace Boot
} // namespace State
} // namespace phosphor

