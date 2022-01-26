#include "utils.hpp"

#include <gpiod.h>

#include <phosphor-logging/lg2.hpp>

namespace phosphor
{
namespace state
{
namespace manager
{
namespace utils
{

PHOSPHOR_LOG2_USING;

constexpr auto MAPPER_BUSNAME = "xyz.openbmc_project.ObjectMapper";
constexpr auto MAPPER_PATH = "/xyz/openbmc_project/object_mapper";
constexpr auto MAPPER_INTERFACE = "xyz.openbmc_project.ObjectMapper";
constexpr auto PROPERTY_INTERFACE = "org.freedesktop.DBus.Properties";

std::string getService(sdbusplus::bus::bus& bus, std::string path,
                       std::string interface)
{
    auto mapper = bus.new_method_call(MAPPER_BUSNAME, MAPPER_PATH,
                                      MAPPER_INTERFACE, "GetObject");

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
    catch (const sdbusplus::exception::exception& e)
    {
        error("Error in mapper call with path {PATH}, interface "
              "{INTERFACE}, and exception {ERROR}",
              "PATH", path, "INTERFACE", interface, "ERROR", e);
        throw;
    }

    return mapperResponse.begin()->first;
}

void setProperty(sdbusplus::bus::bus& bus, const std::string& path,
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
    sdbusplus::bus::bus& bus, const std::string& errorMsg,
    sdbusplus::xyz::openbmc_project::Logging::server::Entry::Level errLevel)
{

    try
    {
        // Create interface requires something for additionalData
        std::map<std::string, std::string> additionalData;
        additionalData.emplace("_PID", std::to_string(getpid()));

        auto method = bus.new_method_call(
            "xyz.openbmc_project.Logging", "/xyz/openbmc_project/logging",
            "xyz.openbmc_project.Logging.Create", "Create");

        method.append(errorMsg, errLevel, additionalData);
        auto resp = bus.call(method);
    }
    catch (const sdbusplus::exception::exception& e)
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

} // namespace utils
} // namespace manager
} // namespace state
} // namespace phosphor
