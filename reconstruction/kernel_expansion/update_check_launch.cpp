#include "update_check_launch.hpp"

namespace bandwagon::kernel_expansion {

std::uint32_t LaunchUpdateCheck(UpdateCheckState* state,
                                UpdateCheckServices* services) {
  if (!state || !services) return 0;

  services->DetachNic(state->nic_attachment);
  state->nic_attachment = 0;

  SpinLockCookie cookie = services->AcquireStateLock();
  if (state->referenced_object != 0) {
    state->referenced_object_auxiliary = 0;
    services->DereferenceObject(state->referenced_object);
    state->referenced_object = 0;
  }
  services->ReleaseStateLock(cookie);

  ModuleHandle module = 0;
  if (services->LoadImage(kUpdateCheckPath, kXexLoadFlags, 0, &module) == 0) {
    Procedure entry = 0;
    if (services->ResolveProcedure(module, kXexEntryOrdinal, &entry) == 0) {
      std::uint32_t version = 0;
      if (services->InvokeVersionCheck(entry, &version)) {
        cookie = services->AcquireStateLock();
        if (version > state->latest_version) {
          state->latest_version = version;
          state->update_available = true;
          state->launch_phase = 3;
        }
        services->ReleaseStateLock(cookie);
      }
    }
  }

  if (module != 0) {
    services->PrepareModuleForUnload(module);
    services->UnloadImage(module);
  }
  return 0;
}

}  // namespace bandwagon::kernel_expansion
