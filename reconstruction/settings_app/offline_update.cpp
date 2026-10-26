#include "offline_update.hpp"

#include <stdexcept>

namespace bandwagon::settings_app {

bool IsOfflineUpdateDriveIndex(int index) { return index >= 0 && index < 2; }

std::string BuildOfflineUpdatePath(int index) {
  if (!IsOfflineUpdateDriveIndex(index)) {
    throw std::out_of_range("offline update drive index");
  }
  return "Mass" + std::to_string(index) +
         ":\\$SoftmodUpdate\\UpdateData.zip";
}

OfflineUpdateInfo ParseOfflineUpdate(const std::uint8_t* data,
                                     std::size_t size,
                                     bandwagon::updater::Sha1 sha1) {
  OfflineUpdateInfo result;
  result.archive =
      bandwagon::updater::ParseUpdateArchiveWrapper(data, size, sha1);
  const auto& version = result.archive.version;
  result.display_version = "v" + std::to_string(version.major) + "." +
                           std::to_string(version.minor) + "." +
                           std::to_string(version.revision);
  return result;
}

}  // namespace bandwagon::settings_app
