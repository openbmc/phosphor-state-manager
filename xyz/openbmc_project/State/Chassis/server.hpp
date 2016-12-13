#pragma once
#include <tuple>
#include <systemd/sd-bus.h>
#include <sdbusplus/server.hpp>

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

class Chassis
{
    public:
        /* Define all of the basic class operations:
         *     Not allowed:
         *         - Default constructor to avoid nullptrs.
         *         - Copy operations due to internal unique_ptr.
         *     Allowed:
         *         - Move operations.
         *         - Destructor.
         */
        Chassis() = delete;
        Chassis(const Chassis&) = delete;
        Chassis& operator=(const Chassis&) = delete;
        Chassis(Chassis&&) = default;
        Chassis& operator=(Chassis&&) = default;
        virtual ~Chassis() = default;

        /** @brief Constructor to put object onto bus at a dbus path.
         *  @param[in] bus - Bus to attach to.
         *  @param[in] path - Path to attach at.
         */
        Chassis(bus::bus& bus, const char* path);

        enum class Transition
        {
            Off,
            On,
        };
        enum class PowerState
        {
            Off,
            On,
        };



        /** Get value of RequestedPowerTransition */
        virtual Transition requestedPowerTransition() const;
        /** Set value of RequestedPowerTransition */
        virtual Transition requestedPowerTransition(Transition value);
        /** Get value of CurrentPowerState */
        virtual PowerState currentPowerState() const;
        /** Set value of CurrentPowerState */
        virtual PowerState currentPowerState(PowerState value);

    /** @brief Convert a string to an appropriate enum value.
     *  @param[in] s - The string to convert in the form of
     *                 "xyz.openbmc_project.State.Chassis.<value name>"
     *  @return - The enum value.
     */
    static Transition convertTransitionFromString(std::string& s);
    /** @brief Convert a string to an appropriate enum value.
     *  @param[in] s - The string to convert in the form of
     *                 "xyz.openbmc_project.State.Chassis.<value name>"
     *  @return - The enum value.
     */
    static PowerState convertPowerStateFromString(std::string& s);

    private:

        /** @brief sd-bus callback for get-property 'RequestedPowerTransition' */
        static int _callback_get_RequestedPowerTransition(
            sd_bus*, const char*, const char*, const char*,
            sd_bus_message*, void*, sd_bus_error*);
        /** @brief sd-bus callback for set-property 'RequestedPowerTransition' */
        static int _callback_set_RequestedPowerTransition(
            sd_bus*, const char*, const char*, const char*,
            sd_bus_message*, void*, sd_bus_error*);

        /** @brief sd-bus callback for get-property 'CurrentPowerState' */
        static int _callback_get_CurrentPowerState(
            sd_bus*, const char*, const char*, const char*,
            sd_bus_message*, void*, sd_bus_error*);
        /** @brief sd-bus callback for set-property 'CurrentPowerState' */
        static int _callback_set_CurrentPowerState(
            sd_bus*, const char*, const char*, const char*,
            sd_bus_message*, void*, sd_bus_error*);


        static constexpr auto _interface = "xyz.openbmc_project.State.Chassis";
        static const vtable::vtable_t _vtable[];
        sdbusplus::server::interface::interface
                _xyz_openbmc_project_State_Chassis_interface;

        Transition _requestedPowerTransition = Transition::Off;
        PowerState _currentPowerState{};

};

/* Specialization of sdbusplus::server::bindings::details::convertForMessage
 * for enum-type Chassis::Transition.
 *
 * This converts from the enum to a constant c-string representing the enum.
 *
 * @param[in] e - Enum value to convert.
 * @return C-string representing the name for the enum value.
 */
std::string convertForMessage(Chassis::Transition e);
/* Specialization of sdbusplus::server::bindings::details::convertForMessage
 * for enum-type Chassis::PowerState.
 *
 * This converts from the enum to a constant c-string representing the enum.
 *
 * @param[in] e - Enum value to convert.
 * @return C-string representing the name for the enum value.
 */
std::string convertForMessage(Chassis::PowerState e);

} // namespace server
} // namespace State
} // namespace openbmc_project
} // namespace xyz
} // namespace sdbusplus

