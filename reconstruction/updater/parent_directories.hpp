#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace bandwagon::updater {

// Returns the directory prefixes visited by EnsureParentDirectories. The
// final path component is never included; the Xbox-specific caller creates
// each returned prefix and accepts ERROR_ALREADY_EXISTS (183).
std::vector<std::string> EnumerateParentDirectories(std::string_view path);

}  // namespace bandwagon::updater
