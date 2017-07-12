#pragma once

#include <string>
#include <vector>
#include <experimental/filesystem>
#include "host_state_manager.hpp"
#include "config.h"

namespace phosphor
{
namespace state
{
namespace manager
{

namespace fs = std::experimental::filesystem;

/** @brief Serialize and persist requested host state
 *  @param[in] host - const reference to host state.
 *  @param[in] dir - pathname of file where the serialized host state will
 *                   be placed.
 *  @return fs::path - pathname of persisted error file
 */
fs::path serialize(const Host& host,
                   const fs::path& dir = fs::path(HOST_STATE_PERSIST_PATH));

/** @brief Deserialze a persisted requested host state.
 *  @param[in] path - pathname of persisted host state file
 *  @param[in] host - reference to host state object which is the target of
 *             deserialization.
 *  @return bool - true if the deserialization was successful, false otherwise.
 */
bool deserialize(const fs::path& path, Host& host);

} // namespace manager
} // namespace state
} // namespace phosphor
