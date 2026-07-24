#include "chassis_availability_monitor.hpp"

#include <nlohmann/json.hpp>
#include <sdbusplus/bus.hpp>

#include <cstdio>
#include <cstdlib>
#include <fstream>

#include <gtest/gtest.h>

namespace phosphor::state::manager
{

using json = nlohmann::json;

class ChassisAvailabilityTest : public ::testing::Test
{
  protected:
    sdbusplus::bus_t bus = sdbusplus::bus::new_default();
    std::filesystem::path testDir;
    std::filesystem::path testFile;

    void createTestConfig(const json& config, const std::string& filename)
    {
        char tmpDir[] = "/tmp/test_chassis_availability_monitor_XXXXXX";
        ASSERT_NE(mkdtemp(tmpDir), nullptr);

        testDir = std::filesystem::path(tmpDir);
        testFile = testDir / filename;

        std::ofstream stream(testFile);
        ASSERT_TRUE(stream.is_open());
        stream << config;
    }

    void TearDown() override
    {
        if (!testDir.empty())
        {
            std::filesystem::remove_all(testDir);
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
    std::ofstream stream(testFile);
    ASSERT_TRUE(stream.is_open());
    stream << R"({"availableObjectPath":"missing closing brace")";
    stream.close();

    // Exception thrown on malformed JSON
    EXPECT_THROW(ChassisAvailability monitor(bus, testFile),
                 nlohmann::detail::parse_error);
}

TEST_F(ChassisAvailabilityTest, FileNotFound)
{
    testFile = "/tmp/nonexistent.json";

    // Exception thrown when config file doesn't exist
    EXPECT_THROW(ChassisAvailability monitor(bus, testFile),
                 std::runtime_error);
}

} // namespace phosphor::state::manager
