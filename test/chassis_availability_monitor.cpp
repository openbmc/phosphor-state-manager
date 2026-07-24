#include "chassis_availability_monitor.hpp"

#include <sdbusplus/bus.hpp>

#include <cstdio>
#include <cstdlib>

#include <gtest/gtest.h>

namespace phosphor
{
namespace state
{
namespace manager
{

class ChassisAvailabilityTest : public ::testing::Test
{
  protected:
    sdbusplus::bus_t bus = sdbusplus::bus::new_default();
    std::string testFile;

    void createTestConfig(const json& config, const std::string& filename)
    {
        testFile = "/tmp/" + filename;
        std::FILE* tmpf = fopen(testFile.c_str(), "w");
        std::fputs(config.dump().c_str(), tmpf);
        std::fclose(tmpf);
    }

    void TearDown() override
    {
        if (!testFile.empty())
        {
            std::remove(testFile.c_str());
        }
    }
};

TEST_F(ChassisAvailabilityTest, BasicGoodPath)
{
    auto validConfig = R"(
        {
            "availableObjectPath":
                "/xyz/openbmc_project/inventory/system/chassis<N>",
            "conditions": [
                {
                    "baseObjectPath":
                        "/xyz/openbmc_project/inventory/system/chassis<N>",
                    "interface": "xyz.openbmc_project.Inventory.Item",
                    "property": "Present",
                    "availableValue": true
                },
                {
                    "baseObjectPath":
                        "/xyz/openbmc_project/inventory/system/chassis<N>",
                    "interface":
                        "xyz.openbmc_project.State.Decorator.PowerSystemInputs",
                    "property": "Status",
                    "availableValue":
                        "xyz.openbmc_project.State.Decorator.PowerSystemInputs.Status.Good"
                },
                {
                    "baseObjectPath":
                        "/xyz/openbmc_project/inventory/system/chassis<N>",
                    "interface": "xyz.openbmc_project.Common.Progress",
                    "property": "PercentComplete",
                    "availableValue": 100
                }
            ]
        }
    )"_json;

    createTestConfig(validConfig, "chassis_availability_good.json");

    EXPECT_NO_THROW(ChassisAvailability monitor(bus, testFile));
}

TEST_F(ChassisAvailabilityTest, MissingAvailableObjectPath)
{
    auto missingPath = R"(
        {
            "conditions": [
                {
                    "baseObjectPath":
                        "/xyz/openbmc_project/inventory/system/chassis<N>",
                    "interface": "xyz.openbmc_project.Inventory.Item",
                    "property": "Present",
                    "availableValue": true
                }
            ]
        }
    )"_json;

    createTestConfig(missingPath, "chassis_availability_missing_path.json");

    EXPECT_THROW(ChassisAvailability monitor(bus, testFile), std::exception);
}

TEST_F(ChassisAvailabilityTest, InvalidAvailableValueType)
{
    auto invalidType = R"(
        {
            "availableObjectPath":
                "/xyz/openbmc_project/inventory/system/chassis<N>",
            "conditions": [
                {
                    "baseObjectPath":
                        "/xyz/openbmc_project/inventory/system/chassis<N>",
                    "interface": "xyz.openbmc_project.Inventory.Item",
                    "property": "Present",
                    "availableValue": ["invalid", "array", "type"]
                }
            ]
        }
    )"_json;

    createTestConfig(invalidType, "chassis_availability_invalid_type.json");

    EXPECT_THROW(ChassisAvailability monitor(bus, testFile),
                 std::invalid_argument);
}

TEST_F(ChassisAvailabilityTest, InvalidJsonFormat)
{
    testFile = "/tmp/chassis_availability_invalid_json.json";
    std::FILE* tmpf = fopen(testFile.c_str(), "w");
    std::fputs(R"({"availableObjectPath":"missing closing brace")", tmpf);
    std::fclose(tmpf);

    // Exception thrown on malformed JSON
    EXPECT_THROW(ChassisAvailability monitor(bus, testFile),
                 nlohmann::detail::parse_error);
}

TEST_F(ChassisAvailabilityTest, FileNotFound)
{
    // Exception thrown when config file doesn't exist
    EXPECT_THROW(ChassisAvailability monitor(bus, "/tmp/nonexistent.json"),
                 std::runtime_error);
}

} // namespace manager
} // namespace state
} // namespace phosphor
