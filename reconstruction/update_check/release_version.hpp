#pragma once

#include <cstdint>
#include <string_view>

namespace bandwagon::update_check {

// Reconstructs the tag scanner and bit packing inside ParseReleaseVersion.
// The JSON lookup performed by the original caller is intentionally outside
// this dependency-free boundary.
bool ParseReleaseTag(std::string_view tag, std::uint32_t* packed_version);

}  // namespace bandwagon::update_check
