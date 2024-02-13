#include "config.h"

#include <unistd.h>

#include <phosphor-logging/elog.hpp>
#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/exception.hpp>
#include <xyz/openbmc_project/Logging/Create/client.hpp>
#include <xyz/openbmc_project/Logging/Entry/client.hpp>
#include <xyz/openbmc_project/State/Boot/Progress/client.hpp>

#include <cstdlib>
#include <format>
#include <fstream>
#include <string>

namespace phosphor
{
namespace state
{
namespace manager
{

PHOSPHOR_LOG2_USING;

using BootProgress =
    sdbusplus::client::xyz::openbmc_project::state::boot::Progress<>;
using LoggingCreate =
    sdbusplus::client::xyz::openbmc_project::logging::Create<>;
using LoggingEntry = sdbusplus::client::xyz::openbmc_project::logging::Entry<>;

constexpr auto HOST_STATE_SVC = "xyz.openbmc_project.State.Host";
constexpr auto HOST_STATE_PATH = "/xyz/openbmc_project/state/host0";
constexpr auto PROPERTY_INTERFACE = "org.freedesktop.DBus.Properties";
constexpr auto BOOT_PROGRESS_PROP = "BootProgress";

constexpr auto SYSTEMD_SERVICE = "org.freedesktop.systemd1";
constexpr auto SYSTEMD_OBJ_PATH = "/org/freedesktop/systemd1";
constexpr auto SYSTEMD_INTERFACE = "org.freedesktop.systemd1.Manager";
constexpr auto HOST_STATE_QUIESCE_TGT = "obmc-host-quiesce@0.target";

bool wasHostBooting(sdbusplus::bus_t& bus)
{
    try
    {
        using ProgressStages = BootProgress::ProgressStages;

        auto method = bus.new_method_call(HOST_STATE_SVC, HOST_STATE_PATH,
                                          PROPERTY_INTERFACE, "Get");
        method.append(BootProgress::interface, BOOT_PROGRESS_PROP);

        auto response = bus.call(method);

        std::variant<ProgressStages> bootProgressV;
        response.read(bootProgressV);
        auto bootProgress = std::get<ProgressStages>(bootProgressV);

        if (bootProgress == ProgressStages::Unspecified)
        {
            info("Host was not booting before BMC reboot");
            return false;
        }

        info("Host was booting before BMC reboot: {BOOTPROGRESS}",
             "BOOTPROGRESS", bootProgress);
    }
    catch (const sdbusplus::exception_t& e)
    {
        error("Error reading BootProgress, error {ERROR}, service {SERVICE}, "
              "path {PATH}",
              "ERROR", e, "SERVICE", HOST_STATE_SVC, "PATH", HOST_STATE_PATH);

        throw;
    }

    return true;
}

void createErrorLog(sdbusplus::bus_t& bus)
{
    try
    {
        // Create interface requires something for additionalData
        std::map<std::string, std::string> additionalData;
        additionalData.emplace("_PID", std::to_string(getpid()));

        static constexpr auto errorMessage =
            "xyz.openbmc_project.State.Error.HostNotRunning";
        auto method = bus.new_method_call(LoggingCreate::default_service,
                                          LoggingCreate::instance_path,
                                          LoggingCreate::interface, "Create");
        method.append(errorMessage, LoggingEntry::Level::Error, additionalData);
        auto resp = bus.call(method);
    }
    catch (const sdbusplus::exception_t& e)
    {
        error(
            "sdbusplus D-Bus call exception, error {ERROR}, objpath {OBJPATH}, "
            "interface {INTERFACE}",
            "ERROR", e, "OBJPATH", LoggingCreate::instance_path, "INTERFACE",
            LoggingCreate::interface);

        throw std::runtime_error(
            "Error in invoking D-Bus logging create interface");
    }
    catch (const std::exception& e)
    {
        error("D-bus call exception: {ERROR}", "ERROR", e);
        throw e;
    }
}

// Once CHASSIS_ON_FILE is removed, the obmc-chassis-poweron@.target has
// completed and the phosphor-chassis-state-manager code has processed it.
bool isChassisTargetComplete()
{
    auto chassisFile = std::format(CHASSIS_ON_FILE, 0);

    std::ifstream f(chassisFile);
    return !f.good();
}

void moveToHostQuiesce(sdbusplus::bus_t& bus)
{
    try
    {
        auto method = bus.new_method_call(SYSTEMD_SERVICE, SYSTEMD_OBJ_PATH,
                                          SYSTEMD_INTERFACE, "StartUnit");

        method.append(HOST_STATE_QUIESCE_TGT);
        method.append("replace");

        bus.call_noreply(method);
    }
    catch (const sdbusplus::exception_t& e)
    {
        error("sdbusplus call exception starting quiesce target: {ERROR}",
              "ERROR", e);

        throw std::runtime_error(
            "Error in invoking D-Bus systemd StartUnit method");
    }
}

} // namespace manager
} // namespace state
} // namespace phosphor

int main()
{
    using namespace phosphor::state::manager;
    PHOSPHOR_LOG2_USING;

    auto bus = sdbusplus::bus::new_default();

    // Chassis power is on if this service starts but need to wait for the
    // obmc-chassis-poweron@.target to complete before potentially initiating
    // another systemd target transition (i.e. Quiesce->Reboot)
    while (!isChassisTargetComplete())
    {
        debug("Waiting for chassis on target to complete");
        std::this_thread::sleep_for(std::chrono::seconds(1));

        // There is no timeout here, wait until it happens or until system
        // is powered off and this service is stopped
    }

    info("Chassis power on has completed, checking if host is "
         "still running after the BMC reboot");

    // Check the last BootProgeress to see if the host was booting before
    // the BMC reboot occurred
    if (!wasHostBooting(bus))
    {
        return 0;
    }

    // Host was booting before the BMC reboot so log an error and go to host
    // quiesce target
    createErrorLog(bus);
    moveToHostQuiesce(bus);

    return 0;
}
