#pragma once

#include <cstddef>
#include <cstdint>

namespace bandwagon::kernel_expansion {

inline constexpr std::uint32_t kPpcNopInstruction = 0x60000000;

class InstructionPatchServices {
 public:
  virtual ~InstructionPatchServices() = default;
  virtual void FlushCpuCache(const void* address, std::size_t size) = 0;
  virtual std::uintptr_t GetPhysicalAddress(const void* address) = 0;
  virtual void FlushHypervisorDataCache(std::uintptr_t physical_address,
                                        std::size_t size) = 0;
};

// Lift of 0x801f2d70. The routine replaces one big-endian PPC instruction with
// nop, flushes the CPU cache for that word, resolves its physical address, and
// flushes the corresponding hypervisor data-cache range.
void ReplaceInstructionWithNop(std::uint32_t* instruction,
                               InstructionPatchServices* services);

}  // namespace bandwagon::kernel_expansion
