#pragma once

#include <cstdint>

namespace bandwagon::settings_app {

enum class UpdatePaneDecision {
  kUpToDate,
  kDownloadAvailable,
  kInstallAvailable,
  kChecking,
  kFailed,
};

// Reconstructs the non-ImGui branch tree in DrawAutoUpdateSettings.
UpdatePaneDecision DecideUpdatePane(std::uint32_t query_state,
                                    bool metadata_available,
                                    std::uint32_t current_version,
                                    std::uint32_t available_version,
                                    std::uint32_t source_state);

}  // namespace bandwagon::settings_app
