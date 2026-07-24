#pragma once

#include "config.h"

#include <sdbusplus/bus.hpp>
#include <xyz/openbmc_project/Logging/Entry/server.hpp>

constexpr auto PROPERTY_INTERFACE = "org.freedesktop.DBus.Properties";

constexpr auto SYSTEMD_SERVICE = "org.freedesktop.systemd1";
constexpr auto SYSTEMD_OBJ_PATH = "/org/freedesktop/systemd1";
constexpr auto SYSTEMD_MANAGER_INTERFACE = "org.freedesktop.systemd1.Manager";
constexpr auto SYSTEMD_UNIT_INTERFACE = "org.freedesktop.systemd1.Unit";

namespace phosphor::state::manager::utils
{

/** @brief Tell systemd to generate d-bus events
 *
 * @param[in] bus          - The Dbus bus object
 *
 * @return void, will throw exception on failure
 */
void subscribeToSystemdSignals(sdbusplus::bus_t& bus);

/** @brief Get service name from object path and interface
 *
 * @param[in] bus          - The Dbus bus object
 * @param[in] path         - The Dbus object path
 * @param[in] interface    - The Dbus interface
 *
 * @return The name of the service
 */
std::string getService(sdbusplus::bus_t& bus,
                       const sdbusplus::object_path& path,
                       std::string interface);

/** @brief Get the value of input property
 *
 * @param[in] bus          - The Dbus bus object
 * @param[in] path         - The Dbus object path
 * @param[in] interface    - The Dbus interface
 * @param[in] property     - The property name to get
 *
 * @return The value of the property
 */
std::string getProperty(sdbusplus::bus_t& bus, const std::string& path,
                        const std::string& interface,
                        const std::string& propertyName);

/** @brief Set the value of a property
 *
 * @param[in] bus          - The Dbus bus object
 * @param[in] path         - The Dbus object path
 * @param[in] interface    - The Dbus interface
 * @param[in] property     - The property name to set
 * @param[in] value        - The value of property
 */
void setProperty(sdbusplus::bus_t& bus, const std::string& path,
                 const std::string& interface, const std::string& property,
                 const std::variant<bool, std::string>& value);

/** @brief Return the value of the input GPIO
 *
 * @param[in] gpioName          - The name of the GPIO to read
 *
 *  * @return The value of the gpio (0 or 1) or -1 on error
 */
int getGpioValue(const std::string& gpioName);

/** @brief Create an error log
 *
 * @param[in] bus            - The Dbus bus object
 * @param[in] errorMsg       - The error message
 * @param[in] errLevel       - The error level
 * @param[in] additionalData - Optional extra data to add to the log
 *
 * @return The D-Bus object path of the created log entry
 */
sdbusplus::object_path createError(
    sdbusplus::bus_t& bus, const std::string& errorMsg,
    sdbusplus::server::xyz::openbmc_project::logging::Entry::Level errLevel,
    std::map<std::string, std::string> additionalData = {});

/** @brief Call phosphor-dump-manager to create BMC dump linked to an error log
 *
 * @param[in] bus        - The Dbus bus object
 * @param[in] objectPath - D-Bus object path of the associated error log entry
 *                         (e.g. /xyz/openbmc_project/logging/entry/5).
 *                         Passed as EventId to CreateDump so dreport can
 *                         collect the dump details inside the dump archive.
 *                         Pass an empty string if there is no associated entry.
 */
void createBmcDump(sdbusplus::bus_t& bus,
                   const sdbusplus::object_path& objectPath);

/** @brief Attempt to locate the obmc-chassis-lost-power@ file
 *    to indicate that an AC loss occurred.
 *
 * @param[in] chassisId  - the chassis instance
 */
bool checkACLoss(size_t& chassisId);

/** @brief Determine if the BMC is at its Ready state
 *
 * @param[in] bus          - The Dbus bus object
 */
bool isBmcReady(sdbusplus::bus_t& bus);

/** @brief Wait BMC to enter ready state or timeout reached.
 *
 * @param[in] bus          - The Dbus bus object
 * @param[in] timeout      - Timeout in second
 */
bool waitBmcReady(sdbusplus::bus_t& bus, std::chrono::seconds timeout);

/** @brief Determine if any firmware being updated
 *
 * @param[in] bus          - The Dbus bus object
 */
bool isFirmwareUpdating(sdbusplus::bus_t& bus);

/** @brief Determine if a systemd unit is active or activating
 *
 * @param[in] bus          - The Dbus bus object
 * @param[in] target       - The systemd unit name
 *
 * @return true when the unit ActiveState is active or activating
 */
bool stateActive(sdbusplus::bus_t& bus, const std::string& target);

} // namespace phosphor::state::manager::utils
