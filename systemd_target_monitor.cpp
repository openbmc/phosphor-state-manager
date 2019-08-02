#include <getopt.h>
#include <iostream>
#include <phosphor-logging/log.hpp>
#include <sdbusplus/bus.hpp>
#include <sdeventplus/event.hpp>
#include <set>
#include <systemd_target_parser.hpp>

using phosphor::logging::level;
using phosphor::logging::log;

bool gVerbose = false;

void dump_targets(TargetErrorData& targetData)
{
    std::cout << "## Data Structure of Json ##" << std::endl;
    for (auto& [key, value] : targetData)
    {
        std::cout << key << " " << value.errorToLog << std::endl;
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
    return;
}

int main(int argc, char* argv[])
{
    auto bus = sdbusplus::bus::new_default();
    auto event = sdeventplus::Event::get_default();
    bus.attach_event(event.get(), SD_EVENT_PRIORITY_NORMAL);

    // TODO openbmc/openbmc#3600 - CLI11 not present in SDK
    // Move to CLI11 once it works within SDK
    int arg;
    int optIndex = 0;
    std::set<std::string> filePaths;

    static struct option longOpts[] = {{"file", required_argument, 0, 'f'},
                                       {"verbose", no_argument, 0, 'v'},
                                       {"help", no_argument, 0, 'h'},
                                       {0, 0, 0, 0}};

    while ((arg = getopt_long(argc, argv, "f:hv", longOpts, &optIndex)) != -1)
    {
        switch (arg)
        {
            case 'f':
                filePaths.insert(optarg);
                break;
            case 'v':
                gVerbose = true;
                break;
            case 'h':
            default:
                print_usage();
                exit(0);
        }
    }

    if (filePaths.empty())
    {
        log<level::ERR>("No input files");
        print_usage();
        exit(-1);
    }

    TargetErrorData targetData = parseFiles(filePaths);

    if (gVerbose)
    {
        dump_targets(targetData);
    }

    if (targetData.size() == 0)
    {
        log<level::ERR>("Invalid input files, no targets found");
        print_usage();
        exit(-1);
    }

    // TODO - Begin monitoring for systemd unit changes and logging appropriate
    //        errors

    return event.loop();
}
