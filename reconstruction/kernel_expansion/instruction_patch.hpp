#pragma once

#include <cstddef>
#include <cstdint>

namespace bandwagon::kernel_expansion {

inline constexpr std::uint32_t kPpcNopInstruction = 0x60000000;
inline constexpr std::uint32_t kPpcRelativeBranchOpcode = 0x48000000;
inline constexpr std::uint32_t kPpcRelativeBranchDisplacementMask =
    0x03fffffcu;
inline constexpr std::int32_t kPpcRelativeBranchMinimum = -0x02000000;
inline constexpr std::int32_t kPpcRelativeBranchMaximum = 0x01fffffc;

// Separates the target's 32-bit virtual address, which is used to encode a
// branch displacement, from portable host storage used by reconstruction
// tests.
struct InstructionPatchLocation {
  std::uint32_t virtual_address = 0;
  std::uint32_t* words = nullptr;
};

class InstructionPatchServices {
 public:
  virtual ~InstructionPatchServices() = default;
  virtual void FlushCpuCache(const void* address, std::size_t size) = 0;
  virtual std::uintptr_t GetPhysicalAddress(const void* address) = 0;
  virtual void FlushHypervisorDataCache(std::uintptr_t physical_address,
                                        std::size_t size) = 0;
  virtual void DebugBreak() = 0;
};

// Lift of 0x801f2d70. The routine replaces one big-endian PPC instruction with
// nop, flushes the CPU cache for that word, resolves its physical address, and
// flushes the corresponding hypervisor data-cache range.
void ReplaceInstructionWithNop(std::uint32_t* instruction,
                               InstructionPatchServices* services);

// Address-taken `blr; padding` leaf at 0x801f18e8. A bare return preserves the
// incoming r3; this typed model exposes that observable first-result register.
std::uint64_t ReturnImmediatelyHook(std::uint64_t first_argument);

// Lift of 0x801f03c0. Writes one PPC relative branch when destination is in
// the signed 26-bit branch range. Only a link_mode low byte exactly equal to
// one sets LK. The original instruction is optionally copied first.
bool InstallRelativeBranchHook(InstructionPatchLocation target,
                               std::uint32_t destination_address,
                               std::uint32_t link_mode,
                               void* original_instruction_bytes,
                               InstructionPatchServices* services);

// Lift of 0x801f18f0. Writes lis/addi/mtctr/bctr(l) as a four-word absolute
// branch stub and optionally preserves the overwritten words first.
void InstallAbsoluteBranchHook(InstructionPatchLocation target,
                               std::uint32_t destination_address,
                               std::uint32_t link_mode,
                               void* original_instruction_bytes,
                               InstructionPatchServices* services);

}  // namespace bandwagon::kernel_expansion
