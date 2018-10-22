#pragma once

namespace utility
{
constexpr auto gpioBasePath = "/sys/class/gpio";
constexpr auto gpioPortOffset = 8;

/**
 * Converts the GPIO port/offset nomenclature value
 * to a number.  Supports the ASPEED method where
 * num = base + (port * 8) + offset, and the port is an
 * A-Z character that converts to 0 to 25.  The base
 * is obtained form the hardware.
 *
 * For example: "A5" -> 5,  "Z7" -> 207
 *
 * @param[in] gpio - the GPIO name
 *
 * @return int - the GPIO number or < 0 if not found
 */
int convertGpioToNum(const char* gpio);

/** @brief Set the value of a specified gpio
 * 
 *  @param[in] gpio_name - GPIO to set
 *  @param[in] active_low - is pin is active at low voltage
 *  @param[in] asserted - is pin in active state
 * 
 *  @return bool - success of setting the GPIO
 */
bool gpioSetValue(const char* gpioName, bool activeLow, bool asserted);

} // namespace utility