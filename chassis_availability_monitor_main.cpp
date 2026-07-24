#include "chassis_availability_monitor.hpp"

#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/bus.hpp>

PHOSPHOR_LOG2_USING;

int main()
{
    auto bus = sdbusplus::bus::new_default();

    constexpr auto configPath =
        "/usr/share/phosphor-state-manager/chassis-availability/"
        "phosphor-chassis-availability-default.json";

    phosphor::state::manager::ChassisAvailability monitor(bus, configPath);

    while (true)
    {
        bus.process_discard();
        bus.wait();
    }
    return 0;
}
