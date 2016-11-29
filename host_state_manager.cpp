#include <iostream>
#include "host_state_manager.hpp"

namespace phosphor
{
namespace statemanager
{

Host::Host(
        sdbusplus::bus::bus& bus,
        const char* busName,
        const char* objPath) :
        sdbusplus::server::object::object<
            sdbusplus::xyz::openbmc_project::State::server::Host>(
               bus, objPath),
         _bus(bus),
         _path(objPath) {}

} // namespace statemanager
} // namepsace phosphor
