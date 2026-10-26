#include "nand_write_protection.hpp"

#include <cassert>
#include <iostream>

int main() {
  using namespace bandwagon::kernel_expansion;

  NandWriteProtectionState state;
  assert(GetNandWriteProtectionEnabled(&state) == 1);

  SetNandWriteProtectionDisabled(&state, 1);
  assert(GetNandWriteProtectionEnabled(&state) == 0);
  SetNandWriteProtectionDisabled(&state, 0xff);
  assert(GetNandWriteProtectionEnabled(&state) == 0);
  SetNandWriteProtectionDisabled(&state, 0);
  assert(GetNandWriteProtectionEnabled(&state) == 1);

  SetNandWriteProtectionDisabled(&state, 0x100);
  assert(GetNandWriteProtectionEnabled(&state) == 1);
  SetNandWriteProtectionDisabled(&state, 0x101);
  assert(GetNandWriteProtectionEnabled(&state) == 0);

  state.enabled = 0x7f;
  assert(GetNandWriteProtectionEnabled(&state) == 0x7f);
  assert(GetNandWriteProtectionEnabled(nullptr) == 0);
  SetNandWriteProtectionDisabled(nullptr, 0);

  std::cout << "Kernel Expansion NAND write-protection tests passed\n";
}
