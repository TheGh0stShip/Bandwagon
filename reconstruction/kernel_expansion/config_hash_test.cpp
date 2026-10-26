#include "config_hash.hpp"

#include <cassert>
#include <iostream>

int main() {
  using bandwagon::kernel_expansion::ComputeConfigKeyHash;

  assert(ComputeConfigKeyHash("") == 0);
  assert(ComputeConfigKeyHash("abc") == ComputeConfigKeyHash("ABC"));
  assert(ComputeConfigKeyHash("Mixed-Case_123") ==
         ComputeConfigKeyHash("MIXED-CASE_123"));

  assert(ComputeConfigKeyHash("BootAnimationType") == 0x74090f50);
  assert(ComputeConfigKeyHash("BootAnimationThemeName") == 0x529534db);
  assert(ComputeConfigKeyHash("DashboardPath") == 0xa7a2d18e);
  assert(ComputeConfigKeyHash("CheckForUpdatesOnStartup") == 0x6682f927);
  assert(ComputeConfigKeyHash("EjectButtonAction") == 0x1fd224bb);
  assert(ComputeConfigKeyHash("OtherOSName") == 0xe6faaef3);
  assert(ComputeConfigKeyHash(nullptr) == 0);

  std::cout << "Kernel Expansion config hash tests passed\n";
}
