#include <algorithm>
#include <sdbusplus/server.hpp>
#include <sdbusplus/exception.hpp>
#include <xyz/openbmc_project/State/Chassis/server.hpp>

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

Chassis::Chassis(bus::bus& bus, const char* path)
        : _xyz_openbmc_project_State_Chassis_interface(
                bus, path, _interface, _vtable, this)
{
}



auto Chassis::requestedPowerTransition() const ->
        Transition
{
    return _requestedPowerTransition;
}

int Chassis::_callback_get_RequestedPowerTransition(
        sd_bus* bus, const char* path, const char* interface,
        const char* property, sd_bus_message* reply, void* context,
        sd_bus_error* error)
{
    using sdbusplus::server::binding::details::convertForMessage;

    try
    {
        auto m = message::message(sd_bus_message_ref(reply));

        auto o = static_cast<Chassis*>(context);
        m.append(convertForMessage(o->requestedPowerTransition()));
    }
    catch(sdbusplus::internal_exception_t& e)
    {
        sd_bus_error_set_const(error, e.name(), e.description());
        return -EINVAL;
    }

    return true;
}

auto Chassis::requestedPowerTransition(Transition value) ->
        Transition
{
    if (_requestedPowerTransition != value)
    {
        _requestedPowerTransition = value;
        _xyz_openbmc_project_State_Chassis_interface.property_changed("RequestedPowerTransition");
    }

    return _requestedPowerTransition;
}

int Chassis::_callback_set_RequestedPowerTransition(
        sd_bus* bus, const char* path, const char* interface,
        const char* property, sd_bus_message* value, void* context,
        sd_bus_error* error)
{
    try
    {
        auto m = message::message(sd_bus_message_ref(value));

        auto o = static_cast<Chassis*>(context);

        std::string v{};
        m.read(v);
        o->requestedPowerTransition(convertTransitionFromString(v));
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
namespace Chassis
{
static const auto _property_RequestedPowerTransition =
    utility::tuple_to_array(message::types::type_id<
            std::string>());
}
}
auto Chassis::currentPowerState() const ->
        PowerState
{
    return _currentPowerState;
}

int Chassis::_callback_get_CurrentPowerState(
        sd_bus* bus, const char* path, const char* interface,
        const char* property, sd_bus_message* reply, void* context,
        sd_bus_error* error)
{
    using sdbusplus::server::binding::details::convertForMessage;

    try
    {
        auto m = message::message(sd_bus_message_ref(reply));

        auto o = static_cast<Chassis*>(context);
        m.append(convertForMessage(o->currentPowerState()));
    }
    catch(sdbusplus::internal_exception_t& e)
    {
        sd_bus_error_set_const(error, e.name(), e.description());
        return -EINVAL;
    }

    return true;
}

auto Chassis::currentPowerState(PowerState value) ->
        PowerState
{
    if (_currentPowerState != value)
    {
        _currentPowerState = value;
        _xyz_openbmc_project_State_Chassis_interface.property_changed("CurrentPowerState");
    }

    return _currentPowerState;
}

int Chassis::_callback_set_CurrentPowerState(
        sd_bus* bus, const char* path, const char* interface,
        const char* property, sd_bus_message* value, void* context,
        sd_bus_error* error)
{
    try
    {
        auto m = message::message(sd_bus_message_ref(value));

        auto o = static_cast<Chassis*>(context);

        std::string v{};
        m.read(v);
        o->currentPowerState(convertPowerStateFromString(v));
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
namespace Chassis
{
static const auto _property_CurrentPowerState =
    utility::tuple_to_array(message::types::type_id<
            std::string>());
}
}


namespace
{
/** String to enum mapping for Chassis::Transition */
static const std::tuple<const char*, Chassis::Transition> mappingChassisTransition[] =
        {
            std::make_tuple( "xyz.openbmc_project.State.Chassis.Transition.Off",                 Chassis::Transition::Off ),
            std::make_tuple( "xyz.openbmc_project.State.Chassis.Transition.On",                 Chassis::Transition::On ),
        };

} // anonymous namespace

auto Chassis::convertTransitionFromString(std::string& s) ->
        Transition
{
    auto i = std::find_if(
            std::begin(mappingChassisTransition),
            std::end(mappingChassisTransition),
            [&s](auto& e){ return 0 == strcmp(s.c_str(), std::get<0>(e)); } );
    if (std::end(mappingChassisTransition) == i)
    {
        throw sdbusplus::exception::InvalidEnumString();
    }
    else
    {
        return std::get<1>(*i);
    }
}

std::string convertForMessage(Chassis::Transition v)
{
    auto i = std::find_if(
            std::begin(mappingChassisTransition),
            std::end(mappingChassisTransition),
            [v](auto& e){ return v == std::get<1>(e); });
    return std::get<0>(*i);
}

namespace
{
/** String to enum mapping for Chassis::PowerState */
static const std::tuple<const char*, Chassis::PowerState> mappingChassisPowerState[] =
        {
            std::make_tuple( "xyz.openbmc_project.State.Chassis.PowerState.Off",                 Chassis::PowerState::Off ),
            std::make_tuple( "xyz.openbmc_project.State.Chassis.PowerState.On",                 Chassis::PowerState::On ),
        };

} // anonymous namespace

auto Chassis::convertPowerStateFromString(std::string& s) ->
        PowerState
{
    auto i = std::find_if(
            std::begin(mappingChassisPowerState),
            std::end(mappingChassisPowerState),
            [&s](auto& e){ return 0 == strcmp(s.c_str(), std::get<0>(e)); } );
    if (std::end(mappingChassisPowerState) == i)
    {
        throw sdbusplus::exception::InvalidEnumString();
    }
    else
    {
        return std::get<1>(*i);
    }
}

std::string convertForMessage(Chassis::PowerState v)
{
    auto i = std::find_if(
            std::begin(mappingChassisPowerState),
            std::end(mappingChassisPowerState),
            [v](auto& e){ return v == std::get<1>(e); });
    return std::get<0>(*i);
}

const vtable::vtable_t Chassis::_vtable[] = {
    vtable::start(),
    vtable::property("RequestedPowerTransition",
                     details::Chassis::_property_RequestedPowerTransition
                        .data(),
                     _callback_get_RequestedPowerTransition,
                     _callback_set_RequestedPowerTransition,
                     vtable::property_::emits_change),
    vtable::property("CurrentPowerState",
                     details::Chassis::_property_CurrentPowerState
                        .data(),
                     _callback_get_CurrentPowerState,
                     _callback_set_CurrentPowerState,
                     vtable::property_::emits_change),
    vtable::end()
};

} // namespace server
} // namespace State
} // namespace openbmc_project
} // namespace xyz
} // namespace sdbusplus

