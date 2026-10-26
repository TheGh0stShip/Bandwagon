#include "instruction_patch.hpp"

#include <cstring>

namespace bandwagon::kernel_expansion {

void ReplaceInstructionWithNop(std::uint32_t* instruction,
                               InstructionPatchServices* services) {
  if (!instruction || !services) return;

  *instruction = kPpcNopInstruction;
  services->FlushCpuCache(instruction, sizeof(*instruction));
  const std::uintptr_t physical = services->GetPhysicalAddress(instruction);
  services->FlushHypervisorDataCache(physical, sizeof(*instruction));
}

std::uint64_t ReturnImmediatelyHook(std::uint64_t first_argument) {
  return first_argument;
}

bool InstallRelativeBranchHook(InstructionPatchLocation target,
                               std::uint32_t destination_address,
                               std::uint32_t link_mode,
                               void* original_instruction_bytes,
                               InstructionPatchServices* services) {
  if (!target.words || !services) return false;

  const std::uint32_t displacement =
      destination_address - target.virtual_address;
  // The target performs a wrapping 32-bit subtraction and signed compares.
  // Express the same accepted modular intervals without relying on the
  // implementation-defined uint32_t-to-int32_t conversion in C++17.
  if (displacement > static_cast<std::uint32_t>(kPpcRelativeBranchMaximum) &&
      displacement < 0xfe000000u) {
    services->DebugBreak();
    return false;
  }

  if (original_instruction_bytes) {
    std::memcpy(original_instruction_bytes, target.words,
                sizeof(*target.words));
  }
  const std::uint32_t link =
      static_cast<std::uint8_t>(link_mode) == 1 ? 1u : 0u;
  *target.words = kPpcRelativeBranchOpcode |
                  (displacement & kPpcRelativeBranchDisplacementMask) | link;

  services->FlushCpuCache(target.words, sizeof(*target.words));
  const std::uintptr_t physical = services->GetPhysicalAddress(target.words);
  services->FlushHypervisorDataCache(physical, sizeof(*target.words));
  return true;
}

void InstallAbsoluteBranchHook(InstructionPatchLocation target,
                               std::uint32_t destination_address,
                               std::uint32_t link_mode,
                               void* original_instruction_bytes,
                               InstructionPatchServices* services) {
  if (!target.words || !services) return;

  if (original_instruction_bytes) {
    std::memcpy(original_instruction_bytes, target.words,
                4 * sizeof(*target.words));
  }

  std::uint32_t high = destination_address >> 16;
  const std::uint32_t low = destination_address & 0xffffu;
  if ((low & 0x8000u) != 0) ++high;
  // The target forms this word with addis/addi arithmetic rather than an OR;
  // retain the carry into bit 16 when high becomes 0x10000.
  target.words[0] = 0x3d600000u + high;  // lis r11, adjusted high
  target.words[1] = 0x396b0000u + low;   // addi r11, r11, signed low
  target.words[2] = 0x7d6903a6u;         // mtctr r11
  target.words[3] = static_cast<std::uint8_t>(link_mode) == 1
                        ? 0x4e800421u                 // bctrl
                        : 0x4e800420u;                // bctr

  constexpr std::size_t kStubSize = 4 * sizeof(std::uint32_t);
  services->FlushCpuCache(target.words, kStubSize);
  const std::uintptr_t physical = services->GetPhysicalAddress(target.words);
  services->FlushHypervisorDataCache(physical, kStubSize);
}

}  // namespace bandwagon::kernel_expansion
