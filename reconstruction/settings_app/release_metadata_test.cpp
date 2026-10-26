#include "release_metadata.hpp"

#include <cassert>
#include <iostream>

int main() {
  using bandwagon::settings_app::BuildReleaseMetadata;
  using bandwagon::settings_app::ReleaseAsset;
  using bandwagon::settings_app::ReleaseMetadata;

  ReleaseMetadata result;
  const std::vector<ReleaseAsset> assets = {
      {"UpdateData.zip", 10, "sha256:first", 100},
      {"ignored.bin", 20, "", 200},
      {"ChangeLog.txt", 30, "", 20},
      {"UpdateData.zip", 40, "sha256:last", 400},
      {"ChangeLog.txt", 50, "", 30},
  };
  assert(BuildReleaseMetadata("v1.2.477", true, "changes", assets, &result));
  assert(result.packed_version == 0x010201ddu);
  assert(result.prerelease);
  assert(result.body == "changes");
  assert(result.update_asset_id == 40);
  assert(result.update_digest == "sha256:last");
  assert(result.update_size == 400);
  assert(result.change_log_asset_id == 50);

  const ReleaseMetadata before = result;
  assert(!BuildReleaseMetadata("invalid", false, "", assets, &result));
  assert(result.update_asset_id == before.update_asset_id);
  assert(!BuildReleaseMetadata("v1.2.3", false, "",
                               {{"UpdateData.zip", 0, "", 1}}, &result));
  assert(!BuildReleaseMetadata("v1.2.3", false, "",
                               {{"UpdateData.zip", 1, "", 0}}, &result));
  assert(!BuildReleaseMetadata("v1.2.3", false, "", assets, nullptr));

  std::cout << "SettingsApp release-metadata reconstruction tests passed\n";
}
