#include "copied_continuation.hpp"

namespace bandwagon::stage4 {

CopiedContinuationPlan PlanCopiedContinuation(
    std::uint64_t r1, std::uint64_t r2, std::uint64_t r28,
    std::uint64_t r29, std::uint64_t incoming_r30,
    std::uint64_t first_call_result) {
  CopiedContinuationPlan plan;
  plan.first_r3 = r1 + 0x50;
  plan.first_r4 = r1 + 0x150;
  plan.first_r5 = incoming_r30;

  if (r28 == 0) return plan;
  plan.make_second_call = true;
  plan.second_r3 = r28;
  if (static_cast<std::uint32_t>(first_call_result) != 0) {
    plan.second_r4 = kCopiedZeroAddress;
  } else if (static_cast<std::uint32_t>(r29) == 0) {
    plan.second_r4 = 0x54;
  } else {
    plan.second_r4 = r2 + 0x10cf8;
  }
  return plan;
}

}  // namespace bandwagon::stage4
