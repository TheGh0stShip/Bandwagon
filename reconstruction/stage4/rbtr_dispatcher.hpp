#pragma once

#include <cstdint>

namespace bandwagon::stage4 {

constexpr std::uint32_t kRbtrMagic = 0x72627472u;

enum class DispatchKind {
  kDelegateOriginal,
  kReturn,
  kCopyAndExecute,
  kDelegateOperation5,
};

struct DispatchPlan {
  DispatchKind kind = DispatchKind::kReturn;
  std::uint32_t return_value = 0;
  std::uint64_t branch_target = 0;
  std::uint64_t copy_destination = 0;
  std::uint64_t copy_source = 0;
  std::uint32_t copy_word_count = 0;
  std::uint64_t delegated_r3 = 0;
  std::uint64_t delegated_r4 = 0;
  std::uint64_t delegated_r5 = 0;
};

// Reconstructs the resident dispatcher at HV address 0x600031fb8 without
// copying memory, maintaining caches, or transferring control.
DispatchPlan PlanRbtrDispatch(std::uint32_t magic, std::uint32_t operation,
                              std::uint64_t r5, std::uint64_t r6,
                              std::uint64_t r7);

}  // namespace bandwagon::stage4
