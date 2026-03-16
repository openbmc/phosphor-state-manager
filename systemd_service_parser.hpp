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
using StateChangeMonitorData = std::vector<std::string>;

/** @brief Combined result of parsing service monitor JSON files */
struct ServiceMonitorResult
{
    ServiceMonitorData services;
    StateChangeMonitorData stateChangeServices;
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
 *
 * @param[in] filePaths - The file(s) to parse
 *
 * @return  Combined service monitor data (services + stateChangeServices)
 */
ServiceMonitorResult parseServiceFiles(
    const std::vector<std::string>& filePaths);
