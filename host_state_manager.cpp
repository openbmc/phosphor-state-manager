#include <iostream>
#include <systemd/sd-bus.h>
#include <map>
#include <string>
#include "host_state_manager.hpp"

namespace phosphor
{
namespace state
{
namespace manager
{

// When you see server:: you know we're referencing our base class
using namespace sdbusplus::xyz::openbmc_project::State;

/* Map a transition to it's systemd target */
const std::map<server::Host::Transition,std::string> SYSTEMD_TABLE = {
        {server::Host::Transition::Off,"obmc-chassis-stop@0.target"},
        {server::Host::Transition::On,"obmc-chassis-start@0.target"}
};

/* Map a system state to the HostState */
const std::map<std::string,server::Host::HostState> SYS_HOST_STATE_TABLE = {
        {"HOST_BOOTING",server::Host::HostState::Running},
        {"HOST_POWERED_OFF",server::Host::HostState::Off}
};

Host::Host(
        sdbusplus::bus::bus& bus,
        const char* busName,
        const char* objPath) :
        sdbusplus::server::object::object<
            server::Host>(
               bus, objPath),
         bus(bus),
         path(objPath),
         stateSignal(bus,
                     "type='signal',member='GotoSystemState'",
                     Host::handleSysStateChange,
                     this),
         tranActive(false)
{
    determineInitialState();
}

/* TODO - Will be rewritten once sdbusplus client bindings are in place
 *        Use object manager and sdbusplus client
 */
void Host::determineInitialState()
{
    int rc = 0;
    int pgood = -1;
    sd_bus_message *response = NULL;
    sd_bus_error busError = SD_BUS_ERROR_NULL;
    sd_bus *bus = NULL;
    rc = sd_bus_open_system(&bus);
    if(rc < 0)
    {
        std::cerr << "Failed to open system bus" << std::endl;
        goto finish;
    }

    rc = sd_bus_get_property(bus,
                             "org.openbmc.control.Power",
                             "/org/openbmc/control/power0",
                             "org.openbmc.control.Power",
                             "pgood",
                             &busError,
                             &response,
                             "i");
    if(rc < 0)
    {
        std::cerr << "Failed to read pgood property" << std::endl;
        goto finish;
    }

    rc = sd_bus_message_read(response, "i", &pgood);
    if(rc < 0)
    {
        std::cerr << "Failed to extract pgood from msg" << std::endl;
        goto finish;
    }

    if(pgood==1)
    {
        std::cout << "HOST is BOOTED " << pgood << std::endl;
        server::Host::currentHostState(HostState::Running);
    }
    else
    {
        std::cout << "HOST is not BOOTED " << pgood << std::endl;
        server::Host::currentHostState(HostState::Off);
    }

    // Set transition initially to Off
    // TODO - Eventually need to restore this from persistent storage
    server::Host::requestedHostTransition(Transition::Off);

finish:
    sd_bus_error_free(&busError);
    response = sd_bus_message_unref(response);
    sd_bus_unref(bus);

    return;
}

bool Host::verifyValidTransition(const Transition &tranReq,
                                        const HostState &curState,
                                        const bool &tranActive)
{
    bool valid = false;

    // Make sure we're not in process of a transition
    if (tranActive)
    {
        std::cerr << "Busy, currently executing transition" << std::endl;
        goto finish;
    }

    /* Valid Transitions
     *
     * CurrentHostState    RequestedHostTransition
     * Off              -> On
     * Running          -> Off
     * Running          -> Reboot
     */

    switch (tranReq)
    {
        case Transition::Off:
        {
            if(curState == HostState::Off)
            {
                std::cout << "Already at requested Off state" << std::endl;
            }
            else if(curState == HostState::Running)
            {
                valid = true;
            }
            break;
        }
        case Transition::On:
        {
            if(curState == HostState::Running)
            {
                std::cout << "Already at requested On state" << std::endl;
            }
            else if(curState == HostState::Off)
            {
                valid = true;
            }
            break;
        }
        case Transition::Reboot:
        {
            if(curState == HostState::Off)
            {
                std::cout << "Can not request reboot in off state" << std::endl;
            }
            else if(curState == HostState::Running)
            {
                valid = true;
            }
            break;
        }
    }

    finish:
    return valid;
}

int Host::executeTransition(const Transition& tranReq)
{
    tranActive = true;

    int rc = 0;
    sd_bus *bus = NULL;
    sd_bus_message *response = NULL;
    sd_bus_error busError = SD_BUS_ERROR_NULL;

    const char* sysdUnit = SYSTEMD_TABLE.find(tranReq)->second.c_str();

    rc = sd_bus_open_system(&bus);
    if(rc < 0)
    {
        std::cerr << "Failed to open system bus" << std::endl;
        goto finish;
    }

    rc = sd_bus_call_method(bus,
                              "org.freedesktop.systemd1",
                              "/org/freedesktop/systemd1",
                              "org.freedesktop.systemd1.Manager",
                              "StartUnit",
                              &busError,
                              &response,
                              "ss",
                              sysdUnit,
                              "replace");
    if(rc < 0)
    {
        std::cerr << "Failed to call systemd" << std::endl;
        goto finish;
    }
    else
    {
        rc = 0;
    }

finish:
    sd_bus_error_free(&busError);
    response = sd_bus_message_unref(response);
    sd_bus_unref(bus);

    return rc;
}

int Host::handleSysStateChange(sd_bus_message *msg, void *usrData,
                               sd_bus_error *retError)
{

    const char *newState = nullptr;
    int rc = sd_bus_message_read(msg, "s", &newState);
    if (rc < 0)
    {
        std::cerr << "Failed to parse signal message: " << strerror(-rc) <<
                std::endl;
        return -1;
    }

    std::cout << "The System State has changed to " << newState << std::endl;

    auto it = SYS_HOST_STATE_TABLE.find(newState);
    if(it != SYS_HOST_STATE_TABLE.end())
    {
        Host::HostState gotoState = it->second;
        Host* hostInst = static_cast<Host*>(usrData);
        hostInst->currentHostState(gotoState);
    }
    else
    {
        std::cout << "Not a relevant state change for host" << std::endl;
    }

    return 0;
}

Host::Transition Host::requestedHostTransition(Transition value)
{
    std::cout << "Someone is setting the RequestedHostTransition field" <<
        std::endl;

    if(Host::verifyValidTransition(value,
                                   server::Host::currentHostState(),
                                   tranActive))
    {
        std::cout << "Valid transition so start it" << std::endl;
        int rc = Host::executeTransition(value);
        if(!rc)
        {
            std::cout << "Transaction executed with success" << std::endl;
            return server::Host::requestedHostTransition(value);
        }
    }
    std::cout << "Not a valid transaction request" << std::endl;
    return server::Host::requestedHostTransition();
}

Host::HostState Host::currentHostState(HostState value)
{
    std::cout << "Changing HostState" << std::endl;

    // Transaction now complete
    tranActive = false;
    return server::Host::currentHostState(value);
}

} // namespace manager
} // namespace state
} // namepsace phosphor
