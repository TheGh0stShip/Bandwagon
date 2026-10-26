#include "offline_update.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {

bandwagon::updater::Sha1Digest FakeSha1(const std::uint8_t* data,
                                        std::size_t size) {
  bandwagon::updater::Sha1Digest digest{};
  for (std::size_t i = 0; i < digest.size(); ++i) {
    digest[i] = static_cast<std::uint8_t>(data[i % size] ^ (i * 17u));
  }
  return digest;
}

}  // namespace

int main() {
  using bandwagon::settings_app::BuildOfflineUpdatePath;
  using bandwagon::settings_app::IsOfflineUpdateDriveIndex;

  assert(IsOfflineUpdateDriveIndex(0));
  assert(IsOfflineUpdateDriveIndex(1));
  assert(!IsOfflineUpdateDriveIndex(-1));
  assert(!IsOfflineUpdateDriveIndex(2));
  assert(BuildOfflineUpdatePath(0) ==
         "Mass0:\\$SoftmodUpdate\\UpdateData.zip");
  assert(BuildOfflineUpdatePath(1) ==
         "Mass1:\\$SoftmodUpdate\\UpdateData.zip");
  bool rejected = false;
  try {
    (void)BuildOfflineUpdatePath(2);
  } catch (const std::out_of_range&) {
    rejected = true;
  }
  assert(rejected);

  const std::vector<std::uint8_t> zip = {'P', 'K', 3, 4, 1, 2, 3, 4};
  const auto wrapped = bandwagon::updater::BuildUpdateArchiveWrapper(
      {1, 0, 477}, zip.data(), zip.size(), FakeSha1);
  const auto info = bandwagon::settings_app::ParseOfflineUpdate(
      wrapped.data(), wrapped.size(), FakeSha1);
  assert(info.archive.version.major == 1);
  assert(info.archive.version.minor == 0);
  assert(info.archive.version.revision == 477);
  assert(info.display_version == "v1.0.477");
  assert(info.archive.zip_data == zip);

  auto corrupted = wrapped;
  corrupted.back() ^= 1;
  rejected = false;
  try {
    (void)bandwagon::settings_app::ParseOfflineUpdate(
        corrupted.data(), corrupted.size(), FakeSha1);
  } catch (const std::runtime_error&) {
    rejected = true;
  }
  assert(rejected);

  std::cout << "SettingsApp offline-update reconstruction tests passed\n";
}
