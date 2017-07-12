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
 *  @param[in] e - const reference to host.
 */
template<class Archive>
void save(Archive& a, const Host& e)
{
    a(e.sdbusplus::xyz::openbmc_project::State::server::Host::requestedHostTransition());
}

/** @brief Function required by Cereal to perform deserialization.
 *  @tparam Archive - Cereal archive type (binary in our case).
 *  @param[in] a - reference to Cereal archive.
 *  @param[in] e - reference to host.
 */
template<class Archive>
void load(Archive& a, Host& e)
{
    using namespace
        sdbusplus::xyz::openbmc_project::State::server;

    Host::Transition requestedHostTransition{};

    a(requestedHostTransition);

    e.requestedHostTransition(requestedHostTransition);
}

fs::path serialize(const Host& e, const fs::path& dir)
{
    std::ofstream os(dir.c_str(), std::ios::binary);
    cereal::BinaryOutputArchive oarchive(os);
    oarchive(e);
    return dir;
}

bool deserialize(const fs::path& path, Host& e)
{
    if (fs::exists(path))
    {
        std::ifstream is(path.c_str(), std::ios::in | std::ios::binary);
        cereal::BinaryInputArchive iarchive(is);
        iarchive(e);
        return true;
    }
    return false;
}
}
} // namespace state
} // namespace phosphor
