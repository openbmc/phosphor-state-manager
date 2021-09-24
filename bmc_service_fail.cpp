#include <unistd.h>

#include <phosphor-logging/elog.hpp>
#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/exception.hpp>
#include <xyz/openbmc_project/Logging/Create/server.hpp>
#include <xyz/openbmc_project/Logging/Entry/server.hpp>

#include <string>

namespace phosphor
{
namespace state
{
namespace manager
{

PHOSPHOR_LOG2_USING;

constexpr auto LOGGING_SVC = "xyz.openbmc_project.Logging";
constexpr auto LOGGING_PATH = "/xyz/openbmc_project/logging";
constexpr auto LOGGING_CREATE_INTF = "xyz.openbmc_project.Logging.Create";

void createServiceFailErrorLog(sdbusplus::bus::bus& bus)
{
    try
    {
        // Create interface requires something for additionalData
        std::map<std::string, std::string> additionalData;
        additionalData.emplace("_PID", std::to_string(getpid()));

        static constexpr auto errorMessage =
            "xyz.openbmc_project.State.Error.BmcServiceFailure";
        auto method = bus.new_method_call(LOGGING_SVC, LOGGING_PATH,
                                          LOGGING_CREATE_INTF, "Create");
        auto level =
            sdbusplus::xyz::openbmc_project::Logging::server::convertForMessage(
                sdbusplus::xyz::openbmc_project::Logging::server::Entry::Level::
                    Error);
        method.append(errorMessage, level, additionalData);
        auto resp = bus.call(method);
    }
    catch (const sdbusplus::exception::exception& e)
    {
        error(
            "sdbusplus D-Bus call exception, error {ERROR}, objpath {OBJPATH}, "
            "interface {INTERFACE}",
            "ERROR", e, "OBJPATH", LOGGING_PATH, "INTERFACE",
            LOGGING_CREATE_INTF);

        throw std::runtime_error(
            "Error in invoking D-Bus logging create interface");
    }
    catch (std::exception& e)
    {
        error("D-bus call exception: {ERROR}", "ERROR", e);
        throw e;
    }
}

} // namespace manager
} // namespace state
} // namespace phosphor

int main()
{
    using namespace phosphor::state::manager;
    PHOSPHOR_LOG2_USING;

    error("A BMC service has entered failed state, creating error log");
    auto bus = sdbusplus::bus::new_default();

    createServiceFailErrorLog(bus);

    return 0;
}
