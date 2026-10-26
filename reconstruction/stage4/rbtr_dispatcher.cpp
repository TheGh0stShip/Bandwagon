#include "rbtr_dispatcher.hpp"

namespace bandwagon::stage4 {

DispatchPlan PlanRbtrDispatch(std::uint32_t magic, std::uint32_t operation,
                              std::uint64_t r5, std::uint64_t r6,
                              std::uint64_t r7) {
  DispatchPlan plan;
  if (magic != kRbtrMagic) {
    plan.kind = DispatchKind::kDelegateOriginal;
    plan.branch_target = 0x1cc8;
    return plan;
  }
  if (operation >= 1 && operation <= 3) {
    plan.kind = DispatchKind::kReturn;
    plan.return_value = 1;
    return plan;
  }
  if (operation == 4) {
    plan.kind = DispatchKind::kCopyAndExecute;
    plan.branch_target = r5;
    plan.copy_destination = r5;
    plan.copy_source = r6;
    plan.copy_word_count = r7;
    return plan;
  }
  if (operation == 5) {
    plan.kind = DispatchKind::kDelegateOperation5;
    plan.branch_target = 0xa880;
    plan.delegated_r3 = r6;
    plan.delegated_r4 = r5;
    plan.delegated_r5 = r7;
    return plan;
  }
  plan.kind = DispatchKind::kReturn;
  plan.return_value = 2;
  return plan;
}

}  // namespace bandwagon::stage4
