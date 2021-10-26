#include "host_condition.hpp"

#include <gpiod.hpp>
#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/lg2.hpp>

namespace phosphor
{
namespace condition
{

PHOSPHOR_LOG2_USING;

using namespace phosphor::logging;

void Host::scanGpioPin()
{
    /*
     * Check the host0-ready/-n pin is defined or not
     */
    if (gpiod::find_line("host0-ready"))
    {
        lineName = "host0-ready";
        isActHigh = true;
    }
    else if (gpiod::find_line("host0-ready-n"))
    {
        lineName = "host0-ready-n";
        isActHigh = false;
    }
}

Host::FirmwareCondition Host::currentFirmwareCondition() const
{
    auto retVal = Host::FirmwareCondition::Unknown;

    /*
     * Check host is ready or not
     */
    if (!lineName.empty())
    {
        auto line = gpiod::find_line(lineName);

        try
        {
            int32_t gpioVal = 0;

            line.request({lineName, gpiod::line_request::DIRECTION_INPUT,
                          (true == isActHigh)
                              ? 0
                              : gpiod::line_request::FLAG_ACTIVE_LOW});

            gpioVal = line.get_value();
            line.release();

            retVal = (0 == gpioVal) ? Host::FirmwareCondition::Off
                                    : Host::FirmwareCondition::Running;
        }
        catch (std::system_error&)
        {
            error("Error when read gpio value");
        }
    }

    return retVal;
}
} // namespace condition
} // namespace phosphor
