#include <systemd_service_parser.hpp>
#include <systemd_target_parser.hpp>

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <iostream>

#include <gtest/gtest.h>

namespace fs = std::filesystem;

// Enable debug by default for debug when needed
bool gVerbose = true;

TEST(TargetJsonParser, BasicGoodPath)
{
    auto defaultData1 = R"(
        {
            "targets" : {
                "multi-user.target" : {
                    "errorsToMonitor": ["default"],
                    "errorToLog": "xyz.openbmc_project.State.BMC.Error.MultiUserTargetFailure"},
                "obmc-chassis-poweron@0.target" : {
                    "errorsToMonitor": ["timeout", "failed"],
                    "errorToLog": "xyz.openbmc_project.State.Chassis.Error.PowerOnTargetFailure"}
                }
        }
    )"_json;

    auto defaultData2 = R"(
        {
            "targets" : {
                "obmc-host-start@0.target" : {
                    "errorsToMonitor": ["default"],
                    "errorToLog": "xyz.openbmc_project.State.Host.Error.HostStartFailure"},
                "obmc-host-stop@0.target" : {
                    "errorsToMonitor": ["dependency"],
                    "errorToLog": "xyz.openbmc_project.State.Host.Error.HostStopFailure"}
                }
        }
    )"_json;

    std::FILE* tmpf = fopen("/tmp/good_file1.json", "w");
    std::fputs(defaultData1.dump().c_str(), tmpf);
    std::fclose(tmpf);

    tmpf = fopen("/tmp/good_file2.json", "w");
    std::fputs(defaultData2.dump().c_str(), tmpf);
    std::fclose(tmpf);

    std::vector<std::string> filePaths;
    filePaths.emplace_back("/tmp/good_file1.json");
    filePaths.emplace_back("/tmp/good_file2.json");

    TargetErrorData targetData = parseFiles(filePaths);

    EXPECT_EQ(targetData.size(), 4);
    EXPECT_NE(targetData.find("multi-user.target"), targetData.end());
    EXPECT_NE(targetData.find("obmc-chassis-poweron@0.target"),
              targetData.end());
    EXPECT_NE(targetData.find("obmc-host-start@0.target"), targetData.end());
    EXPECT_NE(targetData.find("obmc-host-stop@0.target"), targetData.end());
    targetEntry tgt = targetData["obmc-chassis-poweron@0.target"];
    EXPECT_EQ(tgt.errorToLog,
              "xyz.openbmc_project.State.Chassis.Error.PowerOnTargetFailure");
    EXPECT_EQ(tgt.errorsToMonitor.size(), 2);
    // Check a target with "default" for errorsToMonitor, should have 3 defaults
    tgt = targetData["obmc-host-start@0.target"];
    EXPECT_EQ(tgt.errorsToMonitor.size(), 3);

    std::remove("/tmp/good_file1.json");
    std::remove("/tmp/good_file2.json");
}

TEST(TargetJsonParser, InvalidErrorToMonitor)
{
    auto invalidDataError = R"(
        {
            "targets" : {
                "obmc-chassis-poweron@0.target" : {
                    "errorsToMonitor": ["timeout", "invalid"],
                    "errorToLog": "xyz.openbmc_project.State.Chassis.Error.PowerOnTargetFailure"}
                }
        }
    )"_json;

    std::FILE* tmpf = fopen("/tmp/invalid_error_file.json", "w");
    std::fputs(invalidDataError.dump().c_str(), tmpf);
    std::fclose(tmpf);

    std::vector<std::string> filePaths;
    filePaths.emplace_back("/tmp/invalid_error_file.json");

    // Verify exception thrown on invalid errorsToMonitor
    EXPECT_THROW(TargetErrorData targetData = parseFiles(filePaths),
                 std::out_of_range);
    std::remove("/tmp/invalid_error_file.json");
}

TEST(TargetJsonParser, InvalidFileFormat)
{
    std::FILE* tmpf = fopen("/tmp/invalid_json_file.json", "w");
    std::fputs(R"({"targets":{"missing closing quote}})", tmpf);
    fclose(tmpf);

    std::vector<std::string> filePaths;
    filePaths.emplace_back("/tmp/invalid_json_file.json");

    // Verify exception thrown on invalid json file format
    EXPECT_THROW(TargetErrorData targetData = parseFiles(filePaths),
                 nlohmann::detail::parse_error);
    std::remove("/tmp/invalid_json_file.json");
}

TEST(TargetJsonParser, NotJustDefault)
{
    auto notJustDefault = R"(
        {
            "targets" : {
                "obmc-chassis-poweron@0.target" : {
                    "errorsToMonitor": ["timeout", "default"],
                    "errorToLog": "xyz.openbmc_project.State.Chassis.Error.PowerOnTargetFailure"}
                }
        }
    )"_json;

    std::FILE* tmpf = fopen("/tmp/not_just_default_file.json", "w");
    std::fputs(notJustDefault.dump().c_str(), tmpf);
    std::fclose(tmpf);

    std::vector<std::string> filePaths;
    filePaths.emplace_back("/tmp/not_just_default_file.json");

    // Verify exception thrown on invalid errorsToMonitor
    EXPECT_THROW(TargetErrorData targetData = parseFiles(filePaths),
                 std::invalid_argument);
    std::remove("/tmp/not_just_default_file.json");
}

TEST(ServiceJsonParser, ImportantServices)
{
    auto serviceData = R"(
        {
            "services" : ["bmcweb.service"],
            "important_services" : [
                "phosphor-ipmi-net@*.service",
                {
                    "service": "phosphor-ipmi-host.service",
                    "start_units": ["ipmid-recover.service", "foo.target"]
                },
                {"service": "bar.service"}
            ]
        }
    )"_json;

    std::FILE* tmpf = fopen("/tmp/important_services.json", "w");
    std::fputs(serviceData.dump().c_str(), tmpf);
    std::fclose(tmpf);

    auto result = parseServiceFiles({"/tmp/important_services.json"});

    EXPECT_EQ(result.services.size(), 1);
    ASSERT_EQ(result.importantServices.size(), 3);
    EXPECT_EQ(result.importantServices[0].service,
              "phosphor-ipmi-net@*.service");
    EXPECT_TRUE(result.importantServices[0].startUnits.empty());
    EXPECT_EQ(result.importantServices[1].service,
              "phosphor-ipmi-host.service");
    EXPECT_EQ(
        result.importantServices[1].startUnits,
        (std::vector<std::string>{"ipmid-recover.service", "foo.target"}));
    EXPECT_EQ(result.importantServices[2].service, "bar.service");
    EXPECT_TRUE(result.importantServices[2].startUnits.empty());

    std::remove("/tmp/important_services.json");
}
