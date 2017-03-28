#include <cstdlib>
#include <unistd.h>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/match.hpp>
#include <phosphor-logging/log.hpp>
#include <xyz/openbmc_project/Control/Host/server.hpp>

using namespace phosphor::logging;
using namespace sdbusplus::xyz::openbmc_project::Control::server;


bool cmdDone     = false;
bool hostRunning = false;

// Function called on host control signals
static int hostControlSignal(sd_bus_message* msg,
                             void* userData,
                             sd_bus_error* retError)
{
    std::string cmdCompleted{};
    std::string cmdStatus{};

    auto sdPlusMsg = sdbusplus::message::message(msg);
    sdPlusMsg.read(cmdCompleted, cmdStatus);

    log<level::DEBUG>("Host control signal values",
            entry("COMMAND=%s",cmdCompleted.c_str()),
            entry("STATUS=%s",cmdStatus.c_str()));

    // Verify it's the command this code is interested in and then check status
    if(Host::convertCommandFromString(cmdCompleted) == Host::Command::Heartbeat)
    {
        cmdDone = true;

        if(Host::convertResultFromString(cmdStatus) == Host::Result::Success)
        {
            hostRunning = true;
        }
    }

    return 0;
}

int main(int argc, char *argv[])
{
    log<level::INFO>("Check if host is running");

    auto bus = sdbusplus::bus::new_default();

    // Setup Signal Handler
    sdbusplus::server::match::match hostControlSignals(bus,
                               "type='signal',"
                               "member='CommandComplete',"
                               "path='/xyz/openbmc_project/control/host0',"
                               "interface='xyz.openbmc_project.Control.Host'",
                               hostControlSignal,
                               nullptr);

    // TODO Initiate heartbeat command

    // Wait for signal
    while(!cmdDone)
    {
        bus.process_discard();
        if (cmdDone) break;
        bus.wait();
    }

    // If host running then create file
    if(hostRunning)
    {
        // TODO - Add file creation
        log<level::INFO>("Host is running!");
    }

    return 0;
}
