#pragma once

#include <cstdint>
#include <string_view>

namespace bandwagon::stage3 {

constexpr std::uint32_t kStage4SuccessMarker = 0x41414141u;
constexpr std::uint32_t kStage4ReadFatalCode = 0x00012414u;
constexpr std::uint32_t kStage4ExecuteFatalCode = 0x00012415u;

struct Stage3EntryPlan {
  bool run_badupdate_exploit = false;
  bool load_stage4 = false;
  bool execute_stage4 = false;
  bool free_stage4 = false;
  bool send_smc_message = false;
  std::uint8_t smc_command = 0;
  bool launch_default = false;
  std::uint32_t fatal_code = 0;
};

// Models GameSaveExploitMain under the platform assumption that the fatal
// display routine does not return, as expected by the original control flow.
Stage3EntryPlan BuildStage3EntryPlan(bool resident_dispatcher_available,
                                     bool stage4_read_succeeded,
                                     std::uint32_t stage4_result);

constexpr std::string_view kStage4Path =
    "PAYLOAD:\\BadUpdateExploit-4thStage.bin";
constexpr std::string_view kDefaultPayloadPath = "PAYLOAD:\\default.xex";

}  // namespace bandwagon::stage3
