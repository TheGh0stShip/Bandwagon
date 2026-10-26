#include "resident_runtime.hpp"

#include "copied_continuation.hpp"
#include "rbtr_dispatcher.hpp"

namespace bandwagon::stage4 {

std::uint32_t RunRbtrDispatcher(std::uint64_t r3, std::uint64_t r4,
                                std::uint64_t r5, std::uint64_t r6,
                                std::uint64_t r7,
                                ResidentPlatform* platform) {
  if (!platform) return 0;
  if (static_cast<std::uint32_t>(r3) != kRbtrMagic) {
    return platform->BranchOriginalDispatcher(r3, r4, r5, r6, r7);
  }
  const std::uint32_t operation = static_cast<std::uint32_t>(r4);
  if (operation >= 1 && operation <= 3) return 1;
  if (operation == 4) {
    const std::uint64_t entry = r5;
    std::uint64_t remaining = r7;
    do {
      platform->WriteWord(r5, platform->ReadWord(r6));
      platform->StoreDataCacheBlock(r5);
      platform->InvalidateInstructionCacheBlock(r5);
      r5 += 4;
      r6 += 4;
    } while (--remaining != 0);
    platform->SynchronizeInstructionStream();
    return platform->Execute(entry);
  }
  if (operation == 5) return platform->BranchOperation5(r6, r5, r7);
  return 2;
}

std::uint32_t RunCopiedContinuation(
    std::uint64_t r1, std::uint64_t r2, std::uint64_t r28,
    std::uint64_t r29, std::uint64_t incoming_r30,
    ResidentPlatform* platform) {
  if (!platform) return 0;
  const std::uint64_t first_result = platform->CallB3d8(
      r1 + 0x50, r1 + 0x150, incoming_r30, kCopiedKeyAddress);
  const auto plan = PlanCopiedContinuation(r1, r2, r28, r29, incoming_r30,
                                           first_result);
  if (plan.make_second_call)
    platform->CallB388(plan.second_r3, plan.second_r4);
  return platform->BranchContinuationExit(plan.exit_branch_target);
}

}  // namespace bandwagon::stage4
