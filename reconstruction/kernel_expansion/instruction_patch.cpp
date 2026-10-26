#include "instruction_patch.hpp"

namespace bandwagon::kernel_expansion {

void ReplaceInstructionWithNop(std::uint32_t* instruction,
                               InstructionPatchServices* services) {
  if (!instruction || !services) return;

  *instruction = kPpcNopInstruction;
  services->FlushCpuCache(instruction, sizeof(*instruction));
  const std::uintptr_t physical = services->GetPhysicalAddress(instruction);
  services->FlushHypervisorDataCache(physical, sizeof(*instruction));
}

}  // namespace bandwagon::kernel_expansion
