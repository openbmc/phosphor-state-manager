#include "config.h"

#include "utils.hpp"

#include <phosphor-logging/lg2.hpp>
#include <xyz/openbmc_project/Logging/Settings/client.hpp>

#include <filesystem>
#include <fstream>
#include <string>

PHOSPHOR_LOG2_USING;

constexpr auto PROPERTY_INTERFACE = "org.freedesktop.DBus.Properties";
using LoggingSettings =
    sdbusplus::client::xyz::openbmc_project::logging::Settings<>;

// Check if the TPM measurement file exists and has a valid value.
// If the TPM measurement is invalid, it logs an error message.
void checkTpmMeasurement()
{
    bool tpmError = false;
    std::string errorMsg;
    if (!std::filesystem::exists(std::string(SYSFS_TPM_MEASUREMENT_PATH)))
    {
        tpmError = true;
        errorMsg = "TPM measurement file does not exist: " +
                   std::string(SYSFS_TPM_MEASUREMENT_PATH);
    }
    else
    {
        std::string tpmValueStr;
        std::ifstream tpmFile(std::string(SYSFS_TPM_MEASUREMENT_PATH));

        tpmFile >> tpmValueStr;
        if (tpmValueStr.empty())
        {
            tpmError = true;
            errorMsg = "TPM measurement value is empty: " +
                       std::string(SYSFS_TPM_MEASUREMENT_PATH);
        }
        else if (tpmValueStr == "0")
        {
            tpmError = true;
            errorMsg = "TPM measurement value is 0: " +
                       std::string(SYSFS_TPM_MEASUREMENT_PATH);
        }
        tpmFile.close();
    }

    if (tpmError)
    {
        // Doesn't have valid TPM measurement, log an error message
        std::map<std::string, std::string> additionalData;
        error("{ERROR}", "ERROR", errorMsg);
        additionalData.emplace("ERROR", errorMsg);
        auto bus = sdbusplus::bus::new_default();
        phosphor::state::manager::utils::createError(
            bus, "xyz.openbmc_project.State.Error.TpmMeasurementFail",
            sdbusplus::server::xyz::openbmc_project::logging::Entry::Level::
                Error,
            additionalData);
    }
    return;
}

// Utilize the QuiesceOnHwError setting as an indication that the system
// is operating in an environment where the user should be notified of
// security settings (i.e. "Manufacturing")
bool isMfgModeEnabled()
{
    auto bus = sdbusplus::bus::new_default();
    std::string path = "/xyz/openbmc_project/logging/settings";
    std::string interface = LoggingSettings::interface;
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
    catch (const sdbusplus::exception_t& e)
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

    dbgFile.exceptions(
        std::ifstream::failbit | std::ifstream::badbit | std::ifstream::eofbit);

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
            error("The system is not secure");
            std::map<std::string, std::string> additionalData;
            additionalData.emplace("SECURE_BOOT_GPIO",
                                   std::to_string(secureBootGpio));
            additionalData.emplace("SYSFS_SECURE_BOOT_VAL",
                                   std::to_string(secureBootVal));
            additionalData.emplace("SYSFS_ABR_IMAGE_VAL",
                                   std::to_string(abrImage));

            auto bus = sdbusplus::bus::new_default();
            phosphor::state::manager::utils::createError(
                bus, "xyz.openbmc_project.State.Error.SecurityCheckFail",
                sdbusplus::server::xyz::openbmc_project::logging::Entry::Level::
                    Warning,
                additionalData);
        }
    }

    // Check the TPM measurement if TPM is enabled
    if (std::filesystem::exists(std::string(SYSFS_TPM_DEVICE_PATH)))
    {
        checkTpmMeasurement();
    }

    return 0;
}
