#include "nand_write_protection.hpp"

namespace bandwagon::kernel_expansion {

std::uint8_t GetNandWriteProtectionEnabled(
    const NandWriteProtectionState* state) {
  return state ? state->enabled : 0;
}

void SetNandWriteProtectionDisabled(NandWriteProtectionState* state,
                                    std::uint32_t disabled) {
  if (!state) return;
  state->enabled = static_cast<std::uint8_t>(disabled) == 0 ? 1 : 0;
}

}  // namespace bandwagon::kernel_expansion
