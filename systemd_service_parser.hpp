#pragma once

#include <nlohmann/json.hpp>

#include <map>
#include <string>
#include <vector>

/** @brief Array of services to monitor */
using ServiceMonitorData = std::vector<std::string>;

using json = nlohmann::json;

extern bool gVerbose;

/** @brief Parse input json file(s) for services to monitor
 *
 * @note This function will throw exceptions for an invalid json file
 * @note See phosphor-service-monitor-default.json for example of json file
 *       format
 *
 * @param[in] filePaths - The file(s) to parse
 *
 * @return  Service(s) to monitor for errors
 */
ServiceMonitorData parseServiceFiles(const std::vector<std::string>& filePaths);
