#include "update_decision.hpp"

namespace bandwagon::settings_app {

UpdatePaneDecision DecideUpdatePane(std::uint32_t query_state,
                                    bool metadata_available,
                                    std::uint32_t current_version,
                                    std::uint32_t available_version,
                                    std::uint32_t source_state) {
  if (query_state != 0) {
    return query_state == 1 ? UpdatePaneDecision::kFailed
                            : UpdatePaneDecision::kChecking;
  }
  if (!metadata_available || available_version <= current_version) {
    return UpdatePaneDecision::kUpToDate;
  }
  return source_state == 3 ? UpdatePaneDecision::kDownloadAvailable
                           : UpdatePaneDecision::kInstallAvailable;
}

}  // namespace bandwagon::settings_app
