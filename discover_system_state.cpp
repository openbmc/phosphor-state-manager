#include <iostream>
#include <map>
#include <string>
#include <config.h>
#include <systemd/sd-bus.h>
#include <sdbusplus/server.hpp>
#include <phosphor-logging/log.hpp>
#include "chassis_state_manager.hpp"

namespace phosphor
{
namespace state
{
namespace manager
{

using namespace phosphor::logging;

constexpr auto MAPPER_BUSNAME = "xyz.openbmc_project.ObjectMapper";
constexpr auto MAPPER_PATH = "/xyz/openbmc_project/object_mapper";
constexpr auto MAPPER_INTERFACE = "xyz.openbmc_project.ObjectMapper";

constexpr auto PROPERTY_INTERFACE = "org.freedesktop.DBus.Properties";

constexpr auto HOST_PATH = "/xyz/openbmc_project/state/host0";

constexpr auto SETTINGS_PATH = "/org/openbmc/settings/host0";
constexpr auto SETTINGS_INTERFACE = "org.openbmc.settings.Host";

constexpr auto CHASSIS_PATH = "/xyz/openbmc_project/state/chassis0";

std::string getService(sdbusplus::bus::bus& bus, std::string path,
    std::string interface)
{
    auto mapper = bus.new_method_call(MAPPER_BUSNAME,
                                      MAPPER_PATH,
                                      MAPPER_INTERFACE,
                                      "GetObject");

    mapper.append(path, std::vector<std::string>({interface}));
    auto mapperResponseMsg = bus.call(mapper);

    if (mapperResponseMsg.is_method_error())
    {
        log<level::ERR>("Error in mapper call",
            entry("PATH=%s", path.c_str()),
            entry("INTERFACE=%s", interface.c_str()));
        throw std::runtime_error("Error in mapper call");
    }

    std::map<std::string, std::vector<std::string>> mapperResponse;
    mapperResponseMsg.read(mapperResponse);
    if (mapperResponse.empty())
    {
        log<level::ERR>("Error reading mapper response",
            entry("PATH=%s", path.c_str()),
            entry("INTERFACE=%s", interface.c_str()));
        throw std::runtime_error("Error reading mapper response");
    }

    return mapperResponse.begin()->first;
}

std::string getProperty(sdbusplus::bus::bus& bus, std::string path,
    std::string interface, std::string propertyName)
{
    sdbusplus::message::variant<std::string> property;
    std::string service = getService(bus, path, interface);

    auto method = bus.new_method_call(service.c_str(),
                                      path.c_str(),
                                      PROPERTY_INTERFACE,
                                      "Get");

    method.append(interface, propertyName);
    auto reply = bus.call(method);

    if (reply.is_method_error())
    {
        log<level::ERR>("Error in property Get",
            entry("PROPERTY=%s", propertyName.c_str()));
        throw std::runtime_error("Error in property Get");
    }

    reply.read(property);

    if (sdbusplus::message::variant_ns::get<std::string>(property).empty())
    {
        log<level::ERR>("Error reading property response",
            entry("PROPERTY=%s", propertyName.c_str()));
        throw std::runtime_error("Error reading property response");
    }

    return sdbusplus::message::variant_ns::get<std::string>(property);
}

void setProperty(sdbusplus::bus::bus& bus, std::string path,
    std::string interface, std::string property, std::string value)
{
    sdbusplus::message::variant<std::string> variantValue = value;
    std::string service = getService(bus, path, interface);

    auto method = bus.new_method_call(service.c_str(),
                                      path.c_str(),
                                      PROPERTY_INTERFACE,
                                      "Set");

    method.append(interface, property, variantValue);
    bus.call_noreply(method);

    return;
}

} // namespace manager
} // namespace state
} // namepsace phosphor

int main()
{
    auto bus = sdbusplus::bus::new_default();

    using namespace phosphor::state::manager;
    namespace server = sdbusplus::xyz::openbmc_project::State::server;

    std::string currentPowerState = getProperty(bus, CHASSIS_PATH,
        CHASSIS_BUSNAME, "CurrentPowerState");

    if(currentPowerState == convertForMessage(server::Chassis::PowerState::Off))
    {
        std::string power_policy =
            getProperty(bus, SETTINGS_PATH, SETTINGS_INTERFACE, "power_policy");

        log<level::INFO>("Host power is off, checking power policy",
            entry("POWER_POLICY=%s", power_policy.c_str()));

        if (power_policy == "ALWAYS_POWER_ON")
        {
            log<level::INFO>("power_policy=ALWAYS_POWER_ON, powering host on");
            setProperty(bus, HOST_PATH, HOST_BUSNAME,
                "RequestedHostTransition",
                "xyz.openbmc_project.State.Host.Transition.On");
        }

    }

    return 0;
}
