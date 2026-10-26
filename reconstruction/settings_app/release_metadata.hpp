#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace bandwagon::settings_app {

struct ReleaseAsset {
  std::string name;
  std::uint64_t id = 0;
  std::string digest;
  std::uint32_t size = 0;
};

struct ReleaseMetadata {
  std::uint32_t packed_version = 0;
  bool prerelease = false;
  std::string body;
  std::uint64_t update_asset_id = 0;
  std::string update_digest;
  std::uint32_t update_size = 0;
  std::uint64_t change_log_asset_id = 0;
};

// Reconstructs the first-party field selection after JsonCpp has decoded the
// GitHub response. Repeated matching assets overwrite earlier values. The
// portable boundary rejects malformed tags; the closed function ignores
// sscanf's conversion count and can pack indeterminate stack values instead.
bool BuildReleaseMetadata(std::string_view tag_name, bool prerelease,
                          std::string_view body,
                          const std::vector<ReleaseAsset>& assets,
                          ReleaseMetadata* output);

}  // namespace bandwagon::settings_app
