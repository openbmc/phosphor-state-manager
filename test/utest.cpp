#include "host_state_manager.hpp"
#include <gtest/gtest.h>

using namespace phosphor::state::manager;
using namespace sdbusplus::xyz::openbmc_project::State;

/* Begin verifyValidTransition() test cases */
TEST(VerifyValidTransition, TransitionActive) {
    server::Host::Transition tranReq;
    bool verify = Host::verifyValidTransition(server::Host::Transition::Off,
                                              Host::HostState::Running,
                                              true,
                                              tranReq);
    EXPECT_EQ(verify, false);
}

TEST(VerifyValidTransition, AlreadyAtStateOff) {
    server::Host::Transition tranReq;
    bool verify = Host::verifyValidTransition(server::Host::Transition::Off,
                                              Host::HostState::Off,
                                              false,
                                              tranReq);
    EXPECT_EQ(verify, false);
}

TEST(VerifyValidTransition, AlreadyAtStateRunning) {
    server::Host::Transition tranReq;
    bool verify = Host::verifyValidTransition(server::Host::Transition::On,
                                              Host::HostState::Running,
                                              false,
                                              tranReq);
    EXPECT_EQ(verify, false);
}

TEST(VerifyValidTransition, OffToReboot) {
    server::Host::Transition tranReq;
    bool verify = Host::verifyValidTransition(server::Host::Transition::Reboot,
                                              Host::HostState::Off,
                                              false,
                                              tranReq);
    EXPECT_EQ(verify, false);
}

TEST(VerifyValidTransition, RunningToOff) {
    server::Host::Transition tranReq;
    bool verify = Host::verifyValidTransition(server::Host::Transition::Off,
                                              Host::HostState::Running,
                                              false,
                                              tranReq);
    EXPECT_EQ(verify, true);
    EXPECT_EQ(tranReq,server::Host::Transition::Off);
}

TEST(VerifyValidTransition, OffToRunning) {
    server::Host::Transition tranReq;
    bool verify = Host::verifyValidTransition(server::Host::Transition::On,
                                              Host::HostState::Off,
                                              false,
                                              tranReq);
    EXPECT_EQ(verify, true);
    EXPECT_EQ(tranReq,server::Host::Transition::On);
}

TEST(VerifyValidTransition, RunningToRebootTranOn) {
    server::Host::Transition tranReq;
    bool verify = Host::verifyValidTransition(server::Host::Transition::Reboot,
                                              Host::HostState::Running,
                                              true,
                                              tranReq);
    EXPECT_EQ(verify, false);
}

TEST(VerifyValidTransition, RunningToRebootTranOff) {
    server::Host::Transition tranReq;
    bool verify = Host::verifyValidTransition(server::Host::Transition::Reboot,
                                              Host::HostState::Running,
                                              false,
                                              tranReq);
    EXPECT_EQ(verify, true);
    EXPECT_EQ(tranReq,server::Host::Transition::Off);
}

TEST(VerifyValidTransition, OffToRebootTranOn) {
    server::Host::Transition tranReq;
    bool verify = Host::verifyValidTransition(server::Host::Transition::Reboot,
                                              Host::HostState::Off,
                                              true,
                                              tranReq);
    EXPECT_EQ(verify, true);
    EXPECT_EQ(tranReq,server::Host::Transition::On);
}
/* End verifyValidTransition() test cases */
