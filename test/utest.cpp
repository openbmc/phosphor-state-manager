#include "host_state_manager.hpp"
#include <gtest/gtest.h>

using namespace phosphor::statemanager;
using namespace sdbusplus::xyz::openbmc_project::State;

/* Begin verifyValidTransition() test cases */
TEST(VerifyValidTransition, TransitionActive) {
    bool verify = Host::verifyValidTransition(server::Host::Transition::Off,
                                              Host::HostState::Running,
                                              true);
    EXPECT_EQ(verify, false);
}

TEST(VerifyValidTransition, AlreadyAtStateOff) {
    bool verify = Host::verifyValidTransition(server::Host::Transition::Off,
                                              Host::HostState::Off,
                                              false);
    EXPECT_EQ(verify, false);
}

TEST(VerifyValidTransition, AlreadyAtStateRunning) {
    bool verify = Host::verifyValidTransition(server::Host::Transition::On,
                                              Host::HostState::Running,
                                              false);
    EXPECT_EQ(verify, false);
}

TEST(VerifyValidTransition, OffToReboot) {
    bool verify = Host::verifyValidTransition(server::Host::Transition::Reboot,
                                              Host::HostState::Off,
                                              false);
    EXPECT_EQ(verify, false);
}

TEST(VerifyValidTransition, RunningToOff) {
    bool verify = Host::verifyValidTransition(server::Host::Transition::Off,
                                              Host::HostState::Running,
                                              false);
    EXPECT_EQ(verify, true);
}

TEST(VerifyValidTransition, OffToRunning) {
    bool verify = Host::verifyValidTransition(server::Host::Transition::On,
                                              Host::HostState::Off,
                                              false);
    EXPECT_EQ(verify, true);
}
/* End verifyValidTransition() test cases */
