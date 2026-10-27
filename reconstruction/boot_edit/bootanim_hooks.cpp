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

void RunBootAnimationShutdownHook(
    bool editor_visible, bool alternate_continuation,
    const BootAnimationShutdownHookServices& services) {
  if (editor_visible && services.shutdown_editor) services.shutdown_editor();
  if (services.continue_boot_animation) {
    services.continue_boot_animation(alternate_continuation);
  }
}

float SmoothStep(float lower_edge, float upper_edge, float value) {
  if (value < lower_edge) return 0.0f;
  if (!(value < upper_edge)) return 1.0f;
  const float t = (value - lower_edge) / (upper_edge - lower_edge);
  return t * t * (3.0f - 2.0f * t);
}

}  // namespace bandwagon::boot_edit
