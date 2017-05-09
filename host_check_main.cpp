#include <cstdlib>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <cstdio>
#include <sdbusplus/bus.hpp>
#include <phosphor-logging/log.hpp>
#include <xyz/openbmc_project/Control/Host/server.hpp>
#include <config.h>

using namespace std::literals;
using namespace phosphor::logging;
using namespace sdbusplus::xyz::openbmc_project::Control::server;

// Required strings for sending the msg to check on host
constexpr auto MAPPER_BUSNAME = "xyz.openbmc_project.ObjectMapper";
constexpr auto MAPPER_PATH = "/xyz/openbmc_project/object_mapper";
constexpr auto MAPPER_INTERFACE = "xyz.openbmc_project.ObjectMapper";
constexpr auto CONTROL_HOST_PATH = "/xyz/openbmc_project/control/host0";
constexpr auto CONTROL_HOST_INTERFACE = "xyz.openbmc_project.Control.Host";

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

// Send hearbeat to host to determine if it's running
void sendHeartbeat(sdbusplus::bus::bus& bus)
{
    auto mapper = bus.new_method_call(MAPPER_BUSNAME,
                                      MAPPER_PATH,
                                      MAPPER_INTERFACE,
                                      "GetObject");

    mapper.append(CONTROL_HOST_PATH,
                  std::vector<std::string>({CONTROL_HOST_INTERFACE}));
    auto mapperResponseMsg = bus.call(mapper);

    if (mapperResponseMsg.is_method_error())
    {
        log<level::ERR>("Error in mapper call for control host");
        // TODO openbmc/openbmc#851 - Once available, throw returned error
        throw std::runtime_error("Error in mapper call for control host");
    }

    std::map<std::string, std::vector<std::string>> mapperResponse;
    mapperResponseMsg.read(mapperResponse);
    if (mapperResponse.empty())
    {
        log<level::ERR>("Error reading mapper resp for control host");
        // TODO openbmc/openbmc#851 - Once available, throw returned error
        throw std::runtime_error("Error reading mapper resp for control host");
    }

    const auto& host = mapperResponse.begin()->first;

    auto method = bus.new_method_call(host.c_str(),
                                      CONTROL_HOST_PATH,
                                      CONTROL_HOST_INTERFACE,
                                      "Execute");
    method.append(convertForMessage(Host::Command::Heartbeat).c_str());

    auto reply = bus.call(method);
    if (reply.is_method_error())
    {
        log<level::ERR>("Error in call to control host Execute");
        throw std::runtime_error("Error in call to control host Execute");
    }

    return;
}

int main(int argc, char *argv[])
{
    log<level::INFO>("Check if host is running");

    auto bus = sdbusplus::bus::new_default();

    auto s = "type='signal',member='CommandComplete',path='"s +
             CONTROL_HOST_PATH + "',interface='" +
             CONTROL_HOST_INTERFACE + "'";

    // Setup Signal Handler
    sdbusplus::bus::match::match hostControlSignals(bus,
                               s.c_str(),
                               hostControlSignal,
                               nullptr);

    sendHeartbeat(bus);

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
        log<level::INFO>("Host is running!");
        // Create file for host instance and create in filesystem to indicate
        // to services that host is running
        auto size = std::snprintf(nullptr,0,HOST_RUNNING_FILE,0);
        size++; // null
        std::unique_ptr<char[]> buf(new char[size]);
        std::snprintf(buf.get(),size,HOST_RUNNING_FILE,0);
        std::ofstream outfile(buf.get());
        outfile.close();
    }
    else
    {
        log<level::INFO>("Host is not running!");
    }

    return 0;
}
