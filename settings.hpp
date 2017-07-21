#pragma once

#include <string>
#include <utility>
#include <sdbusplus/bus.hpp>

namespace settings
{

constexpr auto root = "/";
constexpr auto autoRebootIntf =
    "xyz.openbmc_project.Control.Boot.RebootPolicy";
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
         * @param[in] bus - The Dbus bus object
         */
        Objects(sdbusplus::bus::bus& bus);
        Objects(const Objects&) = delete;
        Objects& operator=(const Objects&) = delete;
        Objects(Objects&&) = delete;
        Objects& operator=(Objects&&) = delete;
        ~Objects() = default;

        using Path = std::string;
        using Service = std::string;

        /** @brief host auto_reboot settings object */
        std::pair<Path, Service> autoReboot;

        /** @brief host power_restore_policy settings object */
        std::pair<Path, Service> powerRestorePolicy;
};

} // namespace settings
