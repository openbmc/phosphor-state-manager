#include "systemd_service_parser.hpp"
#include "systemd_target_parser.hpp"
#include "systemd_target_signal.hpp"

#include <CLI/CLI.hpp>
#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/bus.hpp>
#include <sdeventplus/event.hpp>

#include <iostream>
#include <vector>

PHOSPHOR_LOG2_USING;

bool gVerbose = false;

void dump_targets(const TargetErrorData& targetData)
{
    std::cout << "## Data Structure of Json ##" << std::endl;
    for (const auto& [target, value] : targetData)
    {
        std::cout << target << " " << value.errorToLog << std::endl;
        std::cout << "    ";
        for (auto& eToMonitor : value.errorsToMonitor)
        {
            std::cout << eToMonitor << ", ";
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
}

void print_usage(void)
{
    std::cout << "[-f <file1> -f <file2> ...] : Full path to json file(s) with "
                 "target/error mappings"
              << std::endl;
    std::cout << "[-s <file1> -s <file2> ...] : Full path to json file(s) with "
                 "services to monitor for errors"
              << std::endl;
    return;
}

int main(int argc, char* argv[])
{
    auto bus = sdbusplus::bus::new_default();
    auto event = sdeventplus::Event::get_default();
    bus.attach_event(event.get(), SD_EVENT_PRIORITY_NORMAL);
    std::vector<std::string> targetFilePaths;
    std::vector<std::string> serviceFilePaths;

    CLI::App app{"OpenBmc systemd target and service monitor"};
    app.add_option("-f,--file", targetFilePaths,
                   "Full path to json file(s) with target/error mappings");
    app.add_option("-s,--service", serviceFilePaths,
                   "Full path to json file(s) with services to monitor");
    app.add_flag("-v", gVerbose, "Enable verbose output");

    CLI11_PARSE(app, argc, argv);

    // target file input required
    if (targetFilePaths.empty())
    {
        error("No input files");
        print_usage();
        exit(-1);
    }

    TargetErrorData targetData = parseFiles(targetFilePaths);
    if (targetData.size() == 0)
    {
        error("Invalid input files, no targets found");
        print_usage();
        exit(-1);
    }

    ServiceMonitorData serviceData;
    if (!serviceFilePaths.empty())
    {
        serviceData = parseServiceFiles(serviceFilePaths);
    }

    if (gVerbose)
    {
        dump_targets(targetData);
    }

    phosphor::state::manager::SystemdTargetLogging targetMon(targetData, bus);

    // Subscribe to systemd D-bus signals indicating target completions
    targetMon.subscribeToSystemdSignals();

    return event.loop();
}
