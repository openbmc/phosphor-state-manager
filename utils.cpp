#include "utils.hpp"

#include <phosphor-logging/lg2.hpp>

namespace phosphor
{
namespace state
{
namespace manager
{
namespace utils
{

PHOSPHOR_LOG2_USING;

constexpr auto MAPPER_BUSNAME = "xyz.openbmc_project.ObjectMapper";
constexpr auto MAPPER_PATH = "/xyz/openbmc_project/object_mapper";
constexpr auto MAPPER_INTERFACE = "xyz.openbmc_project.ObjectMapper";
constexpr auto PROPERTY_INTERFACE = "org.freedesktop.DBus.Properties";

std::string getService(sdbusplus::bus::bus& bus, std::string path,
                       std::string interface)
{
    auto mapper = bus.new_method_call(MAPPER_BUSNAME, MAPPER_PATH,
                                      MAPPER_INTERFACE, "GetObject");

    mapper.append(path, std::vector<std::string>({interface}));

    std::vector<std::pair<std::string, std::vector<std::string>>>
        mapperResponse;

    try
    {
        auto mapperResponseMsg = bus.call(mapper);

        mapperResponseMsg.read(mapperResponse);
        if (mapperResponse.empty())
        {
            error(
                "Error no matching service with path {PATH} and interface {INTERFACE}",
                "PATH", path, "INTERFACE", interface);
            throw std::runtime_error("Error no matching service");
        }
    }
    catch (const sdbusplus::exception::exception& e)
    {
        error("Error in mapper call with path {PATH}, interface "
              "{INTERFACE}, and exception {ERROR}",
              "PATH", path, "INTERFACE", interface, "ERROR", e);
        throw;
    }

    return mapperResponse.begin()->first;
}

void setProperty(sdbusplus::bus::bus& bus, const std::string& path,
                 const std::string& interface, const std::string& property,
                 const std::string& value)
{
    std::variant<std::string> variantValue = value;
    std::string service = getService(bus, path, interface);

    auto method = bus.new_method_call(service.c_str(), path.c_str(),
                                      PROPERTY_INTERFACE, "Set");

    method.append(interface, property, variantValue);
    bus.call_noreply(method);

    return;
}

} // namespace utils
} // namespace manager
} // namespace state
} // namespace phosphor
