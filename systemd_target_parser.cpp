#include <cassert>
#include <fstream>
#include <iostream>
#include <systemd_target_parser.hpp>

void validateErrosToMonitor(std::vector<std::string>& errorsToMonitor)
{
    assert(errorsToMonitor.size() > 0);

    const std::vector<std::string> validErrorsToMonitor = {
        "default", "timeout", "failed", "dependency"};
    for (auto errorToMonitor : errorsToMonitor)
    {
        if (std::find(validErrorsToMonitor.begin(), validErrorsToMonitor.end(),
                      errorToMonitor) == validErrorsToMonitor.end())
        {
            throw std::out_of_range("Found invalid error to monitor");
        }
    }
    // See if default was in the errors to monitor, if so replace with defaults
    auto errorItr =
        std::find(errorsToMonitor.begin(), errorsToMonitor.end(), "default");
    if (errorItr != errorsToMonitor.end())
    {
        // delete "default" and insert defaults
        errorsToMonitor.erase(errorItr);
        errorsToMonitor.push_back("timeout");
        errorsToMonitor.push_back("failed");
        errorsToMonitor.push_back("dependency");
    }
}

TargetErrorData parseFiles(const std::set<std::string>& filePaths)
{
    TargetErrorData systemdTargetMap;
    for (auto jsonFile : filePaths)
    {
        if (gVerbose)
        {
            std::cout << "Parsing input file " << jsonFile << std::endl;
        }
        std::ifstream fileStream(jsonFile);
        json j = json::parse(fileStream);

        for (auto it = j["targets"].begin(); it != j["targets"].end(); ++it)
        {
            targetEntry entry;
            if (gVerbose)
            {
                std::cout << "target: " << it.key() << " | " << it.value()
                          << std::endl;
            }

            // Be unforgiving on invalid json files. Just throw or allow
            // nlohmann to throw an exception if something is off
            auto errorsToMonitor = it.value().find("errorsToMonitor");
            entry.errorsToMonitor =
                errorsToMonitor->get<std::vector<std::string>>();

            validateErrosToMonitor(entry.errorsToMonitor);

            auto errorToLog = it.value().find("errorToLog");
            entry.errorToLog = errorToLog->get<std::string>();

            systemdTargetMap[it.key()] = entry;
        }
    }
    return systemdTargetMap;
}
