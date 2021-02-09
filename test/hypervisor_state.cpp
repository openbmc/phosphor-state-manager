#include "config.h"

#include <hypervisor_state_manager.hpp>
#include <sdbusplus/bus.hpp>
#include <sdeventplus/event.hpp>

#include <gtest/gtest.h>

namespace server = sdbusplus::xyz::openbmc_project::State::server;

TEST(checkBootProgress, BasicPaths)
{
    auto bus = sdbusplus::bus::new_default();
    auto event = sdeventplus::Event::get_default();
    bus.attach_event(event.get(), SD_EVENT_PRIORITY_NORMAL);
    auto objPathInst = std::string{HYPERVISOR_OBJPATH} + '0';

    phosphor::state::manager::Hypervisor hypObj(bus, objPathInst.c_str());

    std::string bootProgress = "Invalid.Boot.Progress";
    hypObj.checkBootProgress(bootProgress);
    EXPECT_EQ(hypObj.currentHostState(), server::Host::HostState::Off);

    bootProgress = "xyz.openbmc_project.State.Boot.Progress."
                   "ProgressStages.SystemInitComplete";
    hypObj.checkBootProgress(bootProgress);
    EXPECT_EQ(hypObj.currentHostState(), server::Host::HostState::Standby);

    bootProgress = "xyz.openbmc_project.State.Boot.Progress."
                   "ProgressStages.OSStart";
    hypObj.checkBootProgress(bootProgress);
    EXPECT_EQ(hypObj.currentHostState(),
              server::Host::HostState::TransitioningToRunning);

    bootProgress = "xyz.openbmc_project.State.Boot.Progress."
                   "ProgressStages.OSRunning";
    hypObj.checkBootProgress(bootProgress);
    EXPECT_EQ(hypObj.currentHostState(), server::Host::HostState::Running);
}
