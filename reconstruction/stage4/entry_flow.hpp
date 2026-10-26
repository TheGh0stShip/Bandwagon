#pragma once

#include <array>
#include <cstdint>

#include "maintenance_helpers.hpp"

namespace bandwagon::stage4 {

constexpr std::uint32_t kStage4ReturnMarker = 0x41414141u;

enum class Stage4ModeKind {
  kRestoreAndPatch,
  kApplyAddressValues,
  kClearMappingRange,
  kNoOperation,
};

struct Stage4EntryPlan {
  Stage4ModeKind kind = Stage4ModeKind::kNoOperation;
  std::uint32_t return_value = kStage4ReturnMarker;

  bool send_smc_message = false;
  std::array<std::uint8_t, 3> smc_prefix{};
  bool relocate_clean_hv = false;
  std::uint32_t clean_hv_source_offset = 0;
  std::uint64_t clean_hv_destination = 0;
  std::uint32_t relocation_count = 0;
  bool apply_patch_stream = false;
  std::uint32_t patch_stream_offset = 0;

  bool clear_initial_word_bit = false;
  std::uint64_t initial_word_address = 0;
  std::uint32_t initial_word_clear_mask = 0;
  bool apply_address_value_table = false;

  MappingRangePlan mapping_range;
};

// Reconstructs the decisions and call arguments of the Stage4 entry at file
// offset zero. table_first_address is the first big-endian word reached through
// r6 in mode 1; mode 2 interprets the low word of r6 as a signed loop bound.
Stage4EntryPlan BuildStage4EntryPlan(std::uint64_t mode_register,
                                     std::uint64_t r6,
                                     std::uint32_t table_first_address);

}  // namespace bandwagon::stage4
