#include "systemd_service_parser.hpp"

#include <phosphor-logging/lg2.hpp>

#include <fstream>

PHOSPHOR_LOG2_USING;

ServiceMonitorResult parseServiceFiles(
    const std::vector<std::string>& filePaths)
{
    ServiceMonitorResult result;
    for (const auto& jsonFile : filePaths)
    {
        if (gVerbose)
        {
            debug("Parsing input service file {FILE}", "FILE", jsonFile);
        }
        std::ifstream fileStream(jsonFile);
        if (!fileStream.is_open())
        {
            error("Failed to open service monitor file: {FILE}", "FILE",
                  jsonFile);
            continue;
        }
        auto j = json::parse(fileStream);

        for (const auto& service : j["services"].items())
        {
            if (gVerbose)
            {
                debug("service: {SERVICE}", "SERVICE", service.value());
            }

            result.services.push_back(service.value());
        }

        if (j.contains("immediate_quiesce_services"))
        {
            for (const auto& service : j["immediate_quiesce_services"].items())
            {
                if (gVerbose)
                {
                    debug("immediate quiesce service: {SERVICE}", "SERVICE",
                          service.value());
                }

                result.immediateQuiesceServices.push_back(service.value());
            }
        }
    }
    return result;
}
