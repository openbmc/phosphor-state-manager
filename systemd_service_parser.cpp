#include "systemd_service_parser.hpp"

#include <fstream>
#include <iostream>
#include <phosphor-logging/lg2.hpp>

PHOSPHOR_LOG2_USING;

ServiceMonitorResult parseServiceFiles(
    const std::vector<std::string>& filePaths)
{
    ServiceMonitorResult result;
    for (const auto& jsonFile : filePaths)
    {
        if (gVerbose)
        {
            std::cout << "Parsing input service file " << jsonFile << std::endl;
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
                std::cout << "service: " << service.value() << std::endl;
            }

            result.services.push_back(service.value());
        }

        if (j.contains("immediate_quiesce_services"))
        {
            for (const auto& service : j["immediate_quiesce_services"].items())
            {
                if (gVerbose)
                {
                    std::cout
                        << "immediate quiesce service: " << service.value()
                        << std::endl;
                }

                result.immediateQuiesceServices.push_back(service.value());
            }
        }
    }
    return result;
}
