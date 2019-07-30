#include <getopt.h>
#include <iostream>
#include <phosphor-logging/log.hpp>
#include <sdbusplus/bus.hpp>
#include <sdeventplus/event.hpp>
#include <set>

using phosphor::logging::level;
using phosphor::logging::log;

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
                                       {"help", no_argument, 0, 'h'},
                                       {0, 0, 0, 0}};

    while ((arg = getopt_long(argc, argv, "f:h", longOpts, &optIndex)) != -1)
    {
        switch (arg)
        {
            case 'f':
                filePaths.insert(optarg);
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

    // TODO - Load in json config file(s)

    // TODO - Begin monitoring for systemd unit changes and logging appropriate
    //        errors

    return event.loop();
}
