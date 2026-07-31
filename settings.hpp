#pragma once

#include <sdbusplus/bus.hpp>
#include <xyz/openbmc_project/Control/Boot/RebootPolicy/client.hpp>
#include <xyz/openbmc_project/Control/Power/RestorePolicy/client.hpp>

#include <map>
#include <string>
#include <vector>

namespace settings
{

using Path = std::string;
using Service = std::string;
using Interface = std::string;
using Interfaces = std::vector<Interface>;
using MapperResponse = std::map<Path, std::map<Service, Interfaces>>;

constexpr auto defaultRoot = "/";
constexpr auto autoRebootIntf = sdbusplus::client::xyz::openbmc_project::
    control::boot::RebootPolicy<>::interface;
using PowerRestorePolicy =
    sdbusplus::common::xyz::openbmc_project::control::power::RestorePolicy;
constexpr auto powerRestoreIntf = PowerRestorePolicy::interface;

/** @brief Parse mapper subtree response and identify settings paths.
 *
 * @param[in] result                     - Mapper GetSubTree response
 * @param[out] autoReboot                - persistent auto_reboot path
 * @param[out] autoRebootOneTime         - one-time auto_reboot path
 * @param[out] powerRestorePolicy        - persistent power_restore_policy path
 * @param[out] powerRestorePolicyOneTime - one-time power_restore_policy path
 */
void parseMapperPaths(const MapperResponse& result, Path& autoReboot,
                      Path& autoRebootOneTime, Path& powerRestorePolicy,
                      Path& powerRestorePolicyOneTime);

/** @class Objects
 *  @brief Fetch paths of settings d-bus objects of interest, upon construction
 */
struct Objects
{
  public:
    /** @brief Constructor - fetch settings objects
     *
     * @param[in] bus  - The Dbus bus object
     * @param[in] root - The root object path
     */
    explicit Objects(sdbusplus::bus_t& bus, const Path& root = defaultRoot);
    Objects(const Objects&) = delete;
    Objects& operator=(const Objects&) = delete;
    Objects(Objects&&) = delete;
    Objects& operator=(Objects&&) = delete;
    ~Objects() = default;

    /** @brief Fetch d-bus service, given a path and an interface. The
     *         service can't be cached because mapper returns unique
     *         service names.
     *
     * @param[in] path - The Dbus object
     * @param[in] interface - The Dbus interface
     *
     * @return std::string - the dbus service name
     */
    Service service(const Path& path, const Interface& interface) const;

    /** @brief host auto_reboot user settings object */
    Path autoReboot;

    /** @brief host auto_reboot one-time settings object */
    Path autoRebootOneTime;

    /** @brief host power_restore_policy settings object */
    Path powerRestorePolicy;

    /** @brief host power_restore_policy one-time settings object */
    Path powerRestorePolicyOneTime;

    /** @brief The Dbus bus object */
    sdbusplus::bus_t& bus;
};

/** @class HostObjects
 *  @brief Fetch paths of settings d-bus objects of Host
 *  @note  IMPORTANT: This class only supports settings under the
 *         /xyz/openbmc_project/control/hostX object paths
 */
struct HostObjects : public Objects
{
  public:
    /** @brief Constructor - fetch settings objects of Host
     *
     * @param[in] bus - The Dbus bus object
     * @param[in] id  - The Host id
     */
    HostObjects(sdbusplus::bus_t& bus, size_t id);
};

} // namespace settings
