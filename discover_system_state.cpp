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

constexpr auto POWER_PATH = "/org/openbmc/control/power0";
constexpr auto POWER_INTERFACE = "org.freedesktop.DBus.Properties";

constexpr auto OCCSTATUS0_PATH = "/org/openbmc/sensors/host/cpu0/OccStatus";
constexpr auto OCCSTATUS0_INTERFACE = "org.openbmc.SensorValue";

constexpr auto OCCSTATUS1_PATH = "/org/openbmc/sensors/host/cpu1/OccStatus";
constexpr auto OCCSTATUS1_INTERFACE = "org.openbmc.SensorValue";

constexpr auto BOOTPROGRESS_PATH = "/org/openbmc/sensors/host/BootProgress";
constexpr auto BOOTPROGRESS_INTERFACE = "org.openbmc.SensorValue";

constexpr auto HOST_PATH = "/xyz/openbmc_project/state/host0";
constexpr auto HOST_INTERFACE = "xyz.openbmc_project.State.Host";

constexpr auto SETTINGS_PATH = "/org/openbmc/settings/host0";
constexpr auto SETTINGS_INTERFACE = "org.freedesktop.DBus.Properties";

constexpr auto SYSTEM_PATH = "/org/openbmc/managers/System";
constexpr auto SYSTEM_INTERFACE = "org.freedesktop.DBus.Properties";

auto getService(sdbusplus::bus::bus& bus, std::string PATH,
    std::string INTERFACE)
{
    auto mapper = bus.new_method_call(MAPPER_BUSNAME,
                                      MAPPER_PATH,
                                      MAPPER_INTERFACE,
                                      "GetObject");

    mapper.append(PATH, std::vector<std::string>({INTERFACE}));
    auto mapperResponseMsg = bus.call(mapper);

    if (mapperResponseMsg.is_method_error())
    {
        log<level::ERR>("Error in mapper call");
    }

    std::map<std::string, std::vector<std::string>> mapperResponse;
    mapperResponseMsg.read(mapperResponse);
    if (mapperResponse.empty())
    {
        log<level::ERR>("Error reading mapper response");
    }

    return mapperResponse.begin()->first;
}

auto getProperty(sdbusplus::bus::bus& bus, std::string PATH, std::string INTERFACE, std::string PROPERTY)
{
    sdbusplus::message::variant<std::string> Property;
    std::string stringProperty;

    std::string SERVICE = getService(bus, PATH, INTERFACE);

    auto method = bus.new_method_call(SERVICE.c_str(),
                                      PATH.c_str(),
                                      INTERFACE.c_str(),
                                      "Get");

    method.append(INTERFACE.c_str(), PROPERTY.c_str());
    auto reply = bus.call(method);

    if (reply.is_method_error())
    {
        log<level::ERR>("Error in property Get");
        // return nullptr;
    }

    reply.read(Property);
    stringProperty =
        sdbusplus::message::variant_ns::get<std::string>(Property);

    if (stringProperty.empty())
    {
        log<level::ERR>("Error reading property response");
        // return nullptr;
    }

    return stringProperty;
}

// void setProperty(sdbusplus::bus::bus& bus, std::string PATH, std::string INTERFACE, std::string PROPERTY, std::string VALUE)
// {
//     sdbusplus::message::variant<std::string> Property;
//     std::string stringProperty;
//
//     std::string BUSNAME = getBusName(bus, PATH, INTERFACE);
//
//     auto method = bus.new_method_call(BUSNAME.c_str(),
//                                       PATH.c_str(),
//                                       INTERFACE.c_str(),
//                                       "Set");
//
//     method.append(INTERFACE.c_str(), PROPERTY.c_str(), VALUE.c_str());
//     bus.call_noreply(method);
//
//     //TODO: error checking?
//
//     return;
// }

} // namespace manager
} // namespace state
} // namepsace phosphor

int main()
{
    auto bus = sdbusplus::bus::new_default();

    using namespace phosphor::state::manager;

    auto name = getService(bus, POWER_PATH, POWER_INTERFACE);
    std::cout << name << std::endl;

    auto pgood = getProperty(bus, POWER_PATH, POWER_INTERFACE, "pgood");
    // POWER_PATH + POWER_INTERFACE doesn't have a Get method!
    std::cout << pgood.c_str() << std::endl;

    // if(pgood != 1)
    // if (0 != 1)
    // {
    //     // Power is off, so check power policy
    //     auto system_last_state = getProperty(bus, SYSTEM_PATH,
    //         SYSTEM_INTERFACE, "system_last_state");
    //     auto power_policy = getProperty(bus, HOST_PATH,
    //         HOST_INTERFACE, "power_policy");
    //     log<level::INFO>("Last System State: ");
    //     // TODO: print system_last_state!
    //
    //     if (power_policy == "ALWAYS_POWER_ON" || (power_policy ==
    //         "RESTORE_LAST_STATE" && system_last_state == "HOST_POWERED_ON"))
    //     {
    //         setProperty(bus, HOST_PATH, HOST_INTERFACE,
    //             "RequestedHostTransition",
    //             "xyz.openbmc_project.State.Host.Transition.On");
    //     }
    //
    // }

    return 0;
}
