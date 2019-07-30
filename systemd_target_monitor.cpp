#include <CLI/CLI.hpp>
#include <iostream>
#include <phosphor-logging/log.hpp>
#include <sdbusplus/bus.hpp>
#include <sdeventplus/event.hpp>
#include <vector>

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
    std::vector<std::string> filePaths;

    CLI::App app{"OpenBmc systemd target monitor"};
    app.add_option("-f,--file", filePaths,
                   "Full path to json file(s) with target/error mappings");

    CLI11_PARSE(app, argc, argv);

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
