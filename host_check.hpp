#pragma once

#include <chrono>
#include <cstddef>

namespace phosphor
{
namespace state
{
namespace manager
{

using namespace std::literals::chrono_literals;

// Constants for host firmware condition checking with mapper
constexpr int MAX_MAPPER_RETRIES = 5;
constexpr auto MAPPER_RETRY_DELAY = 1000ms;

/** @brief Determine if host is running
 *
 * @return True if host running, False otherwise
 */
bool isHostRunning(size_t hostId = 0);

} // namespace manager
} // namespace state
} // namespace phosphor
