#include <iostream>
#include "host_state_manager.hpp"

namespace phosphor
{
namespace state
{
namespace manager
{

Host::Host(
        sdbusplus::bus::bus& bus,
        const char* busName,
        const char* objPath) :
        sdbusplus::server::object::object<
            sdbusplus::xyz::openbmc_project::State::server::Host>(
               bus, objPath),
         bus(bus),
         path(objPath) {}

} // namespace manager
} // namespace state
} // namepsace phosphor
