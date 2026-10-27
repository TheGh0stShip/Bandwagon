#include "bootanim_hooks.hpp"

#include <cassert>
#include <cstdint>
#include <utility>
#include <vector>

using namespace bandwagon::boot_edit;

int main() {
  std::vector<std::pair<std::uintptr_t, std::uintptr_t>> jumps;
  std::vector<std::uintptr_t> nops;
  BootAnimationHookServices services;
  services.install_jump = [&](std::uintptr_t patch,
                              std::uintptr_t replacement) {
    jumps.emplace_back(patch, replacement);
  };
  services.install_nop = [&](std::uintptr_t address) { nops.push_back(address); };
  InstallBootAnimationHooks(services);
  assert(jumps.size() == kBootAnimationHooks.size());
  for (std::size_t i = 0; i < jumps.size(); ++i) {
    assert(jumps[i].first == kBootAnimationHooks[i].patch_address);
    assert(jumps[i].second == kBootAnimationHooks[i].replacement_address);
  }
  assert(nops == std::vector<std::uintptr_t>(kBootAnimationNopSites.begin(),
                                              kBootAnimationNopSites.end()));

  std::uint32_t mode = 0;
  InvokeKernelBootAnimationRoutine(
      [&](std::uint32_t value) { mode = value; });
  assert(mode == 0x80000000u);
  InvokeKernelBootAnimationRoutine({});
}
