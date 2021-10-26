#pragma once

#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/object.hpp>
#include <xyz/openbmc_project/Condition/HostFirmware/server.hpp>

#include <iostream>

namespace phosphor
{
namespace condition
{

using HostIntf = sdbusplus::server::object::object<
    sdbusplus::xyz::openbmc_project::Condition::server::HostFirmware>;

class Host : public HostIntf
{
  public:
    Host() = delete;
    Host(const Host&) = delete;
    Host& operator=(const Host&) = delete;
    Host(Host&&) = delete;
    Host& operator=(Host&&) = delete;
    virtual ~Host() = default;

    Host(sdbusplus::bus::bus& bus, const std::string& path) :
        HostIntf(bus, path.c_str())
    {
        scanGpioPin();
    };

    /** @brief Override reads to CurrentFirmwareCondition */
    FirmwareCondition currentFirmwareCondition() const override;

  private:
    std::string lineName;
    bool isActHigh;

    /*
     * Scan gpio pin to detect the name and active state
     */
    void scanGpioPin();
};
} // namespace condition
} // namespace phosphor
