#include <iostream>
#include <systemd/sd-bus.h>
#include <map>
#include <string>
#include "host_state_manager.hpp"

namespace phosphor
{
namespace statemanager
{

// When you see server:: you know we're referencing our base class
using namespace sdbusplus::xyz::openbmc_project::State;

/* Map a transition to it's systemd target */
const std::map<server::Host::Transition,std::string> SYSTEMD_TABLE = {
        {server::Host::Transition::Off,"obmc-chassis-stop@0.target"},
        {server::Host::Transition::On,"obmc-chassis-start@0.target"}
};

Host::Host(
        sdbusplus::bus::bus& bus,
        const char* busName,
        const char* objPath) :
        sdbusplus::server::object::object<
            server::Host>(
               bus, objPath),
         _bus(bus),
         _path(objPath),
         _tranActive(false)
{
    determineInitialState();
}

// TODO - Will be rewritten once sdbusplus client bindings are in place
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
    _tranActive = true;

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

    // TODO - This should happen once we get event that target state reached
    _tranActive = false;
    return rc;
}

Host::Transition Host::requestedHostTransition(Transition value)
{
    std::cout << "Someone is setting the RequestedHostTransition field" <<
        std::endl;

    if(Host::verifyValidTransition(value,
                                   server::Host::currentHostState(),
                                   _tranActive))
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
    std::cout << "Someone is being bad and trying to set the HostState field" <<
            std::endl;

    return server::Host::currentHostState();
}

} // namespace statemanager
} // namepsace phosphor
