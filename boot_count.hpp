#pragma once
#include <systemd/sd-bus.h>
#include <sdbusplus/server.hpp>
#include <xyz/openbmc_project/Control/Boot/RebootAttempts/server.hpp>

namespace phosphor
{
namespace control
{
namespace boot
{

using Iface = sdbusplus::server::object::object<
    sdbusplus::xyz::openbmc_project::Control::Boot::server::RebootAttempts>;

/** class RebootAttempts
 *  @brief implementation of boot count
 */
class RebootAttempts : public Iface
{
    public:
        RebootAttempts() = delete;
        RebootAttempts(const RebootAttempts&) = delete;
        RebootAttempts& operator=(const RebootAttempts&) = delete;
        RebootAttempts(RebootAttempts&&) = delete;
        RebootAttempts& operator=(RebootAttempts&&) = delete;
        virtual ~RebootAttempts() = default;

        /** @brief Constructor to put object onto bus at a dbus path.
         *  @param[in] bus - Bus to attach to.
         *  @param[in] path - Path to attach at.
         */
        RebootAttempts(sdbusplus::bus::bus& bus, const char* path) :
           Iface(bus,path)
        {
        }
};


} // namespace boot
} // namespace control
} // namespace phosphor

