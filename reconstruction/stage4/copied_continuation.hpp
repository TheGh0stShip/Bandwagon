#pragma once

#include <cstdint>

namespace bandwagon::stage4 {

constexpr std::uint64_t kCopiedKeyAddress = 0x6000320b8ULL;
constexpr std::uint64_t kCopiedZeroAddress = 0x6000321c8ULL;
constexpr std::uint64_t kCopiedContinuationExit = 0x400029b3cULL;

struct CopiedContinuationPlan {
  std::uint64_t first_call_target = 0xb3d8;
  std::uint64_t first_r3 = 0;
  std::uint64_t first_r4 = 0;
  std::uint64_t first_r5 = 0;
  std::uint64_t first_r6 = kCopiedKeyAddress;

  bool make_second_call = false;
  std::uint64_t second_call_target = 0xb388;
  std::uint64_t second_r3 = 0;
  std::uint64_t second_r4 = 0;

  std::uint64_t exit_branch_target = kCopiedContinuationExit;
};

// Models the copied block installed at 0x600032048. Stack-relative arguments
// are returned as absolute values based on incoming r1. No calls or branches
// are performed.
CopiedContinuationPlan PlanCopiedContinuation(
    std::uint64_t r1, std::uint64_t r2, std::uint64_t r28,
    std::uint64_t r29, std::uint64_t incoming_r30,
    std::uint64_t first_call_result);

}  // namespace bandwagon::stage4
