#include "sub_device_manager.hpp"

#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/message.hpp>

#include <map>
#include <string>
#include <vector>

namespace phosphor
{
namespace state
{
namespace manager
{

PHOSPHOR_LOG2_USING;

constexpr auto ENTITY_MANAGER_SERVICE = "xyz.openbmc_project.EntityManager";
constexpr auto ENTITY_MANAGER_PATH = "/xyz/openbmc_project/inventory";
constexpr auto DBUS_OBJECT_MANAGER_INTERFACE =
    "org.freedesktop.DBus.ObjectManager";

SubDeviceManager::SubDeviceManager(
    sdbusplus::bus_t& bus, const std::string& itemInterface,
    DeviceAddedCallback addedCb, DeviceRemovedCallback removedCb) :
    bus(bus), itemInterface(itemInterface), addedCallback(std::move(addedCb)),
    removedCallback(std::move(removedCb)),
    interfacesAddedMatch(
        bus,
        sdbusRule::interfacesAdded() +
            sdbusRule::sender(ENTITY_MANAGER_SERVICE),
        [this](sdbusplus::message_t& m) { interfacesAdded(m); }),
    interfacesRemovedMatch(
        bus,
        sdbusRule::interfacesRemoved() +
            sdbusRule::sender(ENTITY_MANAGER_SERVICE),
        [this](sdbusplus::message_t& m) { interfacesRemoved(m); })
{}

void SubDeviceManager::scanExistingDevices()
{
    auto method =
        bus.new_method_call(ENTITY_MANAGER_SERVICE, ENTITY_MANAGER_PATH,
                            DBUS_OBJECT_MANAGER_INTERFACE, "GetManagedObjects");

    try
    {
        auto reply = bus.call(method);
        auto managedObjects =
            reply.unpack<std::map<sdbusplus::object_path, DbusInterfaceMap>>();

        for (const auto& [objectPath, interfaces] : managedObjects)
        {
            if (interfaces.contains(itemInterface))
            {
                info("Found existing device at {PATH}", "PATH", objectPath.str);
                addedCallback(objectPath.str, interfaces);
            }
        }
    }
    catch (const sdbusplus::exception_t& e)
    {
        // entity-manager may not be running yet; devices will be picked
        // up via InterfacesAdded signals when it starts
        info("Unable to query entity-manager: {ERROR}", "ERROR", e);
        return;
    }
}

void SubDeviceManager::interfacesAdded(sdbusplus::message_t& msg)
{
    auto [objectPath,
          interfaces] = msg.unpack<sdbusplus::object_path, DbusInterfaceMap>();

    if (interfaces.contains(itemInterface))
    {
        info("Device added at {PATH}", "PATH", objectPath.str);
        addedCallback(objectPath.str, interfaces);
    }
}

void SubDeviceManager::interfacesRemoved(sdbusplus::message_t& msg)
{
    auto [objectPath, interfaces] =
        msg.unpack<sdbusplus::object_path, std::vector<std::string>>();

    for (const auto& iface : interfaces)
    {
        if (iface == itemInterface)
        {
            info("Device removed at {PATH}", "PATH", objectPath.str);
            removedCallback(objectPath.str);
            return;
        }
    }
}

} // namespace manager
} // namespace state
} // namespace phosphor
