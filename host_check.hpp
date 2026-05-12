#pragma once

#include <cstddef>

namespace phosphor::state::manager
{

/** @brief Determine if host is running
 *
 * @return True if host running, False otherwise
 */
bool isHostRunning(size_t hostId = 0);

} // namespace phosphor::state::manager
