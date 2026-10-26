#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace bandwagon::kernel_expansion {

inline constexpr std::size_t kPathFieldCapacity = 0x100;

// Recovered portion of the destination object used at 0x801faac0. The first
// 0x10 bytes are not touched by this routine. The path and parent fields begin
// at the observed offsets 0x10 and 0x110.
struct PathFieldBlock {
  std::array<std::uint8_t, 0x10> opaque_prefix{};
  std::array<char, kPathFieldCapacity> path{};
  std::array<char, kPathFieldCapacity> parent{};
};

static_assert(offsetof(PathFieldBlock, path) == 0x10);
static_assert(offsetof(PathFieldBlock, parent) == 0x110);

// Copies at most 256 non-NUL bytes into path. capture_parent must equal one to
// also copy everything before the final backslash into parent; other values
// leave parent empty. The binary treats a leading backslash as no parent.
void CopyPathAndParentDirectory(PathFieldBlock* output, const char* source,
                                std::uint8_t capture_parent);

}  // namespace bandwagon::kernel_expansion
