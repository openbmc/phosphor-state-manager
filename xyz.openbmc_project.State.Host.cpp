#include <algorithm>
#include <sdbusplus/server.hpp>
#include <sdbusplus/exception.hpp>
#include <xyz/openbmc_project/State/Host/server.hpp>

namespace sdbusplus
{
namespace xyz
{
namespace openbmc_project
{
namespace State
{
namespace server
{

Host::Host(bus::bus& bus, const char* path)
        : _xyz_openbmc_project_State_Host_interface(
                bus, path, _interface, _vtable, this)
{
}



auto Host::hostTransition() const ->
        Transition
{
    return _hostTransition;
}

int Host::_callback_get_HostTransition(
        sd_bus* bus, const char* path, const char* interface,
        const char* property, sd_bus_message* reply, void* context,
        sd_bus_error* error)
{
    using sdbusplus::server::binding::details::convertForMessage;

    try
    {
        auto m = message::message(sd_bus_message_ref(reply));

        auto o = static_cast<Host*>(context);
        m.append(convertForMessage(o->hostTransition()));
    }
    catch(sdbusplus::internal_exception_t& e)
    {
        sd_bus_error_set_const(error, e.name(), e.description());
        return -EINVAL;
    }

    return true;
}

auto Host::hostTransition(Transition value) ->
        Transition
{
    if (_hostTransition != value)
    {
        _hostTransition = value;
        _xyz_openbmc_project_State_Host_interface.property_changed("HostTransition");
    }

    return _hostTransition;
}

int Host::_callback_set_HostTransition(
        sd_bus* bus, const char* path, const char* interface,
        const char* property, sd_bus_message* value, void* context,
        sd_bus_error* error)
{
    try
    {
        auto m = message::message(sd_bus_message_ref(value));

        auto o = static_cast<Host*>(context);

        std::string v{};
        m.read(v);
        o->hostTransition(convertTransitionFromString(v));
    }
    catch(sdbusplus::internal_exception_t& e)
    {
        sd_bus_error_set_const(error, e.name(), e.description());
        return -EINVAL;
    }

    return true;
}

namespace details
{
namespace Host
{
static const auto _property_HostTransition =
    utility::tuple_to_array(message::types::type_id<
            std::string>());
}
}
auto Host::currentHostState() const ->
        HostState
{
    return _currentHostState;
}

int Host::_callback_get_CurrentHostState(
        sd_bus* bus, const char* path, const char* interface,
        const char* property, sd_bus_message* reply, void* context,
        sd_bus_error* error)
{
    using sdbusplus::server::binding::details::convertForMessage;

    try
    {
        auto m = message::message(sd_bus_message_ref(reply));

        auto o = static_cast<Host*>(context);
        m.append(convertForMessage(o->currentHostState()));
    }
    catch(sdbusplus::internal_exception_t& e)
    {
        sd_bus_error_set_const(error, e.name(), e.description());
        return -EINVAL;
    }

    return true;
}

auto Host::currentHostState(HostState value) ->
        HostState
{
    if (_currentHostState != value)
    {
        _currentHostState = value;
        _xyz_openbmc_project_State_Host_interface.property_changed("CurrentHostState");
    }

    return _currentHostState;
}

int Host::_callback_set_CurrentHostState(
        sd_bus* bus, const char* path, const char* interface,
        const char* property, sd_bus_message* value, void* context,
        sd_bus_error* error)
{
    try
    {
        auto m = message::message(sd_bus_message_ref(value));

        auto o = static_cast<Host*>(context);

        std::string v{};
        m.read(v);
        o->currentHostState(convertHostStateFromString(v));
    }
    catch(sdbusplus::internal_exception_t& e)
    {
        sd_bus_error_set_const(error, e.name(), e.description());
        return -EINVAL;
    }

    return true;
}

namespace details
{
namespace Host
{
static const auto _property_CurrentHostState =
    utility::tuple_to_array(message::types::type_id<
            std::string>());
}
}


namespace
{
/** String to enum mapping for Host::Transition */
static const std::tuple<const char*, Host::Transition> mappingHostTransition[] =
        {
            std::make_tuple( "xyz.openbmc_project.State.Host.Transition.Off",                 Host::Transition::Off ),
            std::make_tuple( "xyz.openbmc_project.State.Host.Transition.On",                 Host::Transition::On ),
            std::make_tuple( "xyz.openbmc_project.State.Host.Transition.Reboot",                 Host::Transition::Reboot ),
        };

} // anonymous namespace

auto Host::convertTransitionFromString(std::string& s) ->
        Transition
{
    auto i = std::find_if(
            std::begin(mappingHostTransition),
            std::end(mappingHostTransition),
            [&s](auto& e){ return 0 == strcmp(s.c_str(), std::get<0>(e)); } );
    if (std::end(mappingHostTransition) == i)
    {
        throw sdbusplus::exception::InvalidEnumString();
    }
    else
    {
        return std::get<1>(*i);
    }
}

std::string convertForMessage(Host::Transition v)
{
    auto i = std::find_if(
            std::begin(mappingHostTransition),
            std::end(mappingHostTransition),
            [v](auto& e){ return v == std::get<1>(e); });
    return std::get<0>(*i);
}

namespace
{
/** String to enum mapping for Host::HostState */
static const std::tuple<const char*, Host::HostState> mappingHostHostState[] =
        {
            std::make_tuple( "xyz.openbmc_project.State.Host.HostState.Off",                 Host::HostState::Off ),
            std::make_tuple( "xyz.openbmc_project.State.Host.HostState.Booting",                 Host::HostState::Booting ),
            std::make_tuple( "xyz.openbmc_project.State.Host.HostState.Running",                 Host::HostState::Running ),
        };

} // anonymous namespace

auto Host::convertHostStateFromString(std::string& s) ->
        HostState
{
    auto i = std::find_if(
            std::begin(mappingHostHostState),
            std::end(mappingHostHostState),
            [&s](auto& e){ return 0 == strcmp(s.c_str(), std::get<0>(e)); } );
    if (std::end(mappingHostHostState) == i)
    {
        throw sdbusplus::exception::InvalidEnumString();
    }
    else
    {
        return std::get<1>(*i);
    }
}

std::string convertForMessage(Host::HostState v)
{
    auto i = std::find_if(
            std::begin(mappingHostHostState),
            std::end(mappingHostHostState),
            [v](auto& e){ return v == std::get<1>(e); });
    return std::get<0>(*i);
}

const vtable::vtable_t Host::_vtable[] = {
    vtable::start(),
    vtable::property("HostTransition",
                     details::Host::_property_HostTransition
                        .data(),
                     _callback_get_HostTransition,
                     _callback_set_HostTransition,
                     vtable::property_::emits_change),
    vtable::property("CurrentHostState",
                     details::Host::_property_CurrentHostState
                        .data(),
                     _callback_get_CurrentHostState,
                     _callback_set_CurrentHostState,
                     vtable::property_::emits_change),
    vtable::end()
};

} // namespace server
} // namespace State
} // namespace openbmc_project
} // namespace xyz
} // namespace sdbusplus

