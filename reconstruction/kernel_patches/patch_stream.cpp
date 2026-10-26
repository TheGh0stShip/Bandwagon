#include "patch_stream.hpp"

#include <stdexcept>

namespace bandwagon::kernel_patches {
namespace {

constexpr std::array<PatchRecord, 23> kRecords{{
    {0x8007b920, PatchEffect::kReturnZero},
    {0x8007c4b8, PatchEffect::kForceTrueR3},
    {0x8007c5e8, PatchEffect::kForceFalseR3},
    {0x8007c634, PatchEffect::kClearR11},
    {0x8007c684, PatchEffect::kClearR11},
    {0x8007af08, PatchEffect::kReturnZero},
    {0x800998d0, PatchEffect::kCompareR11With255},
    {0x800992b4, PatchEffect::kForceZeroAndNopFour},
    {0x80109c90, PatchEffect::kReturnOne},
    {0x8010a788, PatchEffect::kReturnZero},
    {0x8010aa68, PatchEffect::kReturnZero},
    {0x8010af30, PatchEffect::kReturnOne},
    {0x8010b138, PatchEffect::kReturnZero},
    {0x8010b0e8, PatchEffect::kReturnZero},
    {0x8010b278, PatchEffect::kReturnZero},
    {0x8010b3f8, PatchEffect::kReturnZero},
    {0x8010bf20, PatchEffect::kReturnOneAndOptionallyStoreOne},
    {0x80111990, PatchEffect::kReturnOne},
    {0x800e1754, PatchEffect::kSkipForward0x18},
    {0x800e17cc, PatchEffect::kLoadR26AndNopTwo},
    {0x8015d9d8, PatchEffect::kReturnOne},
    {0x8009d660, PatchEffect::kSkipForward0x68},
    {0x800d8748, PatchEffect::kReturnOne},
}};

struct EncodedEffect {
  std::array<std::uint32_t, 5> words{};
  std::uint32_t word_count = 0;
};

constexpr EncodedEffect Encode(PatchEffect effect) {
  constexpr std::uint32_t kLiR3Zero = 0x38600000;
  constexpr std::uint32_t kLiR3One = 0x38600001;
  constexpr std::uint32_t kReturn = 0x4e800020;
  constexpr std::uint32_t kNop = 0x60000000;
  switch (effect) {
    case PatchEffect::kReturnZero:
      return {{{kLiR3Zero, kReturn}}, 2};
    case PatchEffect::kForceTrueR3:
      return {{{kLiR3One}}, 1};
    case PatchEffect::kForceFalseR3:
      return {{{kLiR3Zero}}, 1};
    case PatchEffect::kClearR11:
      return {{{0x39600000}}, 1};
    case PatchEffect::kCompareR11With255:
      return {{{0x2b0b00ff}}, 1};
    case PatchEffect::kForceZeroAndNopFour:
      return {{{kLiR3Zero, kNop, kNop, kNop, kNop}}, 5};
    case PatchEffect::kReturnOne:
      return {{{kLiR3One, kReturn}}, 2};
    case PatchEffect::kReturnOneAndOptionallyStoreOne:
      return {{{0x28050000, kLiR3One, 0x41820008, 0x90650000, kReturn}}, 5};
    case PatchEffect::kSkipForward0x18:
      return {{{0x48000018}}, 1};
    case PatchEffect::kLoadR26AndNopTwo:
      return {{{0x835c0098, kNop, kNop}}, 3};
    case PatchEffect::kSkipForward0x68:
      return {{{0x48000068}}, 1};
  }
  throw std::logic_error("unknown kernel patch effect");
}

void AppendBigEndian(std::vector<std::uint8_t>* output, std::uint32_t value) {
  output->push_back(static_cast<std::uint8_t>(value >> 24));
  output->push_back(static_cast<std::uint8_t>(value >> 16));
  output->push_back(static_cast<std::uint8_t>(value >> 8));
  output->push_back(static_cast<std::uint8_t>(value));
}

}  // namespace

const std::array<PatchRecord, 23>& KernelPatchRecords() { return kRecords; }

std::string_view PatchEffectName(PatchEffect effect) {
  switch (effect) {
    case PatchEffect::kReturnZero: return "return zero";
    case PatchEffect::kForceTrueR3: return "set r3 to one";
    case PatchEffect::kForceFalseR3: return "set r3 to zero";
    case PatchEffect::kClearR11: return "clear r11";
    case PatchEffect::kCompareR11With255: return "compare r11 with 255";
    case PatchEffect::kForceZeroAndNopFour: return "set r3 to zero and NOP four instructions";
    case PatchEffect::kReturnOne: return "return one";
    case PatchEffect::kReturnOneAndOptionallyStoreOne:
      return "return one and optionally store one";
    case PatchEffect::kSkipForward0x18: return "skip forward 0x18 bytes";
    case PatchEffect::kLoadR26AndNopTwo: return "load r26 and NOP two instructions";
    case PatchEffect::kSkipForward0x68: return "skip forward 0x68 bytes";
  }
  return "unknown";
}

std::vector<std::uint8_t> BuildKernelPatchStream() {
  std::vector<std::uint8_t> result;
  result.reserve(372);
  for (const auto& record : kRecords) {
    const EncodedEffect encoded = Encode(record.effect);
    AppendBigEndian(&result, record.target);
    AppendBigEndian(&result, encoded.word_count * 4);
    for (std::uint32_t index = 0; index < encoded.word_count; ++index)
      AppendBigEndian(&result, encoded.words[index]);
  }
  AppendBigEndian(&result, 0xffffffff);
  return result;
}

std::uint32_t ReturnOneAndOptionallyStoreOne(std::uint32_t* output) {
  if (output) *output = 1;
  return 1;
}

}  // namespace bandwagon::kernel_patches
