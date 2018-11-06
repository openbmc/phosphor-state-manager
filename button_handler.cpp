#include "button_handler.hpp"

namespace phosphor
{
namespace state
{
namespace button
{

Handler::Handler(sdbusplus::bus::bus& bus) : bus(bus)
{
}

} // namespace button
} // namespace state
} // namespace phosphor
