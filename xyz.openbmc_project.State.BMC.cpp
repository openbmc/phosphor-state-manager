#include <algorithm>
#include <sdbusplus/server.hpp>
#include <sdbusplus/exception.hpp>
#include <xyz/openbmc_project/State/BMC/server.hpp>

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

BMC::BMC(bus::bus& bus, const char* path)
        : _xyz_openbmc_project_State_BMC_interface(
                bus, path, _interface, _vtable, this)
{
}



auto BMC::requestedBMCTransition() const ->
        Transition
{
    return _requestedBMCTransition;
}

int BMC::_callback_get_RequestedBMCTransition(
        sd_bus* bus, const char* path, const char* interface,
        const char* property, sd_bus_message* reply, void* context,
        sd_bus_error* error)
{
    using sdbusplus::server::binding::details::convertForMessage;

    try
    {
        auto m = message::message(sd_bus_message_ref(reply));

        auto o = static_cast<BMC*>(context);
        m.append(convertForMessage(o->requestedBMCTransition()));
    }
    catch(sdbusplus::internal_exception_t& e)
    {
        sd_bus_error_set_const(error, e.name(), e.description());
        return -EINVAL;
    }

    return true;
}

auto BMC::requestedBMCTransition(Transition value) ->
        Transition
{
    if (_requestedBMCTransition != value)
    {
        _requestedBMCTransition = value;
        _xyz_openbmc_project_State_BMC_interface.property_changed("RequestedBMCTransition");
    }

    return _requestedBMCTransition;
}

int BMC::_callback_set_RequestedBMCTransition(
        sd_bus* bus, const char* path, const char* interface,
        const char* property, sd_bus_message* value, void* context,
        sd_bus_error* error)
{
    try
    {
        auto m = message::message(sd_bus_message_ref(value));

        auto o = static_cast<BMC*>(context);

        std::string v{};
        m.read(v);
        o->requestedBMCTransition(convertTransitionFromString(v));
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
namespace BMC
{
static const auto _property_RequestedBMCTransition =
    utility::tuple_to_array(message::types::type_id<
            std::string>());
}
}
auto BMC::currentBMCState() const ->
        BMCState
{
    return _currentBMCState;
}

int BMC::_callback_get_CurrentBMCState(
        sd_bus* bus, const char* path, const char* interface,
        const char* property, sd_bus_message* reply, void* context,
        sd_bus_error* error)
{
    using sdbusplus::server::binding::details::convertForMessage;

    try
    {
        auto m = message::message(sd_bus_message_ref(reply));

        auto o = static_cast<BMC*>(context);
        m.append(convertForMessage(o->currentBMCState()));
    }
    catch(sdbusplus::internal_exception_t& e)
    {
        sd_bus_error_set_const(error, e.name(), e.description());
        return -EINVAL;
    }

    return true;
}

auto BMC::currentBMCState(BMCState value) ->
        BMCState
{
    if (_currentBMCState != value)
    {
        _currentBMCState = value;
        _xyz_openbmc_project_State_BMC_interface.property_changed("CurrentBMCState");
    }

    return _currentBMCState;
}

int BMC::_callback_set_CurrentBMCState(
        sd_bus* bus, const char* path, const char* interface,
        const char* property, sd_bus_message* value, void* context,
        sd_bus_error* error)
{
    try
    {
        auto m = message::message(sd_bus_message_ref(value));

        auto o = static_cast<BMC*>(context);

        std::string v{};
        m.read(v);
        o->currentBMCState(convertBMCStateFromString(v));
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
namespace BMC
{
static const auto _property_CurrentBMCState =
    utility::tuple_to_array(message::types::type_id<
            std::string>());
}
}


namespace
{
/** String to enum mapping for BMC::Transition */
static const std::tuple<const char*, BMC::Transition> mappingBMCTransition[] =
        {
            std::make_tuple( "xyz.openbmc_project.State.BMC.Transition.Reboot",                 BMC::Transition::Reboot ),
            std::make_tuple( "xyz.openbmc_project.State.BMC.Transition.None",                 BMC::Transition::None ),
        };

} // anonymous namespace

auto BMC::convertTransitionFromString(std::string& s) ->
        Transition
{
    auto i = std::find_if(
            std::begin(mappingBMCTransition),
            std::end(mappingBMCTransition),
            [&s](auto& e){ return 0 == strcmp(s.c_str(), std::get<0>(e)); } );
    if (std::end(mappingBMCTransition) == i)
    {
        throw sdbusplus::exception::InvalidEnumString();
    }
    else
    {
        return std::get<1>(*i);
    }
}

std::string convertForMessage(BMC::Transition v)
{
    auto i = std::find_if(
            std::begin(mappingBMCTransition),
            std::end(mappingBMCTransition),
            [v](auto& e){ return v == std::get<1>(e); });
    return std::get<0>(*i);
}

namespace
{
/** String to enum mapping for BMC::BMCState */
static const std::tuple<const char*, BMC::BMCState> mappingBMCBMCState[] =
        {
            std::make_tuple( "xyz.openbmc_project.State.BMC.BMCState.Ready",                 BMC::BMCState::Ready ),
            std::make_tuple( "xyz.openbmc_project.State.BMC.BMCState.NotReady",                 BMC::BMCState::NotReady ),
        };

} // anonymous namespace

auto BMC::convertBMCStateFromString(std::string& s) ->
        BMCState
{
    auto i = std::find_if(
            std::begin(mappingBMCBMCState),
            std::end(mappingBMCBMCState),
            [&s](auto& e){ return 0 == strcmp(s.c_str(), std::get<0>(e)); } );
    if (std::end(mappingBMCBMCState) == i)
    {
        throw sdbusplus::exception::InvalidEnumString();
    }
    else
    {
        return std::get<1>(*i);
    }
}

std::string convertForMessage(BMC::BMCState v)
{
    auto i = std::find_if(
            std::begin(mappingBMCBMCState),
            std::end(mappingBMCBMCState),
            [v](auto& e){ return v == std::get<1>(e); });
    return std::get<0>(*i);
}

const vtable::vtable_t BMC::_vtable[] = {
    vtable::start(),
    vtable::property("RequestedBMCTransition",
                     details::BMC::_property_RequestedBMCTransition
                        .data(),
                     _callback_get_RequestedBMCTransition,
                     _callback_set_RequestedBMCTransition,
                     vtable::property_::emits_change),
    vtable::property("CurrentBMCState",
                     details::BMC::_property_CurrentBMCState
                        .data(),
                     _callback_get_CurrentBMCState,
                     _callback_set_CurrentBMCState,
                     vtable::property_::emits_change),
    vtable::end()
};

} // namespace server
} // namespace State
} // namespace openbmc_project
} // namespace xyz
} // namespace sdbusplus

