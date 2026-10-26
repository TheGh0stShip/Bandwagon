#pragma once

#include <array>
#include <cstdint>
#include <string_view>
#include <vector>

namespace bandwagon::kernel_patches {

enum class PatchEffect {
  kReturnZero,
  kForceTrueR3,
  kForceFalseR3,
  kClearR11,
  kCompareR11With255,
  kForceZeroAndNopFour,
  kReturnOne,
  kReturnOneAndOptionallyStoreOne,
  kSkipForward0x18,
  kLoadR26AndNopTwo,
  kSkipForward0x68,
};

struct PatchRecord {
  std::uint32_t target;
  PatchEffect effect;
};

const std::array<PatchRecord, 23>& KernelPatchRecords();
std::string_view PatchEffectName(PatchEffect effect);

// Serializes the typed record table into the original big-endian stream,
// including its terminal 0xffffffff word.
std::vector<std::uint8_t> BuildKernelPatchStream();

// Readable behavior of the only nontrivial replacement helper.
std::uint32_t ReturnOneAndOptionallyStoreOne(std::uint32_t* output);

}  // namespace bandwagon::kernel_patches
