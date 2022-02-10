#include "config.h"

#include "host_state_manager.hpp"

#include <sdbusplus/bus.hpp>

#include <cstdlib>
#include <exception>
#include <experimental/filesystem>
#include <iostream>

int main(int argc, char **argv)
{
    std::string hostId = "0";
    std::string hostBusName = HOST_BUSNAME;
    if(argc == 2){
        hostId = argv[1];
        if(hostId != "0")
        {
            hostBusName += hostId;
        }
    }

    namespace fs = std::experimental::filesystem;

    auto bus = sdbusplus::bus::new_default();

    auto objPathInst = std::string{HOST_OBJPATH} + hostId;

    // Add sdbusplus ObjectManager.
    sdbusplus::server::manager::manager objManager(bus, objPathInst.c_str());

    phosphor::state::manager::Host manager(bus, objPathInst.c_str(), hostId);

    auto dir = fs::path(HOST_STATE_PERSIST_PATH).parent_path();
    fs::create_directories(dir);

    bus.request_name(hostBusName.c_str());

    while (true)
    {
        bus.process_discard();
        bus.wait();
    }
    return 0;
}
