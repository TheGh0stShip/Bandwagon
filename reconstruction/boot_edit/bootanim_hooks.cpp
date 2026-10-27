#include "bootanim_hooks.hpp"

namespace bandwagon::boot_edit {

void InstallBootAnimationHooks(const BootAnimationHookServices& services) {
  if (services.install_jump) {
    for (const BootAnimationHook& hook : kBootAnimationHooks) {
      services.install_jump(hook.patch_address, hook.replacement_address);
    }
  }
  if (services.install_nop) {
    for (const std::uintptr_t address : kBootAnimationNopSites) {
      services.install_nop(address);
    }
  }
}

void InvokeKernelBootAnimationRoutine(
    const std::function<void(std::uint32_t)>& routine) {
  if (routine) routine(kKernelBootAnimationMode);
}

}  // namespace bandwagon::boot_edit
