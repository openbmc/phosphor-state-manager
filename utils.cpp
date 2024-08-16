#include "config.h"

#include "utils.hpp"

#include <gpiod.h>

#include <phosphor-logging/lg2.hpp>
#include <xyz/openbmc_project/Dump/Create/client.hpp>
#include <xyz/openbmc_project/Logging/Create/client.hpp>
#include <xyz/openbmc_project/ObjectMapper/client.hpp>
#include <xyz/openbmc_project/State/BMC/client.hpp>

#include <chrono>
#include <filesystem>
#include <format>

namespace phosphor
{
namespace state
{
namespace manager
{
namespace utils
{

using namespace std::literals::chrono_literals;

PHOSPHOR_LOG2_USING;

constexpr auto SYSTEMD_SERVICE = "org.freedesktop.systemd1";
constexpr auto SYSTEMD_OBJ_PATH = "/org/freedesktop/systemd1";
constexpr auto SYSTEMD_INTERFACE = "org.freedesktop.systemd1.Manager";
constexpr auto PROPERTY_INTERFACE = "org.freedesktop.DBus.Properties";

using ObjectMapper = sdbusplus::client::xyz::openbmc_project::ObjectMapper<>;

void subscribeToSystemdSignals(sdbusplus::bus_t& bus)
{
    auto method = bus.new_method_call(SYSTEMD_SERVICE, SYSTEMD_OBJ_PATH,
                                      SYSTEMD_INTERFACE, "Subscribe");

    try
    {
        // On OpenBMC based systems, systemd has had a few situations where it
        // has been unable to respond to this call within the default d-bus
        // timeout of 25 seconds. This is due to the large amount of work being
        // done by systemd during OpenBMC startup. Set the timeout for this call
        // to 60 seconds (worst case seen was around 30s so double it).
        bus.call(method, 60s);
    }
    catch (const sdbusplus::exception_t& e)
    {
        error("Failed to subscribe to systemd signals: {ERROR}", "ERROR", e);
        throw std::runtime_error("Unable to subscribe to systemd signals");
    }
    return;
}

std::string getService(sdbusplus::bus_t& bus, std::string path,
                       std::string interface)
{
    auto mapper = bus.new_method_call(ObjectMapper::default_service,
                                      ObjectMapper::instance_path,
                                      ObjectMapper::interface, "GetObject");

    mapper.append(path, std::vector<std::string>({interface}));

    std::vector<std::pair<std::string, std::vector<std::string>>>
        mapperResponse;

    try
    {
        auto mapperResponseMsg = bus.call(mapper);

        mapperResponseMsg.read(mapperResponse);
        if (mapperResponse.empty())
        {
            error(
                "Error no matching service with path {PATH} and interface {INTERFACE}",
                "PATH", path, "INTERFACE", interface);
            throw std::runtime_error("Error no matching service");
        }
    }
    catch (const sdbusplus::exception_t& e)
    {
        error("Error in mapper call with path {PATH}, interface "
              "{INTERFACE}, and exception {ERROR}",
              "PATH", path, "INTERFACE", interface, "ERROR", e);
        throw;
    }

    return mapperResponse.begin()->first;
}

std::string getProperty(sdbusplus::bus_t& bus, const std::string& path,
                        const std::string& interface,
                        const std::string& propertyName)
{
    std::variant<std::string> property;
    std::string service = getService(bus, path, interface);

    auto method = bus.new_method_call(service.c_str(), path.c_str(),
                                      PROPERTY_INTERFACE, "Get");

    method.append(interface, propertyName);

    try
    {
        auto reply = bus.call(method);
        reply.read(property);
    }
    catch (const sdbusplus::exception_t& e)
    {
        error("Error in property Get, error {ERROR}, property {PROPERTY}",
              "ERROR", e, "PROPERTY", propertyName);
        throw;
    }

    if (std::get<std::string>(property).empty())
    {
        error("Error reading property response for {PROPERTY}", "PROPERTY",
              propertyName);
        throw std::runtime_error("Error reading property response");
    }

    return std::get<std::string>(property);
}

void setProperty(sdbusplus::bus_t& bus, const std::string& path,
                 const std::string& interface, const std::string& property,
                 const std::string& value)
{
    std::variant<std::string> variantValue = value;
    std::string service = getService(bus, path, interface);

    auto method = bus.new_method_call(service.c_str(), path.c_str(),
                                      PROPERTY_INTERFACE, "Set");

    method.append(interface, property, variantValue);
    bus.call_noreply(method);

    return;
}

int getGpioValue(const std::string& gpioName)
{
    int gpioval = -1;
    gpiod_line* line = gpiod_line_find(gpioName.c_str());

    if (nullptr != line)
    {
        // take ownership of gpio
        if (0 != gpiod_line_request_input(line, "state-manager"))
        {
            error("Failed request for {GPIO_NAME} GPIO", "GPIO_NAME", gpioName);
        }
        else
        {
            // get gpio value
            gpioval = gpiod_line_get_value(line);

            // release ownership of gpio
            gpiod_line_close_chip(line);
        }
    }
    return gpioval;
}

void createError(
    sdbusplus::bus_t& bus, const std::string& errorMsg,
    sdbusplus::server::xyz::openbmc_project::logging::Entry::Level errLevel,
    std::map<std::string, std::string> additionalData)
{
    try
    {
        // Always add the _PID on for some extra logging debug
        additionalData.emplace("_PID", std::to_string(getpid()));

        using LoggingCreate =
            sdbusplus::client::xyz::openbmc_project::logging::Create<>;

        auto method = bus.new_method_call(LoggingCreate::default_service,
                                          LoggingCreate::instance_path,
                                          LoggingCreate::interface, "Create");

        method.append(errorMsg, errLevel, additionalData);
        auto resp = bus.call(method);
    }
    catch (const sdbusplus::exception_t& e)
    {
        error("sdbusplus D-Bus call exception, error {ERROR} trying to create "
              "an error with {ERROR_MSG}",
              "ERROR", e, "ERROR_MSG", errorMsg);

        throw std::runtime_error(
            "Error in invoking D-Bus logging create interface");
    }
    catch (const std::exception& e)
    {
        error("D-bus call exception: {ERROR}", "ERROR", e);
        throw e;
    }
}

void createBmcDump(sdbusplus::bus_t& bus)
{
    using DumpCreate = sdbusplus::client::xyz::openbmc_project::dump::Create<>;
    auto dumpPath =
        sdbusplus::message::object_path(DumpCreate::namespace_path::value) /
        DumpCreate::namespace_path::bmc;

    auto method =
        bus.new_method_call(DumpCreate::default_service, dumpPath.str.c_str(),
                            DumpCreate::interface, "CreateDump");
    method.append(
        std::vector<
            std::pair<std::string, std::variant<std::string, uint64_t>>>());
    try
    {
        bus.call_noreply(method);
    }
    catch (const sdbusplus::exception_t& e)
    {
        error("Failed to create BMC dump, exception:{ERROR}", "ERROR", e);
        // just continue, this is error path anyway so we're just collecting
        // what we can
    }
}

bool checkACLoss(size_t& chassisId)
{
    std::string chassisLostPowerFileFmt =
        std::format(CHASSIS_LOST_POWER_FILE, chassisId);

    std::filesystem::path chassisPowerLossFile{chassisLostPowerFileFmt};
    return std::filesystem::exists(chassisPowerLossFile);
}

bool isBmcReady(sdbusplus::bus_t& bus)
{
    using BMC = sdbusplus::client::xyz::openbmc_project::state::BMC<>;
    auto bmcPath = sdbusplus::message::object_path(BMC::namespace_path::value) /
                   BMC::namespace_path::bmc;

    auto bmcState =
        getProperty(bus, bmcPath.str, BMC::interface, "CurrentBMCState");

    if (sdbusplus::message::convert_from_string<BMC::BMCState>(bmcState) !=
        BMC::BMCState::Ready)
    {
        debug("BMC State is {BMC_STATE}", "BMC_STATE", bmcState);
        return false;
    }
    return true;
}

bool waitBmcReady(sdbusplus::bus_t& bus, std::chrono::seconds timeout)
{
    while (timeout.count() != 0)
    {
        timeout--;
        if (isBmcReady(bus))
        {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    return false;
}

} // namespace utils
} // namespace manager
} // namespace state
} // namespace phosphor
