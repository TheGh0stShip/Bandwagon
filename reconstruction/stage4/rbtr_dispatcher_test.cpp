#include "rbtr_dispatcher.hpp"

#include <cassert>
#include <iostream>

int main() {
  using bandwagon::stage4::DispatchKind;
  using bandwagon::stage4::PlanRbtrDispatch;

  const auto original = PlanRbtrDispatch(0, 4, 1, 2, 3);
  assert(original.kind == DispatchKind::kDelegateOriginal);
  assert(original.branch_target == 0x1cc8);

  for (std::uint32_t operation = 1; operation <= 3; ++operation) {
    const auto plan = PlanRbtrDispatch(0x72627472u, operation, 1, 2, 3);
    assert(plan.kind == DispatchKind::kReturn);
    assert(plan.return_value == 1);
  }
  const auto unsupported = PlanRbtrDispatch(0x72627472u, 6, 1, 2, 3);
  assert(unsupported.kind == DispatchKind::kReturn);
  assert(unsupported.return_value == 2);

  const auto copy = PlanRbtrDispatch(0x72627472u, 4, 0x1000, 0x2000,
                                     0x100000003ULL);
  assert(copy.kind == DispatchKind::kCopyAndExecute);
  assert(copy.copy_destination == 0x1000 && copy.branch_target == 0x1000);
  assert(copy.copy_source == 0x2000);
  assert(copy.copy_word_count == 0x100000003ULL);

  const auto op5 = PlanRbtrDispatch(0x72627472u, 5, 0x50, 0x60, 0x70);
  assert(op5.kind == DispatchKind::kDelegateOperation5);
  assert(op5.branch_target == 0xa880);
  assert(op5.delegated_r3 == 0x60);
  assert(op5.delegated_r4 == 0x50);
  assert(op5.delegated_r5 == 0x70);

  std::cout << "Stage4 rbtr-dispatcher reconstruction tests passed\n";
}
