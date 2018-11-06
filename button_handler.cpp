#include "button_handler.hpp"
#include <phosphor-logging/log.hpp>
#include <xyz/openbmc_project/State/Chassis/server.hpp>
#include <xyz/openbmc_project/State/Host/server.hpp>
#include "config.h"

namespace phosphor
{
namespace state
{
namespace button
{

namespace sdbusRule = sdbusplus::bus::match::rules;
using namespace sdbusplus::xyz::openbmc_project::State::server;
using namespace phosphor::logging;
using sdbusplus::exception::SdBusError;

constexpr auto propertyIface = "org.freedesktop.DBus.Properties";
constexpr auto chassisIface = "xyz.openbmc_project.State.Chassis";
constexpr auto hostIface = "xyz.openbmc_project.State.Host";
constexpr auto powerButtonIface = "xyz.openbmc_project.Chassis.Buttons.Power";
constexpr auto resetButtonIface = "xyz.openbmc_project.Chassis.Buttons.Reset";
constexpr auto mapperIface = "xyz.openbmc_project.ObjectMapper";

constexpr auto mapperObjPath = "/xyz/openbmc_project/object_mapper";
constexpr auto mapperService = "xyz.openbmc_project.ObjectMapper";

Handler::Handler(sdbusplus::bus::bus& bus) :
    bus(bus), chassisPath(std::string{CHASSIS_OBJPATH} + '0'),
    hostPath(std::string{HOST_OBJPATH} + '0')
{
    if (!getService(POWER_BUTTON_OBJ_PATH, powerButtonIface).empty())
    {
        powerButtonRelease = std::make_unique<sdbusplus::bus::match_t>(
            bus,
            sdbusRule::type::signal() + sdbusRule::member("Released") +
                sdbusRule::path(POWER_BUTTON_OBJ_PATH) +
                sdbusRule::interface(powerButtonIface),
            std::bind(std::mem_fn(&Handler::powerPress), this,
                      std::placeholders::_1));

        powerButtonLongPressRelease = std::make_unique<sdbusplus::bus::match_t>(
            bus,
            sdbusRule::type::signal() + sdbusRule::member("PressedLong") +
                sdbusRule::path(POWER_BUTTON_OBJ_PATH) +
                sdbusRule::interface(powerButtonIface),
            std::bind(std::mem_fn(&Handler::longPowerPress), this,
                      std::placeholders::_1));
    }

    if (!getService(RESET_BUTTON_OBJ_PATH, resetButtonIface).empty())
    {
        resetButtonRelease = std::make_unique<sdbusplus::bus::match_t>(
            bus,
            sdbusRule::type::signal() + sdbusRule::member("Released") +
                sdbusRule::path(RESET_BUTTON_OBJ_PATH) +
                sdbusRule::interface(resetButtonIface),
            std::bind(std::mem_fn(&Handler::resetPress), this,
                      std::placeholders::_1));
    }
}

std::string Handler::getService(const std::string& path,
                                const std::string& interface) const
{
    try
    {
        auto method = bus.new_method_call(mapperService, mapperObjPath,
                                          mapperIface, "GetObject");
        method.append(path, std::vector{interface});
        auto result = bus.call(method);

        std::map<std::string, std::vector<std::string>> objectData;
        result.read(objectData);

        return objectData.begin()->first;
    }
    catch (SdBusError& e)
    {
    }

    return std::string{};
}

bool Handler::poweredOn() const
{
    auto method = bus.new_method_call(CHASSIS_BUSNAME, chassisPath.c_str(),
                                      propertyIface, "Get");
    method.append(chassisIface, "CurrentPowerState");
    auto result = bus.call(method);

    sdbusplus::message::variant<std::string> state;
    result.read(state);

    return Chassis::PowerState::On ==
           Chassis::convertPowerStateFromString(state.get<std::string>());
}

void Handler::powerPress(sdbusplus::message::message& msg)
{
    auto transition = Host::Transition::On;

    try
    {
        if (poweredOn())
        {
            transition = Host::Transition::Off;
        }

        log<level::INFO>("Handling power button press");

        sdbusplus::message::variant<std::string> state =
            convertForMessage(transition);

        auto method = bus.new_method_call(HOST_BUSNAME, hostPath.c_str(),
                                          propertyIface, "Set");
        method.append(hostIface, "RequestedHostTransition", state);

        bus.call(method);
    }
    catch (SdBusError& e)
    {
        log<level::ERR>("Failed power state change on a power button press",
                        entry("ERROR=%s", e.what()));
    }
}

void Handler::longPowerPress(sdbusplus::message::message& msg)
{
    try
    {
        if (!poweredOn())
        {
            log<level::INFO>(
                "Power is off so ignoring long power button press");
            return;
        }

        log<level::INFO>("Handling long power button press");

        sdbusplus::message::variant<std::string> state =
            convertForMessage(Chassis::Transition::Off);

        auto method = bus.new_method_call(CHASSIS_BUSNAME, chassisPath.c_str(),
                                          propertyIface, "Set");
        method.append(chassisIface, "RequestedPowerTransition", state);

        bus.call(method);
    }
    catch (SdBusError& e)
    {
        log<level::ERR>("Failed powering off on long power button press",
                        entry("ERROR=%s", e.what()));
    }
}

void Handler::resetPress(sdbusplus::message::message& msg)
{
    try
    {
        if (!poweredOn())
        {
            log<level::INFO>("Power is off so ignoring reset button press");
            return;
        }

        log<level::INFO>("Handling reset button press");

        sdbusplus::message::variant<std::string> state =
            convertForMessage(Host::Transition::Reboot);

        auto method = bus.new_method_call(HOST_BUSNAME, hostPath.c_str(),
                                          propertyIface, "Set");

        method.append(hostIface, "RequestedHostTransition", state);

        bus.call(method);
    }
    catch (SdBusError& e)
    {
        log<level::ERR>("Failed power state change on a reset button press",
                        entry("ERROR=%s", e.what()));
    }
}

} // namespace button
} // namespace state
} // namespace phosphor
