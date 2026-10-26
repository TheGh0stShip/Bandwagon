#include "entry_flow.hpp"

namespace bandwagon::stage4 {

Stage4EntryPlan BuildStage4EntryPlan(std::uint64_t mode_register,
                                     std::uint64_t r6,
                                     std::uint32_t table_first_address) {
  Stage4EntryPlan plan;
  const std::uint32_t mode = static_cast<std::uint32_t>(mode_register);
  if (mode == 0) {
    plan.kind = Stage4ModeKind::kRestoreAndPatch;
    plan.send_smc_message = true;
    plan.smc_prefix = {0x99, 0xff, 0xff};
    plan.relocate_clean_hv = true;
    plan.clean_hv_source_offset = 0x780;
    plan.clean_hv_destination = 0x8000010600030000ULL;
    plan.relocation_count = 0x200;
    plan.apply_patch_stream = true;
    plan.patch_stream_offset = 0x380;
  } else if (mode == 1) {
    plan.kind = Stage4ModeKind::kApplyAddressValues;
    plan.clear_initial_word_bit = true;
    plan.initial_word_address =
        static_cast<std::uint64_t>(table_first_address) - 0x10000ULL;
    plan.initial_word_clear_mask = 0x20;
    plan.apply_address_value_table = true;
  } else if (mode == 2) {
    plan.kind = Stage4ModeKind::kClearMappingRange;
    plan.mapping_range = PlanMappingRange(r6);
  }
  return plan;
}

}  // namespace bandwagon::stage4
