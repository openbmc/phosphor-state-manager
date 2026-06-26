#include "systemd_service_parser.hpp"
#include "systemd_target_parser.hpp"
#include "systemd_target_signal.hpp"

#include <CLI/CLI.hpp>
#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/bus.hpp>

#include <vector>

PHOSPHOR_LOG2_USING;

bool gVerbose = false;

void dump_targets(const TargetErrorData& targetData)
{
    debug("## Data Structure of Json ##");
    for (const auto& [target, value] : targetData)
    {
        debug("{TARGET} {ERROR_TO_LOG}", "TARGET", target, "ERROR_TO_LOG",
              value.errorToLog);
        for (const auto& eToMonitor : value.errorsToMonitor)
        {
            debug("{ERROR_TO_MONITOR}", "ERROR_TO_MONITOR", eToMonitor);
        }
    }
}

int main(int argc, char* argv[])
{
    auto bus = sdbusplus::bus::new_default();
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
        exit(-1);
    }

    TargetErrorData targetData = parseFiles(targetFilePaths);
    if (targetData.size() == 0)
    {
        error("Invalid input files, no targets found");
        exit(-1);
    }

    ServiceMonitorResult serviceResult;
    if (!serviceFilePaths.empty())
    {
        serviceResult = parseServiceFiles(serviceFilePaths);
    }

    if (gVerbose)
    {
        dump_targets(targetData);
    }

    phosphor::state::manager::SystemdTargetLogging targetMon(
        targetData, serviceResult.services,
        serviceResult.immediateQuiesceServices, bus);

    // Subscribe to systemd D-bus signals indicating target completions
    targetMon.subscribeToSystemdSignals();

    bus.process_loop();
    return 0;
}
