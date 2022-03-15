#pragma once

#include <cstddef>

namespace phosphor
{
namespace state
{
namespace manager
{

/** @brief Determine if host is running
 *
 * @return True if host running, False otherwise
 */
bool isHostRunning(size_t hostId = 0);

} // namespace manager
} // namespace state
} // namespace phosphor
