#include <phosphor-logging/log.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/exception.hpp>

#include <cstdlib>
#include <string>

constexpr auto HOST_STATE_SVC = "xyz.openbmc_project.State.Host";
constexpr auto HOST_STATE_PATH = "/xyz/openbmc_project/state/host0";
constexpr auto PROPERTY_INTERFACE = "org.freedesktop.DBus.Properties";
constexpr auto BOOT_STATE_INTF = "xyz.openbmc_project.State.Boot.Progress";
constexpr auto BOOT_PROGRESS_PROP = "BootProgress";

using namespace phosphor::logging;
using sdbusplus::exception::SdBusError;

bool wasHostBooting(sdbusplus::bus::bus& bus)
{
    try
    {
        auto method = bus.new_method_call(HOST_STATE_SVC, HOST_STATE_PATH,
                                          PROPERTY_INTERFACE, "Get");
        method.append(BOOT_STATE_INTF, BOOT_PROGRESS_PROP);

        auto response = bus.call(method);

        std::variant<std::string> bootProgress;
        response.read(bootProgress);

        if (std::get<std::string>(bootProgress) ==
            "xyz.openbmc_project.State.Boot.Progress.ProgressStages."
            "Unspecified")
        {
            log<level::INFO>("Host was not booting before BMC reboot");
            return false;
        }

        log<level::INFO>("Host was booting before BMC reboot",
                         entry("BOOTPROGRESS=%s",
                               std::get<std::string>(bootProgress).c_str()));
    }
    catch (const SdBusError& e)
    {
        log<level::ERR>("Error reading BootProgress",
                        entry("ERROR=%s", e.what()),
                        entry("SERVICE=%s", HOST_STATE_SVC),
                        entry("PATH=%s", HOST_STATE_PATH));

        throw;
    }

    return true;
}

int main()
{

    auto bus = sdbusplus::bus::new_default();

    // This application will only be started if chassis power is on and the
    // host is not responding after a BMC reboot. Check the last BootProgeress
    // to see if the host was booting before the BMC reboot occurred

    if (!wasHostBooting(bus))
    {
        return 0;
    }

    // Host was booting before the BMC reboot so log an error and go to host
    // quiesce target
    // TODO Create Error
    // TODO Move to Host Quiesce

    return 0;
}
