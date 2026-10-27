#include "bootanim_hooks.hpp"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <limits>
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

  std::vector<int> shutdown_events;
  BootAnimationShutdownHookServices shutdown_services;
  shutdown_services.shutdown_editor = [&] { shutdown_events.push_back(1); };
  shutdown_services.continue_boot_animation = [&](bool alternate) {
    shutdown_events.push_back(alternate ? 3 : 2);
  };
  RunBootAnimationShutdownHook(false, false, shutdown_services);
  assert((shutdown_events == std::vector<int>{2}));
  shutdown_events.clear();
  RunBootAnimationShutdownHook(true, true, shutdown_services);
  assert((shutdown_events == std::vector<int>{1, 3}));

  assert(SmoothStep(2.0f, 6.0f, 1.0f) == 0.0f);
  assert(SmoothStep(2.0f, 6.0f, 6.0f) == 1.0f);
  assert(std::fabs(SmoothStep(2.0f, 6.0f, 4.0f) - 0.5f) < 0.00001f);
  assert(SmoothStep(2.0f, 6.0f,
                    std::numeric_limits<float>::quiet_NaN()) == 1.0f);
}
