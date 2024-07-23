#include "bmc_state_manager.hpp"

#include "utils.hpp"
#include "xyz/openbmc_project/Common/error.hpp"

#include <fcntl.h>
#include <gpiod.h>
#include <sys/mman.h>
#include <unistd.h>

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/exception.hpp>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

namespace phosphor
{
namespace state
{
namespace manager
{

PHOSPHOR_LOG2_USING;

// When you see server:: you know we're referencing our base class
namespace server = sdbusplus::server::xyz::openbmc_project::state;

using namespace phosphor::logging;
using sdbusplus::xyz::openbmc_project::Common::Error::InternalFailure;

constexpr auto obmcQuiesceTarget = "obmc-bmc-service-quiesce@0.target";
constexpr auto obmcStandbyTarget = "multi-user.target";
constexpr auto signalDone = "done";
constexpr auto activeState = "active";

/* Map a transition to it's systemd target */
const std::map<server::BMC::Transition, const char*> SYSTEMD_TABLE = {
    {server::BMC::Transition::Reboot, "reboot.target"}};

constexpr auto SYSTEMD_SERVICE = "org.freedesktop.systemd1";
constexpr auto SYSTEMD_OBJ_PATH = "/org/freedesktop/systemd1";
constexpr auto SYSTEMD_INTERFACE = "org.freedesktop.systemd1.Manager";
constexpr auto SYSTEMD_PRP_INTERFACE = "org.freedesktop.DBus.Properties";

/* Identify BMC reboot cause */
#define AST_G6_SRST 0x1e6e2000
#define AST_G6_SRST_OFFSET 0x74
#define BMC_PWR_ON_RESET(base, offset) *((uint32_t*)(base + (offset)))
#define PAGE_SIZE 0x1000

std::string BMC::getUnitState(const std::string& unitToCheck)
{
    std::variant<std::string> currentState;
    sdbusplus::message::object_path unitTargetPath;

    auto method = this->bus.new_method_call(SYSTEMD_SERVICE, SYSTEMD_OBJ_PATH,
                                            SYSTEMD_INTERFACE, "GetUnit");

    method.append(unitToCheck);

    try
    {
        auto result = this->bus.call(method);
        result.read(unitTargetPath);
    }
    catch (const sdbusplus::exception_t& e)
    {
        // Not all input units will have been loaded yet so just return an
        // empty string if an exception is caught in this path
        info("Unit {UNIT} not found: {ERROR}", "UNIT", unitToCheck, "ERROR", e);
        return std::string{};
    }

    method = this->bus.new_method_call(
        SYSTEMD_SERVICE,
        static_cast<const std::string&>(unitTargetPath).c_str(),
        SYSTEMD_PRP_INTERFACE, "Get");

    method.append("org.freedesktop.systemd1.Unit", "ActiveState");

    try
    {
        auto result = this->bus.call(method);

        // Is input target active or inactive?
        result.read(currentState);
    }
    catch (const sdbusplus::exception_t& e)
    {
        info("Error in ActiveState Get: {ERROR}", "ERROR", e);
        return std::string{};
    }
    return (std::get<std::string>(currentState));
}

void BMC::discoverInitialState()
{
    // First look to see if the BMC quiesce target is active
    auto currentStateStr = getUnitState(obmcQuiesceTarget);
    if (currentStateStr == activeState)
    {
        info("Setting the BMCState field to BMC_QUIESCED");
        this->currentBMCState(BMCState::Quiesced);
        return;
    }

    // If not quiesced, then check standby target
    currentStateStr = getUnitState(obmcStandbyTarget);
    if (currentStateStr == activeState)
    {
        info("Setting the BMCState field to BMC_READY");
        this->currentBMCState(BMCState::Ready);
    }
    else
    {
        info("Setting the BMCState field to BMC_NOTREADY");
        this->currentBMCState(BMCState::NotReady);
    }

    return;
}

void BMC::executeTransition(const Transition tranReq)
{
    // HardReboot does not shutdown any services and immediately transitions
    // into the reboot process
    if (server::BMC::Transition::HardReboot == tranReq)
    {
        // Put BMC state not NotReady when issuing a BMC reboot
        // and stop monitoring for state changes
        this->currentBMCState(BMCState::NotReady);
        this->stateSignal.reset();

        auto method = this->bus.new_method_call(
            SYSTEMD_SERVICE, SYSTEMD_OBJ_PATH, SYSTEMD_INTERFACE, "Reboot");
        try
        {
            this->bus.call(method);
        }
        catch (const sdbusplus::exception_t& e)
        {
            info("Error in HardReboot: {ERROR}", "ERROR", e);
        }
    }
    else
    {
        // Check to make sure it can be found
        auto iter = SYSTEMD_TABLE.find(tranReq);
        if (iter == SYSTEMD_TABLE.end())
            return;

        const auto& sysdUnit = iter->second;

        auto method = this->bus.new_method_call(
            SYSTEMD_SERVICE, SYSTEMD_OBJ_PATH, SYSTEMD_INTERFACE, "StartUnit");
        // The only valid transition is reboot and that
        // needs to be irreversible once started

        method.append(sysdUnit, "replace-irreversibly");

        // Put BMC state not NotReady when issuing a BMC reboot
        // and stop monitoring for state changes
        this->currentBMCState(BMCState::NotReady);
        this->stateSignal.reset();

        try
        {
            this->bus.call(method);
        }
        catch (const sdbusplus::exception_t& e)
        {
            info("Error in StartUnit - replace-irreversibly: {ERROR}", "ERROR",
                 e);
        }
    }
    return;
}

int BMC::bmcStateChange(sdbusplus::message_t& msg)
{
    uint32_t newStateID{};
    sdbusplus::message::object_path newStateObjPath;
    std::string newStateUnit{};
    std::string newStateResult{};

    // Read the msg and populate each variable
    msg.read(newStateID, newStateObjPath, newStateUnit, newStateResult);

    if ((newStateUnit == obmcQuiesceTarget) && (newStateResult == signalDone))
    {
        error("BMC has entered BMC_QUIESCED state");
        this->currentBMCState(BMCState::Quiesced);

        // There is no getting out of Quiesced once entered (other then BMC
        // reboot) so stop watching for signals
        auto method =
            this->bus.new_method_call(SYSTEMD_SERVICE, SYSTEMD_OBJ_PATH,
                                      SYSTEMD_INTERFACE, "Unsubscribe");

        try
        {
            this->bus.call(method);
        }
        catch (const sdbusplus::exception_t& e)
        {
            info("Error in Unsubscribe: {ERROR}", "ERROR", e);
        }

        // disable the system state change object as well
        this->stateSignal.reset();

        return 0;
    }

    // Caught the signal that indicates the BMC is now BMC_READY
    if ((newStateUnit == obmcStandbyTarget) && (newStateResult == signalDone))
    {
        info("BMC_READY");
        this->currentBMCState(BMCState::Ready);
    }

    return 0;
}

BMC::Transition BMC::requestedBMCTransition(Transition value)
{
    info("Setting the RequestedBMCTransition field to "
         "{REQUESTED_BMC_TRANSITION}",
         "REQUESTED_BMC_TRANSITION", value);

    executeTransition(value);
    return server::BMC::requestedBMCTransition(value);
}

BMC::BMCState BMC::currentBMCState(BMCState value)
{
    info("Setting the BMCState field to {CURRENT_BMC_STATE}",
         "CURRENT_BMC_STATE", value);

    return server::BMC::currentBMCState(value);
}

uint32_t BMC::getRegRebootCause()
{
    int mem_fd;
    uint8_t* addr_srst;
    uint32_t power_on_reset_flag = 0;
    uint32_t bmc_reset_event_log_base = 0x0;
    uint32_t bmc_reset_event_log_offset = 0x0;

    bmc_reset_event_log_base = AST_G6_SRST;
    bmc_reset_event_log_offset = AST_G6_SRST_OFFSET;

    mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (mem_fd < 0)
    {
        syslog(LOG_CRIT, "devmem open failed");
        lg2::error("getRegRebootCause - devmem open failed");
        return 0;
    }

    addr_srst = static_cast<uint8_t*>(mmap(NULL, PAGE_SIZE,
                                           PROT_READ | PROT_WRITE, MAP_SHARED,
                                           mem_fd, bmc_reset_event_log_base));
    if (addr_srst == NULL)
    {
        syslog(LOG_CRIT, "Mapping addr_srst failed");
        lg2::error("getRegRebootCause - Mapping addr_srst failed");
        close(mem_fd);
        return 0;
    }

    power_on_reset_flag = BMC_PWR_ON_RESET(addr_srst,
                                           bmc_reset_event_log_offset);

    munmap(addr_srst, PAGE_SIZE);
    close(mem_fd);

    return power_on_reset_flag;
}

void BMC::addSEL()
{
    uint32_t reboot_cause_raw = 0;

    reboot_cause_raw = getRegRebootCause();

    // todo: Analyze raw data and send different sel based on reboot reasons

    // Generate log
    std::string errorMsg = "";
    std::stringstream hex_raw_data;

    hex_raw_data << std::hex << std::setw(8) << std::setfill('0')
                 << reboot_cause_raw;
    std::string hex_str = hex_raw_data.str();

    // Add to journal and SEL
    lg2::error("Last BMC reboot raw data: 0x{RAW_DATA}", "RAW_DATA", hex_str);

    errorMsg = "Last BMC reboot raw data: 0x" + hex_str;

    phosphor::state::manager::utils::createError(
        this->bus, errorMsg,
        sdbusplus::server::xyz::openbmc_project::logging::Entry::Level::Notice);
}

BMC::RebootCause BMC::lastRebootCause(RebootCause value)
{
    info("Setting the RebootCause field to {LAST_REBOOT_CAUSE}",
         "LAST_REBOOT_CAUSE", value);

    /* In order to avoid duplicate records,
    SEL is only added for situations other than unknown causes*/
    if (value != RebootCause::Unknown)
    {
        this->addSEL();
    }

    return server::BMC::lastRebootCause(value);
}

void BMC::updateLastRebootTime()
{
    using namespace std::chrono;
    struct sysinfo info;

    auto rc = sysinfo(&info);
    assert(rc == 0);
    // Since uptime is in seconds, also get the current time in seconds.
    auto now = time_point_cast<seconds>(system_clock::now());
    auto rebootTimeTs = now - seconds(info.uptime);
    rebootTime =
        duration_cast<milliseconds>(rebootTimeTs.time_since_epoch()).count();
    server::BMC::lastRebootTime(rebootTime);
}

uint64_t BMC::lastRebootTime() const
{
    return rebootTime;
}

void BMC::discoverLastRebootCause()
{
    uint64_t bootReason = 0;
    std::ifstream file;
    auto bootstatusPath = "/sys/class/watchdog/watchdog0/bootstatus";

    file.exceptions(std::ifstream::failbit | std::ifstream::badbit |
                    std::ifstream::eofbit);

    try
    {
        file.open(bootstatusPath);
        file >> bootReason;
    }
    catch (const std::exception& e)
    {
        auto rc = errno;
        error("Failed to read sysfs file {FILE} with errno {ERRNO}", "FILE",
              bootstatusPath, "ERRNO", rc);
    }

    switch (bootReason)
    {
        case WDIOF_EXTERN1:
            this->lastRebootCause(RebootCause::Watchdog);
            return;
        case WDIOF_CARDRESET:
            this->lastRebootCause(RebootCause::POR);
            return;
        default:
            this->lastRebootCause(RebootCause::Unknown);
            // Continue below to see if more details can be found
            // on reason for reboot
            break;
    }

    // If the above code could not detect a reason, look for a the
    // reset-cause-pinhole gpio to see if it is the reason for the reboot
    auto gpioval =
        phosphor::state::manager::utils::getGpioValue("reset-cause-pinhole");

    // A 0 indicates a pinhole reset occurred
    if (0 == gpioval)
    {
        info("The BMC reset was caused by a pinhole reset");
        this->lastRebootCause(RebootCause::PinholeReset);

        // Generate log telling user a pinhole reset has occurred
        const std::string errorMsg = "xyz.openbmc_project.State.PinholeReset";
        phosphor::state::manager::utils::createError(
            this->bus, errorMsg,
            sdbusplus::server::xyz::openbmc_project::logging::Entry::Level::
                Notice);
        return;
    }

    // If we still haven't found a reason, see if we lost AC power
    // Note that a pinhole reset will remove AC power to the chassis
    // on some systems so we always want to look for the pinhole reset
    // first as that would be the main reason AC power was lost.
    size_t chassisId = 0;
    if (phosphor::state::manager::utils::checkACLoss(chassisId))
    {
        this->lastRebootCause(RebootCause::POR);

        return;
    }

    // If none of above, Add SEL of Unknown
    this->addSEL();

    return;
}

} // namespace manager
} // namespace state
} // namespace phosphor
