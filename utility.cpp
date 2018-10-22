#include "utility.hpp"

#include <sys/types.h>
#include <dirent.h>

#include <exception>
#include <gpioplus/chip.hpp>
#include <gpioplus/handle.hpp>
#include <string>
#include <fstream>
#include <iostream>
#include <cstring>


namespace utility
{

int convertGpioToNum(const char* gpio)
{
    size_t len = std::strlen(gpio);
    if (len < 2)
    {
        fprintf(stderr, "Invald GPIO name %s\n", gpio);
        return -1;
    }

    /* Read the offset from the last character */
    if (!std::isdigit(gpio[len - 1]))
    {
        fprintf(stderr, "Invalid GPIO offset in GPIO %s\n", gpio);
        return -1;
    }

    int offset = gpio[len - 1] - '0';

    /* Read the port from the second to last character */
    if (!std::isalpha(gpio[len - 2]))
    {
        fprintf(stderr, "Invalid GPIO port in GPIO %s\n", gpio);
        return -1;
    }
    int port = std::toupper(gpio[len - 2]) - 'A';

    /* Check for a 2 character port, like AA */
    if ((len == 3) && std::isalpha(gpio[len - 3]))
    {
        port += 26 * (std::toupper(gpio[len - 3]) - 'A' + 1);
    }

    return (port * gpioPortOffset) + offset;
}

bool gpioSetValue(const char* gpioName, bool activeLow, bool asserted)
{
    int gpioId = convertGpioToNum(gpioName);
    if (gpioId < 0)
    {
        return false;
    }

    try
    {
        gpioplus::Chip chip(gpioId);
        gpioplus::HandleFlags flags(chip.getLineInfo(gpioPortOffset).flags);
        flags.output = true;
        gpioplus::Handle handle(
                chip,
                {{gpioPortOffset, 0}},
                flags,
                "chassiskill");

        bool value = (asserted ^ activeLow);
        handle.setValues({value});
    }
    catch (const std::exception& e)
    {
        fprintf(stderr, "Error: %s\n", e.what());
        return false;
    }

    return true;
}
} // namespace utility