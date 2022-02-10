#include "systemd_service_parser.hpp"

#include <fstream>
#include <iostream>

ServiceMonitorData parseServiceFiles(const std::vector<std::string>& filePaths)
{
    ServiceMonitorData systemdServiceMap;
    for (const auto& jsonFile : filePaths)
    {
        if (gVerbose)
        {
            std::cout << "Parsing input service file " << jsonFile << std::endl;
        }
        std::ifstream fileStream(jsonFile);
        auto j = json::parse(fileStream);

        for (auto& service : j["services"].items())
        {
            if (gVerbose)
            {
                std::cout << "service: " << service.value() << std::endl;
            }

            systemdServiceMap.push_back(service.value());
        }
    }
    return systemdServiceMap;
}
