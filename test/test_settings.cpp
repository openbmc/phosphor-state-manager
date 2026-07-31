#include "../settings.hpp"

#include <sdbusplus/message.hpp>
#include <sdbusplus/test/sdbus_mock.hpp>
#include <xyz/openbmc_project/Common/error.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <map>
#include <string>
#include <vector>

namespace settings
{

using sdbusplus::xyz::openbmc_project::Common::Error::InternalFailure;
using ::testing::_;

using Interfaces = std::vector<Interface>;
using MapperResponse = std::map<Path, std::map<Service, Interfaces>>;
using MapperObjectResponse = std::map<Service, Interfaces>;

namespace
{

void parseMapperPaths(const MapperResponse& result, Path& autoReboot,
                      Path& autoRebootOneTime, Path& powerRestorePolicy,
                      Path& powerRestorePolicyOneTime)
{
    for (const auto& iter : result)
    {
        const Path& path = iter.first;

        for (const auto& serviceIter : iter.second)
        {
            for (const auto& interface : serviceIter.second)
            {
                if (autoRebootIntf == interface)
                {
                    if (path.find("one_time") != std::string::npos)
                    {
                        autoRebootOneTime = path;
                    }
                    else
                    {
                        autoReboot = path;
                    }
                }
                else if (powerRestoreIntf == interface)
                {
                    if (path.find("one_time") != std::string::npos)
                    {
                        powerRestorePolicyOneTime = path;
                    }
                    else
                    {
                        powerRestorePolicy = path;
                    }
                }
            }
        }
    }
}

} // namespace

class SettingsTest : public ::testing::Test
{
  protected:
    sdbusplus::SdBusMock sdbusMock;
    sdbusplus::bus_t mockedBus = sdbusplus::get_mocked_new(&sdbusMock);
};

TEST_F(SettingsTest, EmptyMapperSubTreeThrows)
{
    EXPECT_CALL(sdbusMock, sd_bus_call(_, _, _, _, _))
        .WillRepeatedly([](sd_bus* /*bus*/, sd_bus_message* /*message*/,
                           uint64_t /*timeout*/, sd_bus_error* /*error*/,
                           sd_bus_message** /*reply*/) { return -1; });

    EXPECT_THROW((void)Objects(mockedBus), InternalFailure);
}

TEST(SettingsMapperLogic, DetectsOneTimeAndPersistentPaths)
{
    MapperResponse payload = {
        {"/xyz/openbmc_project/control/host0/auto_reboot",
         {{"xyz.openbmc_project.Settings", {autoRebootIntf}}}},
        {"/xyz/openbmc_project/control/host0/auto_reboot/one_time",
         {{"xyz.openbmc_project.Settings", {autoRebootIntf}}}},
        {"/xyz/openbmc_project/control/host0/power_restore_policy",
         {{"xyz.openbmc_project.Settings", {powerRestoreIntf}}}},
        {"/xyz/openbmc_project/control/host0/power_restore_policy/one_time",
         {{"xyz.openbmc_project.Settings", {powerRestoreIntf}}}},
    };

    Path autoReboot;
    Path autoRebootOneTime;
    Path powerRestorePolicy;
    Path powerRestorePolicyOneTime;
    parseMapperPaths(payload, autoReboot, autoRebootOneTime, powerRestorePolicy,
                     powerRestorePolicyOneTime);

    EXPECT_EQ(autoReboot,
              "/xyz/openbmc_project/control/host0/auto_reboot");
    EXPECT_EQ(autoRebootOneTime,
              "/xyz/openbmc_project/control/host0/auto_reboot/one_time");
    EXPECT_EQ(powerRestorePolicy,
              "/xyz/openbmc_project/control/host0/power_restore_policy");
    EXPECT_EQ(powerRestorePolicyOneTime,
              "/xyz/openbmc_project/control/host0/power_restore_policy/one_time");
}

TEST(SettingsMapperLogic, ServiceReturnsSingleProvider)
{
    MapperObjectResponse payload = {
        {"xyz.openbmc_project.Settings", {autoRebootIntf}}};
    EXPECT_EQ(payload.begin()->first,
              "xyz.openbmc_project.Settings");
}

TEST(SettingsMapperLogic, ServiceWithMultipleProvidersReturnsFirstEntry)
{
    MapperObjectResponse payload = {
        {"xyz.openbmc_project.Settings.B", {autoRebootIntf}},
        {"xyz.openbmc_project.Settings.A", {autoRebootIntf}}};
    EXPECT_EQ(payload.begin()->first,
              "xyz.openbmc_project.Settings.A");
}

} // namespace settings
