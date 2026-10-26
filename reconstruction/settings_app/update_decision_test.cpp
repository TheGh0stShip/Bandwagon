#include "update_decision.hpp"

#include <cassert>
#include <iostream>

int main() {
  using bandwagon::settings_app::DecideUpdatePane;
  using bandwagon::settings_app::UpdatePaneDecision;

  constexpr std::uint32_t current = 0x010001ddu;  // 1.0.477
  assert(DecideUpdatePane(0, true, current, 0x010001deu, 3) ==
         UpdatePaneDecision::kDownloadAvailable);
  assert(DecideUpdatePane(0, true, current, 0x01010000u, 2) ==
         UpdatePaneDecision::kInstallAvailable);
  assert(DecideUpdatePane(0, true, current, current, 3) ==
         UpdatePaneDecision::kUpToDate);
  assert(DecideUpdatePane(0, true, current, 0x0009ffffu, 3) ==
         UpdatePaneDecision::kUpToDate);
  assert(DecideUpdatePane(0, false, current, 0xffffffffu, 3) ==
         UpdatePaneDecision::kUpToDate);
  assert(DecideUpdatePane(1, true, current, 0xffffffffu, 3) ==
         UpdatePaneDecision::kFailed);
  assert(DecideUpdatePane(2, true, current, 0xffffffffu, 3) ==
         UpdatePaneDecision::kChecking);
  assert(DecideUpdatePane(0xffffffffu, true, current, 0xffffffffu, 3) ==
         UpdatePaneDecision::kChecking);

  std::cout << "SettingsApp update-decision reconstruction tests passed\n";
}
