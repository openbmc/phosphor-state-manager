#include "config.h"

#include "utils.hpp"

#include <phosphor-logging/lg2.hpp>

#include <filesystem>
#include <fstream>
#include <string>

PHOSPHOR_LOG2_USING;

constexpr auto PROPERTY_INTERFACE = "org.freedesktop.DBus.Properties";

// Utilize the QuiesceOnHwError setting as an indication that the system
// is operating in an environment where the user should be notified of
// security settings (i.e. "Manufacturing")
bool isMfgModeEnabled()
{
    auto bus = sdbusplus::bus::new_default();
    std::string path = "/xyz/openbmc_project/logging/settings";
    std::string interface = "xyz.openbmc_project.Logging.Settings";
    std::string propertyName = "QuiesceOnHwError";
    std::variant<bool> mfgModeEnabled;

    std::string service =
        phosphor::state::manager::utils::getService(bus, path, interface);

    auto method = bus.new_method_call(service.c_str(), path.c_str(),
                                      PROPERTY_INTERFACE, "Get");

    method.append(interface, propertyName);

    try
    {
        auto reply = bus.call(method);
        reply.read(mfgModeEnabled);
    }
    catch (const sdbusplus::exception::exception& e)
    {
        error("Error in property Get, error {ERROR}, property {PROPERTY}",
              "ERROR", e, "PROPERTY", propertyName);
        throw;
    }

    return std::get<bool>(mfgModeEnabled);
}

int main()
{
    // Read the secure boot gpio
    auto secureBootGpio =
        phosphor::state::manager::utils::getGpioValue("bmc-secure-boot");
    if (secureBootGpio == -1)
    {
        debug("bmc-secure-boot gpio not present or can not be read");
    }
    else if (secureBootGpio == 0)
    {
        info("bmc-secure-boot gpio found and indicates it is NOT enabled");
    }
    else
    {
        info("bmc-secure-boot found and indicates it is enabled");
    }

    // Now read the /sys/kernel/debug/aspeed/ files
    std::string dbgVal;
    std::ifstream dbgFile;
    int secureBootVal = -1;
    int abrImage = -1;

    dbgFile.exceptions(std::ifstream::failbit | std::ifstream::badbit |
                       std::ifstream::eofbit);

    if (std::filesystem::exists(SYSFS_SECURE_BOOT_PATH))
    {
        try
        {
            dbgFile.open(SYSFS_SECURE_BOOT_PATH);
            dbgFile >> dbgVal;
            dbgFile.close();
            info("Read {SECURE_BOOT_VAL} from secure_boot", "SECURE_BOOT_VAL",
                 dbgVal);
            secureBootVal = std::stoi(dbgVal);
        }
        catch (std::exception& e)
        {
            error("Failed to read secure boot sysfs file: {ERROR}", "ERROR", e);
            // just continue and error will be logged at end if in mfg mode
        }
    }
    else
    {
        info("sysfs file secure_boot not present");
    }

    if (std::filesystem::exists(SYSFS_ABR_IMAGE_PATH))
    {

        try
        {
            dbgFile.open(SYSFS_ABR_IMAGE_PATH);
            dbgFile >> dbgVal;
            dbgFile.close();
            info("Read {ABR_IMAGE_VAL} from abr_image", "ABR_IMAGE_VAL",
                 dbgVal);
            abrImage = std::stoi(dbgVal);
        }
        catch (std::exception& e)
        {
            error("Failed to read abr image sysfs file: {ERROR}", "ERROR", e);
            // just continue and error will be logged at end if in mfg mode
        }
    }
    else
    {
        info("sysfs file abr_image not present");
    }

    if (isMfgModeEnabled())
    {
        if ((secureBootGpio != 1) || (secureBootVal != 1) || (abrImage != 0))
        {
            // TODO - Generate Error when in mfg mode
            error("The system is not secure");
        }
    }

    return 0;
}
