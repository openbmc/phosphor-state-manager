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
    /* TODO */
    return 0;
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
        gpioplus::Handle handle(chip, {{gpioPortOffset, 0}}, flags,
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