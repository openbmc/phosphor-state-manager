#pragma once

#include <sdbusplus/bus.hpp>
#include <sdbusplus/bus/match.hpp>

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
     * @brief The handler for a power button press
     *
     * It will power on the system if it's currently off,
     * else it will soft power it off.
     *
     * @param[in] msg - sdbusplus message from signal
     */
    void powerPress(sdbusplus::message::message& msg);

    /**
     * @brief The handler for a long power button press
     *
     * If the system is currently powered on, it will
     * perform an immediate power off.
     *
     * @param[in] msg - sdbusplus message from signal
     */
    void longPowerPress(sdbusplus::message::message& msg);

    /**
     * @brief The handler for a reset button press
     *
     * Reboots the host if it is powered on.
     *
     * @param[in] msg - sdbusplus message from signal
     */
    void resetPress(sdbusplus::message::message& msg);

    /**
     * @brief Checks if system is powered on
     *
     * @return true if powered on, false else
     */
    bool poweredOn() const;

    /**
     * @brief Returns the service name for an object
     *
     * @param[in] path - the object path
     * @param[in] interface - the interface name
     *
     * @return std::string - the D-Bus service name if found, else
     *                       an empty string
     */
    std::string getService(const std::string& path,
                           const std::string& interface) const;

    /**
     * @brief sdbusplus connection object
     */
    sdbusplus::bus::bus& bus;

    /**
     *  @brief The object path of the chassis state manager
     */
    const std::string chassisPath;

    /**
     *  @brief The object path of the host state manager
     */
    const std::string hostPath;

    /**
     * @brief Matches on the power button released signal
     */
    std::unique_ptr<sdbusplus::bus::match_t> powerButtonRelease;

    /**
     * @brief Matches on the power button long press released signal
     */
    std::unique_ptr<sdbusplus::bus::match_t> powerButtonLongPressRelease;

    /**
     * @brief Matches on the reset button released signal
     */
    std::unique_ptr<sdbusplus::bus::match_t> resetButtonRelease;
};

} // namespace button
} // namespace state
} // namespace phosphor
