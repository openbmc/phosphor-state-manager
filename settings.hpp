#pragma once

#include <sdbusplus/bus.hpp>

#include <string>

namespace settings
{

using Path = std::string;
using Service = std::string;
using Interface = std::string;

constexpr auto defaultRoot = "/";
constexpr auto autoRebootIntf = "xyz.openbmc_project.Control.Boot.RebootPolicy";
constexpr auto powerRestoreIntf =
    "xyz.openbmc_project.Control.Power.RestorePolicy";

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
    explicit Objects(sdbusplus::bus::bus& bus, const Path& root = defaultRoot);
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
    sdbusplus::bus::bus& bus;
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
    HostObjects(sdbusplus::bus::bus& bus, size_t id);
};

} // namespace settings
