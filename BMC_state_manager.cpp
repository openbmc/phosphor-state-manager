#include <iostream>
#include <log.hpp>
#include "BMC_state_manager.hpp"

namespace phosphor
{
namespace state
{
namespace manager
{

using namespace sdbusplus::xyz::openbmc_project::State::server;

using namespace phosphor::logging;

constexpr auto SYSTEMD_SERVICE   = "org.freedesktop.systemd1";
constexpr auto SYSTEMD_OBJ_PATH  = "/org/freedesktop/systemd1";
constexpr auto SYSTEMD_INTERFACE = "org.freedesktop.systemd1.Manager";

void BMC::subscribeToSystemdSignals()
{
    auto method = this->bus.new_method_call(SYSTEMD_SERVICE,
                                            SYSTEMD_OBJ_PATH,
                                            SYSTEMD_INTERFACE,
                                            "Subscribe");
    this->bus.call(method);

    return;
}

BMC::Transition BMC::requestedBMCTransition(Transition value)
{
    log<level::INFO>(
            "Setting the RequestedBMCTransition field",
            entry("REQUESTED_BMC_TRANSITION=0x%s",
                  convertForMessage(value).c_str()));
    return BMC::requestedBMCTransition(value);
}

BMC::BMCState BMC::currentBMCState(BMCState value)
{
    log<level::INFO>(
            "Setting the BMCState field",
            entry("CURRENT_BMC_STATE=0x%s",
                  convertForMessage(value).c_str()));
    return BMC::currentBMCState(value);
}


} // namespace manager
} // namespace state
} // namepsace phosphor

