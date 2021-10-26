#include "host_condition.hpp"

#include <gpiod.hpp>
#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>

namespace phosphor
{
namespace condition
{
Host::FirmwareCondition Host::currentFirmwareCondition() const
{
    bool isActHigh = true;
    bool foundFlag = false;
    int32_t gpioVal = 0;
    std::string lineName = "host0-ready";
    auto retVal = Host::FirmwareCondition::Unknown;

    /*
     * Check the host0-ready/-n pin is defined or not
     */
    if (gpiod::find_line(lineName))
    {
        foundFlag = true;
    }
    else if (gpiod::find_line(lineName + "-n"))
    {
        lineName += "-n";
        foundFlag = true;
        isActHigh = false;
    }

    /*
     * Check host is ready or not
     */
    if (true == foundFlag)
    {
        auto line = gpiod::find_line(lineName);

        line.request(
            {lineName, gpiod::line_request::DIRECTION_INPUT,
             (true == isActHigh) ? 0 : gpiod::line_request::FLAG_ACTIVE_LOW});

        gpioVal = line.get_value();
        line.release();

        retVal = (0 == gpioVal) ? Host::FirmwareCondition::Off
                                : Host::FirmwareCondition::Running;
    }

    return retVal;
}
} // namespace condition
} // namespace phosphor
