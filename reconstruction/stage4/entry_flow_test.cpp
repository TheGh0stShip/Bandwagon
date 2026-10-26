#include "entry_flow.hpp"

#include <cassert>
#include <iostream>

int main() {
  using bandwagon::stage4::BuildStage4EntryPlan;
  using bandwagon::stage4::Stage4ModeKind;

  const auto mode0 = BuildStage4EntryPlan(0, 0, 0);
  assert(mode0.kind == Stage4ModeKind::kRestoreAndPatch);
  assert(mode0.return_value == 0x41414141u);
  assert(mode0.send_smc_message);
  assert(mode0.smc_prefix[0] == 0x99 && mode0.smc_prefix[1] == 0xff &&
         mode0.smc_prefix[2] == 0xff);
  assert(mode0.relocate_clean_hv);
  assert(mode0.clean_hv_source_offset == 0x780);
  assert(mode0.clean_hv_destination == 0x8000010600030000ULL);
  assert(mode0.relocation_count == 0x200);
  assert(mode0.apply_patch_stream && mode0.patch_stream_offset == 0x380);

  const auto mode1 = BuildStage4EntryPlan(1, 0xfeed, 0x80050000u);
  assert(mode1.kind == Stage4ModeKind::kApplyAddressValues);
  assert(mode1.clear_initial_word_bit);
  assert(mode1.initial_word_address == 0x80040000u);
  assert(mode1.initial_word_clear_mask == 0x20);
  assert(mode1.apply_address_value_table);
  const auto mode1_underflow = BuildStage4EntryPlan(1, 0, 0);
  assert(mode1_underflow.initial_word_address == 0xffffffffffff0000ULL);

  const auto mode2 = BuildStage4EntryPlan(2, 4, 0);
  assert(mode2.kind == Stage4ModeKind::kClearMappingRange);
  assert(mode2.mapping_range.iterations == 4);
  const auto mode2_zero = BuildStage4EntryPlan(2, 0, 0);
  assert(mode2_zero.mapping_range.iterations == 1);

  const auto unsupported = BuildStage4EntryPlan(3, 0, 0);
  assert(unsupported.kind == Stage4ModeKind::kNoOperation);
  assert(unsupported.return_value == 0x41414141u);
  const auto low_word_mode = BuildStage4EntryPlan(0x100000001ULL, 0, 0x20000);
  assert(low_word_mode.kind == Stage4ModeKind::kApplyAddressValues);

  std::cout << "Stage4 entry-flow reconstruction tests passed\n";
}
