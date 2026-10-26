#pragma once

#include <cstdint>

namespace bandwagon::stage4 {

constexpr std::uint64_t SelectPostOutputTarget(std::uint32_t marker,
                                                std::uint64_t incoming_r4) {
  return marker == 0x41414141u ? incoming_r4 : 0x2540;
}

constexpr std::uint64_t CopiedDispatcherTarget() { return 0x600031fb8ULL; }

class ResidentPlatform {
 public:
  virtual ~ResidentPlatform() = default;
  virtual std::uint32_t BranchOriginalDispatcher(
      std::uint64_t r3, std::uint64_t r4, std::uint64_t r5,
      std::uint64_t r6, std::uint64_t r7) = 0;
  virtual std::uint32_t ReadWord(std::uint64_t address) = 0;
  virtual void WriteWord(std::uint64_t address, std::uint32_t value) = 0;
  virtual void StoreDataCacheBlock(std::uint64_t address) = 0;
  virtual void InvalidateInstructionCacheBlock(std::uint64_t address) = 0;
  virtual void SynchronizeInstructionStream() = 0;
  virtual std::uint32_t Execute(std::uint64_t address) = 0;
  virtual std::uint32_t BranchOperation5(std::uint64_t r3, std::uint64_t r4,
                                         std::uint64_t r5) = 0;

  virtual std::uint64_t CallB3d8(std::uint64_t r3, std::uint64_t r4,
                                 std::uint64_t r5, std::uint64_t r6) = 0;
  virtual void CallB388(std::uint64_t r3, std::uint64_t r4) = 0;
  virtual std::uint32_t BranchContinuationExit(std::uint64_t address) = 0;
};

// Executable source lift of the dispatcher installed at 0x600031fb8. Its
// do/while copy preserves CTR semantics: a zero count underflows and attempts
// 2^64 iterations, so callers must supply a nonzero count.
std::uint32_t RunRbtrDispatcher(std::uint64_t r3, std::uint64_t r4,
                                std::uint64_t r5, std::uint64_t r6,
                                std::uint64_t r7,
                                ResidentPlatform* platform);

// Executable source lift of the continuation installed at 0x600032048.
std::uint32_t RunCopiedContinuation(
    std::uint64_t r1, std::uint64_t r2, std::uint64_t r28,
    std::uint64_t r29, std::uint64_t incoming_r30,
    ResidentPlatform* platform);

}  // namespace bandwagon::stage4
