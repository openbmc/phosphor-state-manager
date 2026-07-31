#include "settings.hpp"

#include "xyz/openbmc_project/Common/error.hpp"

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/exception.hpp>
#include <xyz/openbmc_project/ObjectMapper/client.hpp>

namespace settings
{

PHOSPHOR_LOG2_USING;

using namespace phosphor::logging;
using namespace sdbusplus::xyz::openbmc_project::Common::Error;

using ObjectMapper = sdbusplus::client::xyz::openbmc_project::ObjectMapper<>;

MapperPaths pathsFromMapperResponse(const MapperResponse& result)
{
    MapperPaths paths;
    for (const auto& pathEntry : result)
    {
        const auto& path = pathEntry.first;
        for (const auto& serviceEntry : pathEntry.second)
        {
            for (const auto& interface : serviceEntry.second)
            {
                if (autoRebootIntf == interface)
                {
                    if (path.find("one_time") != std::string::npos)
                    {
                        paths.autoRebootOneTime = path;
                    }
                    else
                    {
                        paths.autoReboot = path;
                    }
                }
                else if (powerRestoreIntf == interface)
                {
                    if (path.find("one_time") != std::string::npos)
                    {
                        paths.powerRestorePolicyOneTime = path;
                    }
                    else
                    {
                        paths.powerRestorePolicy = path;
                    }
                }
            }
        }
    }

    return paths;
}

Service serviceFromMapperResponse(const MapperObjectResponse& result)
{
    if (result.empty())
    {
        error("Invalid response from mapper");
        elog<InternalFailure>();
    }

    return result.begin()->first;
}

Objects::Objects(sdbusplus::bus_t& bus, const Path& root) : bus(bus)
{
    std::vector<std::string> settingsIntfs = {autoRebootIntf, powerRestoreIntf};
    auto depth = 0;

    auto mapperCall = bus.new_method_call(
        ObjectMapper::default_service, ObjectMapper::instance_path,
        ObjectMapper::interface, ObjectMapper::method_names::get_sub_tree);
    mapperCall.append(root);
    mapperCall.append(depth);
    mapperCall.append(settingsIntfs);

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
    catch (const sdbusplus::exception_t& e)
    {
        error("Error in mapper GetSubTree: {ERROR}", "ERROR", e);
        elog<InternalFailure>();
    }

    auto paths = pathsFromMapperResponse(result);
    autoReboot = paths.autoReboot;
    autoRebootOneTime = paths.autoRebootOneTime;
    powerRestorePolicy = paths.powerRestorePolicy;
    powerRestorePolicyOneTime = paths.powerRestorePolicyOneTime;
}

Service Objects::service(const Path& path, const Interface& interface) const
{
    auto mapperCall = bus.new_method_call(
        ObjectMapper::default_service, ObjectMapper::instance_path,
        ObjectMapper::interface, ObjectMapper::method_names::get_object);
    mapperCall.append(path);
    mapperCall.append(Interfaces({interface}));

    MapperObjectResponse result;

    try
    {
        auto response = bus.call(mapperCall);
        response.read(result);
    }
    catch (const sdbusplus::exception_t& e)
    {
        error("Error in mapper GetObject: {ERROR}", "ERROR", e);
        elog<InternalFailure>();
    }

    return serviceFromMapperResponse(result);
}

HostObjects::HostObjects(sdbusplus::bus_t& bus, size_t id) :
    Objects(bus, Path("/xyz/openbmc_project/control/host") + std::to_string(id))
{}

} // namespace settings
