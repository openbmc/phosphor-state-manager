#pragma once

#include <map>
#include <set>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

struct targetEntry
{
    std::string errorToLog;
    std::vector<std::string> errorsToMonitor;
};

using TargetErrorData = std::map<std::string, targetEntry>;

using json = nlohmann::json;

extern bool gVerbose;

/** @brief Parse input json files
 *
 * Will return the parsed data in the TargetErrorData object
 *
 * @note This function will throw exceptions for an invalid json file
 * @note See test/systemd_parser.cpp for example of json file format
 *
 * @param[in] filePaths - The file(s) to parse
 *
 * @return Map of target to error log relationships
 */
TargetErrorData parseFiles(const std::set<std::string>& filePaths);
