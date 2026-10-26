#include "entry_flow.hpp"

#include <cassert>
#include <iostream>

int main() {
  using bandwagon::stage3::BuildStage3EntryPlan;

  const auto resident = BuildStage3EntryPlan(true, false, 0);
  assert(!resident.run_badupdate_exploit && !resident.load_stage4);
  assert(resident.send_smc_message && resident.smc_command == 0x99);
  assert(resident.launch_default && resident.fatal_code == 0);

  const auto read_failure = BuildStage3EntryPlan(false, false, 0);
  assert(read_failure.run_badupdate_exploit && read_failure.load_stage4);
  assert(!read_failure.execute_stage4 && !read_failure.free_stage4);
  assert(read_failure.fatal_code == 0x12414u);
  assert(!read_failure.send_smc_message && !read_failure.launch_default);

  const auto execute_failure = BuildStage3EntryPlan(false, true, 0);
  assert(execute_failure.execute_stage4 && !execute_failure.free_stage4);
  assert(execute_failure.fatal_code == 0x12415u);
  assert(!execute_failure.send_smc_message);

  const auto installed =
      BuildStage3EntryPlan(false, true, 0x41414141u);
  assert(installed.run_badupdate_exploit && installed.load_stage4);
  assert(installed.execute_stage4 && installed.free_stage4);
  assert(installed.fatal_code == 0);
  assert(installed.send_smc_message && installed.smc_command == 0x99);
  assert(installed.launch_default);
  assert(bandwagon::stage3::kStage4Path ==
         "PAYLOAD:\\BadUpdateExploit-4thStage.bin");
  assert(bandwagon::stage3::kDefaultPayloadPath ==
         "PAYLOAD:\\default.xex");

  std::cout << "Stage3 entry-flow reconstruction tests passed\n";
}
