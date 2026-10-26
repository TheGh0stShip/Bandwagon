#include "import_stub.hpp"

#include "absolute_branch.hpp"

#include <algorithm>

namespace bandwagon::aurora_fixer {
namespace {

std::uint32_t LoadBigEndian32(const std::uint8_t* input) {
  return (static_cast<std::uint32_t>(input[0]) << 24) |
         (static_cast<std::uint32_t>(input[1]) << 16) |
         (static_cast<std::uint32_t>(input[2]) << 8) | input[3];
}

void StoreBigEndian32(std::uint8_t* output, std::uint32_t value) {
  output[0] = static_cast<std::uint8_t>(value >> 24);
  output[1] = static_cast<std::uint8_t>(value >> 16);
  output[2] = static_cast<std::uint8_t>(value >> 8);
  output[3] = static_cast<std::uint8_t>(value);
}

std::uint32_t DecodeAbsoluteBranchTarget(const std::uint8_t* stub) {
  const std::uint32_t lis = LoadBigEndian32(stub);
  const std::uint32_t addi = LoadBigEndian32(stub + 4);
  if ((lis & 0xffff0000u) != 0x3d600000u ||
      (addi & 0xffff0000u) != 0x396b0000u) {
    return 0;
  }
  const std::uint32_t high = (lis & 0xffffu) << 16;
  const auto low = static_cast<std::int16_t>(addi & 0xffffu);
  return high + static_cast<std::int32_t>(low);
}

}  // namespace

ImportPatchKind PatchImportStub(std::uint8_t* stub, std::size_t size,
                                std::uint32_t resolved_export,
                                std::uint32_t replacement) {
  if (!stub || size < 4) return ImportPatchKind::kNoMatch;
  if (LoadBigEndian32(stub) == resolved_export) {
    StoreBigEndian32(stub, replacement);
    return ImportPatchKind::kDirectPointer;
  }
  if (size < 16 || DecodeAbsoluteBranchTarget(stub) != resolved_export) {
    return ImportPatchKind::kNoMatch;
  }
  const auto encoded = EncodeAbsoluteBranch(replacement, false);
  std::copy(encoded.begin(), encoded.end(), stub);
  return ImportPatchKind::kAbsoluteBranchStub;
}

}  // namespace bandwagon::aurora_fixer
