#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/tuple.hpp>
#include <cereal/archives/binary.hpp>
#include <fstream>
#include "host_state_serialize.hpp"
#include "host_state_manager.hpp"

namespace phosphor
{
namespace state
{
namespace manager
{
/** @brief Function required by Cereal to perform serialization.
 *  @tparam Archive - Cereal archive type (binary in our case).
 *  @param[in] a - reference to Cereal archive.
 *  @param[in] host - const reference to host.
 */
template<class Archive>
void save(Archive& a, const Host& host)
{
    a(host.sdbusplus::xyz::openbmc_project::
           State::server::Host::requestedHostTransition());
}

/** @brief Function required by Cereal to perform deserialization.
 *  @tparam Archive - Cereal archive type (binary in our case).
 *  @param[in] a - reference to Cereal archive.
 *  @param[in] host - reference to host.
 */
template<class Archive>
void load(Archive& a, Host& host)
{
    using namespace
        sdbusplus::xyz::openbmc_project::State::server;

    Host::Transition requestedHostTransition{};

    a(requestedHostTransition);

    host.requestedHostTransition(requestedHostTransition);
}

fs::path serialize(const Host& host, const fs::path& dir)
{
    std::ofstream os(dir.c_str(), std::ios::binary);
    cereal::BinaryOutputArchive oarchive(os);
    oarchive(host);
    return dir;
}

bool deserialize(const fs::path& path, Host& host)
{
    if (fs::exists(path))
    {
        std::ifstream is(path.c_str(), std::ios::in | std::ios::binary);
        cereal::BinaryInputArchive iarchive(is);
        iarchive(host);
        return true;
    }
    return false;
}
} //namespace manager
} // namespace state
} // namespace phosphor
