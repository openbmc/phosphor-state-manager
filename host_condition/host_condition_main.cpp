#include "config.h"
#include "host_condition.hpp"
#include <sdbusplus/bus.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/asio/io_context.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <sdbusplus/bus.hpp>

#include <cstdlib>
#include <iostream>

int main()
{
    sd_bus* bus;
    // Connect to system bus
    auto io = std::make_shared<boost::asio::io_context>();
    std::string objGroupName = HOST_GPIOS_OBJPATH;
    std::string objPathInst = objGroupName + "/host0";
    std::string busName = HOST_GPIOS_BUSNAME;

    sd_bus_default_system(&bus);

    auto sdbusp = std::make_shared<sdbusplus::asio::connection>(*io, bus);

    auto objManager = std::make_unique<sdbusplus::server::manager::manager>(
        *sdbusp, objGroupName.c_str());

    auto host = std::make_unique<phosphor::condition::Host>(
        *sdbusp, objPathInst.c_str());

    sdbusp->request_name(busName.c_str());

    io->run();

    return 0;
}

