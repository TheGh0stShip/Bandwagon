#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace bandwagon::update_check {

// Reconstructs the tag scanner and bit packing inside ParseReleaseVersion.
// The JSON lookup performed by the original caller is intentionally outside
// this dependency-free boundary.
bool ParseReleaseTag(std::string_view tag, std::uint32_t* packed_version);

// Portable view of the JsonCpp result consumed by ParseReleaseVersion. JSON
// decoding itself remains at the declared JsonCpp 00.11.0 ownership boundary.
struct ReleaseDocument {
  bool parse_succeeded = false;
  std::optional<std::string> tag_name;
};

bool ParseReleaseVersion(const ReleaseDocument& document,
                         std::uint32_t* packed_version);

}  // namespace bandwagon::update_check
