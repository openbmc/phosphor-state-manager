#include <sdbusplus/bus.hpp>
#include <sdeventplus/event.hpp>
#include <systemd_target_signal.hpp>

#include <iostream>

#include <gtest/gtest.h>

// Enable debug by default for debug when needed
bool gVerbose = true;

TEST(TargetSignalData, BasicPaths)
{

    // Create default data structure for testing
    TargetErrorData targetData = {
        {"multi-user.target",
         {"xyz.openbmc_project.State.BMC.Error.MultiUserTargetFailure",
          {"default"}}},
        {"obmc-chassis-poweron@0.target",
         {"xyz.openbmc_project.State.Chassis.Error.PowerOnTargetFailure",
          {"timeout", "failed"}}}};

    ServiceMonitorData serviceData = {
        "xyz.openbmc_project.biosconfig_manager.service",
        "xyz.openbmc_project.Dump.Manager.service"};

    auto bus = sdbusplus::bus::new_default();
    auto event = sdeventplus::Event::get_default();
    bus.attach_event(event.get(), SD_EVENT_PRIORITY_NORMAL);

    phosphor::state::manager::SystemdTargetLogging targetMon(targetData,
                                                             serviceData, bus);

    std::string invalidUnit = "invalid_unit";
    std::string validError = "timeout";
    std::string errorToLog = targetMon.processError(invalidUnit, validError);
    EXPECT_TRUE(errorToLog.empty());

    std::string validUnit = "obmc-chassis-poweron@0.target";
    std::string invalidError = "invalid_error";
    errorToLog = targetMon.processError(validUnit, invalidError);
    EXPECT_TRUE(errorToLog.empty());

    errorToLog = targetMon.processError(validUnit, validError);
    EXPECT_FALSE(errorToLog.empty());
    EXPECT_EQ(errorToLog,
              "xyz.openbmc_project.State.Chassis.Error.PowerOnTargetFailure");
}
