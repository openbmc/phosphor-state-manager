#include "drive_state_manager.hpp"
#include "sub_device_manager.hpp"

#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/manager.hpp>

#include <map>
#include <memory>
#include <string>

PHOSPHOR_LOG2_USING;

using DriveState = sdbusplus::server::xyz::openbmc_project::state::Drive;

constexpr auto ITEM_DRIVE_INTERFACE =
    "xyz.openbmc_project.Inventory.Item.Drive";
constexpr auto MANAGED_HOST_INTERFACE =
    "xyz.openbmc_project.Inventory.Decorator.ManagedHost";

/** @brief Extract drive instance name from entity-manager object path.
 *
 *  Uses the Board entity "Name" property from the Item.Drive interface if
 *  available, otherwise falls back to the last path component.
 */
static std::string extractInstanceName(
    const std::string& objectPath,
    const phosphor::state::manager::DbusInterfaceMap& interfaces)
{
    // Try to get the Name from the Item.Drive interface properties
    auto itemIt = interfaces.find(ITEM_DRIVE_INTERFACE);
    if (itemIt != interfaces.end())
    {
        auto nameIt = itemIt->second.find("Name");
        if (nameIt != itemIt->second.end())
        {
            const auto* nameStr = std::get_if<std::string>(&nameIt->second);
            if (nameStr != nullptr && !nameStr->empty())
            {
                return *nameStr;
            }
        }
    }

    // Fallback: use last path component
    sdbusplus::object_path path(objectPath);
    return path.filename();
}

/** @brief Extract HostIndex from ManagedHost interface if present. */
static std::optional<size_t> extractHostId(
    const phosphor::state::manager::DbusInterfaceMap& interfaces)
{
    auto hostIt = interfaces.find(MANAGED_HOST_INTERFACE);
    if (hostIt == interfaces.end())
    {
        return std::nullopt;
    }

    auto indexIt = hostIt->second.find("HostIndex");
    if (indexIt == hostIt->second.end())
    {
        return std::nullopt;
    }

    // HostIndex can be string or uint64_t in entity-manager schema
    if (const auto* val = std::get_if<uint64_t>(&indexIt->second))
    {
        return static_cast<size_t>(*val);
    }
    if (const auto* val = std::get_if<std::string>(&indexIt->second))
    {
        try
        {
            return std::stoul(*val);
        }
        catch (const std::exception& e)
        {
            error("Failed to parse HostIndex '{VALUE}': {ERROR}", "VALUE", *val,
                  "ERROR", e);
        }
    }

    return std::nullopt;
}

int main()
{
    auto bus = sdbusplus::bus::new_default();

    auto driveBusName = std::string(DriveState::interface);
    const auto* objPath = DriveState::namespace_path::value;

    // Add sdbusplus ObjectManager.
    sdbusplus::server::manager_t objManager(bus, objPath);

    // Map of inventory path -> Drive state manager instance
    std::map<std::string, std::unique_ptr<phosphor::state::manager::Drive>>
        drives;

    // Set up entity-manager discovery for Item.Drive
    phosphor::state::manager::SubDeviceManager emWatcher(
        bus, ITEM_DRIVE_INTERFACE,
        // Device added callback
        [&bus, &drives](
            const std::string& inventoryPath,
            const phosphor::state::manager::DbusInterfaceMap& interfaces) {
            if (drives.contains(inventoryPath))
            {
                return;
            }

            auto instanceName = extractInstanceName(inventoryPath, interfaces);
            auto hostId = extractHostId(interfaces);

            auto drivePath =
                sdbusplus::object_path(DriveState::namespace_path::value) /
                std::string(DriveState::namespace_path::drive) / instanceName;

            info("Creating drive state object {PATH} for inventory {INV}",
                 "PATH", drivePath.str, "INV", inventoryPath);

            drives.emplace(inventoryPath,
                           std::make_unique<phosphor::state::manager::Drive>(
                               bus, drivePath.str.c_str(), instanceName,
                               inventoryPath, hostId));
        },
        // Device removed callback
        [&drives](const std::string& inventoryPath) {
            auto it = drives.find(inventoryPath);
            if (it != drives.end())
            {
                info("Removing drive state object for inventory {INV}", "INV",
                     inventoryPath);
                drives.erase(it);
            }
        });

    // Scan for devices that entity-manager already published before we started
    emWatcher.scanExistingDevices();

    bus.request_name(driveBusName.c_str());

    while (true)
    {
        bus.process_discard();
        bus.wait();
    }

    return 0;
}
