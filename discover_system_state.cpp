#include <iostream>
#include <map>
#include <string>
#include <systemd/sd-bus.h>
#include <sdbusplus/server.hpp>
#include <phosphor-logging/log.hpp>

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

constexpr auto POWER_PATH = "/org/openbmc/control/power0";
constexpr auto POWER_INTERFACE = "org.openbmc.control.Power";

constexpr auto HOST_PATH = "/xyz/openbmc_project/state/host0";
constexpr auto HOST_INTERFACE = "xyz.openbmc_project.State.Host";

constexpr auto SETTINGS_PATH = "/org/openbmc/settings/host0";
constexpr auto SETTINGS_INTERFACE = "org.openbmc.settings.Host";

constexpr auto SYSTEM_PATH = "/org/openbmc/managers/System";
constexpr auto SYSTEM_INTERFACE = "org.openbmc.managers.System";

auto getService(sdbusplus::bus::bus& bus, std::string path,
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
        log<level::ERR>("Error in mapper call");
        throw std::runtime_error("Error in mapper call");
    }

    std::map<std::string, std::vector<std::string>> mapperResponse;
    mapperResponseMsg.read(mapperResponse);
    if (mapperResponse.empty())
    {
        log<level::ERR>("Error reading mapper response");
        throw std::runtime_error("Error reading mapper response");
    }

    return mapperResponse.begin()->first;
}

auto getProperty(sdbusplus::bus::bus& bus, std::string path,
    std::string interface, std::string property)
{
    sdbusplus::message::variant<std::string, int> variantProperty;
    std::string service = getService(bus, path, interface);

    auto method = bus.new_method_call(service.c_str(),
                                      path.c_str(),
                                      PROPERTY_INTERFACE,
                                      "Get");

    method.append(interface, property);
    auto reply = bus.call(method);

    if (reply.is_method_error())
    {
        log<level::ERR>("Error in property Get");
        throw std::runtime_error("Error in property Get");
    }

    reply.read(variantProperty);

    if (variantProperty.which() == 1)
    {
        if (sdbusplus::message::variant_ns::get<int>(variantProperty) != 0 &&
                sdbusplus::message::variant_ns::get<int>(variantProperty) != 1)
        {
            log<level::ERR>("Error reading property response");
            throw std::runtime_error("Error reading property response");
        }
    }
    else
    {
        if (sdbusplus::message::variant_ns::
            get<std::string>(variantProperty).empty())
        {
            log<level::ERR>("Error reading property response");
            throw std::runtime_error("Error reading property response");
        }
    }

    return variantProperty;
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

    int pgood = sdbusplus::message::variant_ns::get<int>(
        getProperty(bus, POWER_PATH, POWER_INTERFACE, "pgood"));

    if(pgood != 1)
    {
        // Power is off, so check power policy
        int system_last_state =
            atoi(sdbusplus::message::variant_ns::get<std::string>(
            getProperty(bus, SYSTEM_PATH, SYSTEM_INTERFACE,
            "system_last_state")).c_str());
        std::string power_policy =
            sdbusplus::message::variant_ns::get<std::string>(
            getProperty(bus, SETTINGS_PATH, SETTINGS_INTERFACE,
            "power_policy"));

        std::cout << "power_policy = " << power_policy.c_str() << '\n';
        std::cout << "system_last_state = " << system_last_state << '\n';

        if (power_policy == "ALWAYS_POWER_ON" ||
            (power_policy == "RESTORE_LAST_STATE" && system_last_state == 1))
        {
            setProperty(bus, HOST_PATH, HOST_INTERFACE,
                "RequestedHostTransition",
                "xyz.openbmc_project.State.Host.Transition.On");
        }

    }

    return 0;
}
