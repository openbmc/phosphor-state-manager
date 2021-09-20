#include "settings.hpp"

#include "xyz/openbmc_project/Common/error.hpp"

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/exception.hpp>

namespace settings
{

PHOSPHOR_LOG2_USING;

using namespace phosphor::logging;
using namespace sdbusplus::xyz::openbmc_project::Common::Error;

constexpr auto mapperService = "xyz.openbmc_project.ObjectMapper";
constexpr auto mapperPath = "/xyz/openbmc_project/object_mapper";
constexpr auto mapperIntf = "xyz.openbmc_project.ObjectMapper";

Objects::Objects(sdbusplus::bus::bus& bus) : bus(bus)
{
    std::vector<std::string> settingsIntfs = {autoRebootIntf, powerRestoreIntf};
    auto depth = 0;

    auto mapperCall = bus.new_method_call(mapperService, mapperPath, mapperIntf,
                                          "GetSubTree");
    mapperCall.append(root);
    mapperCall.append(depth);
    mapperCall.append(settingsIntfs);

    using Interfaces = std::vector<Interface>;
    using MapperResponse = std::map<Path, std::map<Service, Interfaces>>;
    MapperResponse result;

    try
    {
        auto response = bus.call(mapperCall);

        response.read(result);
        if (result.empty())
        {
            error("Invalid response from mapper");
            elog<InternalFailure>();
        }
    }
    catch (const sdbusplus::exception::exception& e)
    {
        error("Error in mapper GetSubTree: {ERROR}", "ERROR", e);
        elog<InternalFailure>();
    }

    for (const auto& iter : result)
    {
        const Path& path = iter.first;

        for (const auto& serviceIter : iter.second)
        {
            for (const auto& interface : serviceIter.second)
            {
                if (autoRebootIntf == interface)
                {
                    /* There are two implementations of the AutoReboot
                     * Interface. A persistent user setting and a one-time
                     * setting which is only valid for one boot of the system.
                     * The one-time setting will have "one_time" in its
                     * object path.
                     */
                    if (path.find("one_time") != std::string::npos)
                    {
                        autoRebootOneTime = path;
                    }
                    else
                    {
                        autoReboot = path;
                    }
                }
                else if (powerRestoreIntf == interface)
                {
                    /* There are two implementations of the PowerRestorePolicy
                     * Interface. A persistent user setting and a one-time
                     * setting which is only valid for one boot of the system.
                     * The one-time setting will have "one_time" in its
                     * object path.
                     */
                    if (path.find("one_time") != std::string::npos)
                    {
                        powerRestorePolicyOneTime = path;
                    }
                    else
                    {
                        powerRestorePolicy = path;
                    }
                }
            }
        }
    }
}

Service Objects::service(const Path& path, const Interface& interface) const
{
    using Interfaces = std::vector<Interface>;
    auto mapperCall =
        bus.new_method_call(mapperService, mapperPath, mapperIntf, "GetObject");
    mapperCall.append(path);
    mapperCall.append(Interfaces({interface}));

    std::map<Service, Interfaces> result;

    try
    {
        auto response = bus.call(mapperCall);
        response.read(result);
    }
    catch (const sdbusplus::exception::exception& e)
    {
        error("Error in mapper GetObject: {ERROR}", "ERROR", e);
        elog<InternalFailure>();
    }

    if (result.empty())
    {
        error("Invalid response from mapper");
        elog<InternalFailure>();
    }

    return result.begin()->first;
}

} // namespace settings
