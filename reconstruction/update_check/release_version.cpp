#include "release_version.hpp"

#include <cstdio>
#include <string>

namespace bandwagon::update_check {

bool ParseReleaseTag(std::string_view tag, std::uint32_t* packed_version) {
  if (!packed_version) return false;

  // The recovered implementation passes a NUL-terminated JsonCpp string to
  // sscanf with this exact format and accepts exactly three conversions.
  const std::string terminated(tag);
  int major = 0;
  int minor = 0;
  int patch = 0;
  if (std::sscanf(terminated.c_str(), "%*c%d.%d.%d", &major, &minor,
                  &patch) != 3) {
    return false;
  }

  *packed_version =
      (static_cast<std::uint32_t>(major) << 24) |
      ((static_cast<std::uint32_t>(minor) & 0xffu) << 16) |
      (static_cast<std::uint32_t>(patch) & 0xffffu);
  return true;
}

}  // namespace bandwagon::update_check
