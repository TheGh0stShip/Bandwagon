#include "copied_continuation.hpp"

#include <cassert>
#include <iostream>

int main() {
  using bandwagon::stage4::PlanCopiedContinuation;

  const auto no_second =
      PlanCopiedContinuation(0x1000, 0x2000, 0, 1, 0x30, 1);
  assert(no_second.first_call_target == 0xb3d8);
  assert(no_second.first_r3 == 0x1050);
  assert(no_second.first_r4 == 0x1150);
  assert(no_second.first_r5 == 0x30);
  assert(no_second.first_r6 == 0x6000320b8ULL);
  assert(!no_second.make_second_call);
  assert(no_second.exit_branch_target == 0x400029b3cULL);

  const auto success =
      PlanCopiedContinuation(0, 0x2000, 0x28, 0, 0, 1);
  assert(success.make_second_call);
  assert(success.second_call_target == 0xb388);
  assert(success.second_r3 == 0x28);
  assert(success.second_r4 == 0x6000321c8ULL);

  const auto fallback =
      PlanCopiedContinuation(0, 0x2000, 0x28, 0, 0, 0);
  assert(fallback.second_r4 == 0x54);
  const auto toc_argument =
      PlanCopiedContinuation(0, 0x2000, 0x28, 1, 0, 0);
  assert(toc_argument.second_r4 == 0x12cf8);

  const auto low_words = PlanCopiedContinuation(
      0, 0x2000, 0x28, 0x100000000ULL, 0, 0x100000000ULL);
  assert(low_words.second_r4 == 0x54);

  std::cout << "Stage4 copied-continuation reconstruction tests passed\n";
}
