#pragma once

#include <cstdint>

namespace bandwagon::kernel_expansion {

// Host representation of the target byte at 0x802037f4. Nonzero enables the
// four NAND write-protection hook wrappers installed by Kernel Expansion.
struct NandWriteProtectionState {
  std::uint8_t enabled = 1;
};

// Table-exposed leaf accessors at 0x801f14a8 and 0x801f14b8.
std::uint8_t GetNandWriteProtectionEnabled(
    const NandWriteProtectionState* state);
void SetNandWriteProtectionDisabled(NandWriteProtectionState* state,
                                    std::uint32_t disabled);

}  // namespace bandwagon::kernel_expansion
