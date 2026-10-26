#pragma once

#include "../updater/archive_wrapper.hpp"

#include <cstddef>
#include <cstdint>
#include <string>

namespace bandwagon::settings_app {

struct OfflineUpdateInfo {
  bandwagon::updater::WrappedUpdateArchive archive;
  std::string display_version;
};

// FindOfflineUpdate scans exactly these two candidate indices.
bool IsOfflineUpdateDriveIndex(int index);
std::string BuildOfflineUpdatePath(int index);

// Reconstructs the wrapper validation and version display performed before
// SettingsApp asks miniz for ChangeLog.txt.
OfflineUpdateInfo ParseOfflineUpdate(
    const std::uint8_t* data, std::size_t size,
    bandwagon::updater::Sha1 sha1);

}  // namespace bandwagon::settings_app
