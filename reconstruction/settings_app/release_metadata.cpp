#include "release_metadata.hpp"

#include "../update_check/release_version.hpp"

namespace bandwagon::settings_app {

bool BuildReleaseMetadata(std::string_view tag_name, bool prerelease,
                          std::string_view body,
                          const std::vector<ReleaseAsset>& assets,
                          ReleaseMetadata* output) {
  if (!output) return false;
  ReleaseMetadata result;
  if (!bandwagon::update_check::ParseReleaseTag(tag_name,
                                                 &result.packed_version)) {
    return false;
  }
  result.prerelease = prerelease;
  result.body.assign(body);

  for (const auto& asset : assets) {
    if (asset.name == "UpdateData.zip") {
      result.update_asset_id = asset.id;
      result.update_digest = asset.digest;
      result.update_size = asset.size;
    } else if (asset.name == "ChangeLog.txt") {
      result.change_log_asset_id = asset.id;
    }
  }
  if (result.update_asset_id == 0 || result.update_size == 0) return false;
  *output = std::move(result);
  return true;
}

}  // namespace bandwagon::settings_app
