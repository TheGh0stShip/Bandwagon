#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace bandwagon::updater {

// Returns the directory prefixes visited by EnsureParentDirectories. The
// first backslash is treated as the root separator and skipped; subsequent
// slash/backslash prefixes are returned, excluding the final component. The
// Xbox-specific caller accepts ERROR_ALREADY_EXISTS (183).
std::vector<std::string> EnumerateParentDirectories(std::string_view path);

}  // namespace bandwagon::updater
