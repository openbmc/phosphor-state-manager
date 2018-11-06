#pragma once

#include <sdbusplus/bus.hpp>

namespace phosphor
{
namespace state
{
namespace button
{

/**
 * @class Handler
 *
 * This class acts on the signals generated by the
 * /xyz/openbmc_project/Chassis/Buttons code when
 * it detects button presses.
 *
 * There are 3 buttons supported - Power, ID, and Host Reset.
 * As not all systems may implement each button, this class will
 * check for that button on D-Bus before listening for its signals.
 */
class Handler
{
  public:
    Handler() = delete;
    ~Handler() = default;
    Handler(const Handler&) = delete;
    Handler& operator=(const Handler&) = delete;
    Handler(Handler&&) = delete;
    Handler& operator=(Handler&&) = delete;

    /**
     * @brief Constructor
     *
     * @param[in] bus - sdbusplus connection object
     */
    Handler(sdbusplus::bus::bus& bus);

  private:
    /**
     * @brief sdbusplus connection object
     */
    sdbusplus::bus::bus& bus;
};

} // namespace button
} // namespace state
} // namespace phosphor
