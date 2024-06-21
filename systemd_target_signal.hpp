#pragma once

#include "systemd_service_parser.hpp"
#include "systemd_target_parser.hpp"

#include <sdbusplus/bus.hpp>
#include <sdbusplus/bus/match.hpp>

extern bool gVerbose;

namespace phosphor
{
namespace state
{
namespace manager
{
/** @class SystemdTargetLogging
 *  @brief Object to monitor input systemd targets and create corresponding
 *         input errors for on failures
 */
class SystemdTargetLogging
{
  public:
    SystemdTargetLogging() = delete;
    SystemdTargetLogging(const SystemdTargetLogging&) = delete;
    SystemdTargetLogging& operator=(const SystemdTargetLogging&) = delete;
    SystemdTargetLogging(SystemdTargetLogging&&) = delete;
    SystemdTargetLogging& operator=(SystemdTargetLogging&&) = delete;
    virtual ~SystemdTargetLogging() = default;

    SystemdTargetLogging(const TargetErrorData& targetData,
                         const ServiceMonitorData& serviceData,
                         sdbusplus::bus_t& bus) :
        targetData(targetData),
        serviceData(serviceData), bus(bus),
        systemdJobRemovedSignal(
            bus,
            sdbusplus::bus::match::rules::type::signal() +
                sdbusplus::bus::match::rules::member("JobRemoved") +
                sdbusplus::bus::match::rules::path(
                    "/org/freedesktop/systemd1") +
                sdbusplus::bus::match::rules::interface(
                    "org.freedesktop.systemd1.Manager"),
            [this](sdbusplus::message_t& m) { systemdUnitChange(m); }),
        systemdNameOwnedChangedSignal(
            bus, sdbusplus::bus::match::rules::nameOwnerChanged(),
            [this](sdbusplus::message_t& m) { processNameChangeSignal(m); })
    {}

    /**
     * @brief subscribe to the systemd signals
     *
     * This object needs to monitor systemd target changes so it can create
     * the required error logs on failures
     *
     **/
    void subscribeToSystemdSignals();

    /** @brief Process the target fail and return error to log
     *
     * @note This is public for unit testing purposes
     *
     * @param[in]  unit       - The systemd unit that failed
     * @param[in]  result     - The failure code from the system unit
     *
     * @return valid pointer to error to log, otherwise nullptr
     */
    std::string processError(const std::string& unit,
                             const std::string& result);

  private:
    /** @brief Start BMC Quiesce Target to indicate critical service failure */
    void startBmcQuiesceTarget();

    /** @brief Call phosphor-logging to create error
     *
     * @param[in]  error      - The error to log
     * @param[in]  result     - The failure code from the systemd unit
     * @param[in]  unit       - The name of the failed unit
     */
    void logError(const std::string& error, const std::string& result,
                  const std::string& unit);

    /** @brief Check if systemd state change is one to monitor
     *
     * Instance specific interface to handle the detected systemd state
     * change
     *
     * @param[in]  msg       - Data associated with subscribed signal
     *
     */
    void systemdUnitChange(sdbusplus::message_t& msg);

    /** @brief Wait for systemd to show up on dbus
     *
     * Once systemd is on dbus, this application can subscribe to systemd
     * signal changes
     *
     * @param[in]  msg       - Data associated with subscribed signal
     *
     */
    void processNameChangeSignal(sdbusplus::message_t& msg);

    /** @brief Systemd targets to monitor and error logs to create */
    const TargetErrorData& targetData;

    /** @brief Systemd targets to monitor and error logs to create */
    const ServiceMonitorData& serviceData;

    /** @brief Persistent sdbusplus DBus bus connection. */
    sdbusplus::bus_t& bus;

    /** @brief Used to subscribe to dbus systemd JobRemoved signals **/
    sdbusplus::bus::match_t systemdJobRemovedSignal;

    /** @brief Used to know when systemd has registered on dbus **/
    sdbusplus::bus::match_t systemdNameOwnedChangedSignal;
};

} // namespace manager
} // namespace state
} // namespace phosphor
