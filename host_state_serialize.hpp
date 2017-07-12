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

/** @brief Serialize and persist host state d-bus object
 *  @param[in] a - const reference to host state.
 *  @param[in] dir - pathname of directory where the serialized error will
 *                   be placed.
 *  @return fs::path - pathname of persisted error file
 */
fs::path serialize(const Host& e,
                   const fs::path& dir = fs::path(HOST_STATE_PERSIST_PATH));

/** @brief Deserialze a persisted host state into a d-bus object
 *  @param[in] path - pathname of persisted host state file
 *  @param[in] e - reference to error object which is the target of
 *             deserialization.
 *  @return bool - true if the deserialization was successful, false otherwise.
 */
bool deserialize(const fs::path& path, Host& e);

}
} // namespace logging
} // namespace phosphor
