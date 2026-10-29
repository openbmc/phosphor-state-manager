#pragma once

#include <nlohmann/json.hpp>

#include <map>
#include <string>
#include <vector>

/** @brief Array of services to monitor for final failure via JobRemoved */
using ServiceMonitorData = std::vector<std::string>;

/** @brief Array of services to monitor via ActiveState property changes.
 *
 *  Services listed here will trigger BMC quiesce as soon as their
 *  ActiveState becomes "failed", even if systemd will restart them
 *  afterwards. This is useful when a transient crash of a critical
 *  service (e.g. ObjectMapper) already causes irreparable damage to
 *  dependent operations.
 */
using ImmediateQuiesceData = std::vector<std::string>;

/** @brief An important service to monitor for final failure.
 *
 *  Important services are monitored the same way as "services" (error log
 *  and BMC dump when systemd gives up on them) but do not quiesce the BMC.
 *  This is useful for services whose failure needs to be recorded but which
 *  the BMC can keep operating without (e.g. ipmid). Instead of quiescing,
 *  the given systemd units are started, which lets a platform choose its own
 *  recovery or notification action.
 */
struct ImportantService
{
    /** @brief Service name, may contain wildcards */
    std::string service;

    /** @brief Systemd units to start when the service fails */
    std::vector<std::string> startUnits;
};

/** @brief Array of important services to monitor for final failure */
using ImportantServiceData = std::vector<ImportantService>;

/** @brief Combined result of parsing service monitor JSON files */
struct ServiceMonitorResult
{
    ServiceMonitorData services;
    ImmediateQuiesceData immediateQuiesceServices;
    ImportantServiceData importantServices;
};

using json = nlohmann::json;

extern bool gVerbose;

/** @brief Parse input json file(s) for services to monitor
 *
 * @note This function will throw exceptions for an invalid json file
 * @note See phosphor-service-monitor-default.json for example of json file
 *       format
 * @note An optional "immediate_quiesce_services" key is also read from each
 *       json file. Files without this key are silently skipped for that part.
 * @note An optional "important_services" key is also read from each json
 *       file, in the same way as "immediate_quiesce_services". Each entry is
 *       either a service name or an object of the form
 *       {"service": "<name>", "start_units": ["<unit>", ...]}.
 *
 * @param[in] filePaths - The file(s) to parse
 *
 * @return  Combined service monitor data (services, immediateQuiesceServices
 *          and importantServices)
 */
ServiceMonitorResult parseServiceFiles(
    const std::vector<std::string>& filePaths);
