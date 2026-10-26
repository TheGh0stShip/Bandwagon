#pragma once

#include "runtime.hpp"

#include <cstdint>
#include <string_view>

namespace bandwagon::kernel_expansion {

inline constexpr std::string_view kUpdateCheckPath =
    "PPROOT:\\System\\UpdateCheck.xex";

struct UpdateCheckState {
  std::uintptr_t nic_attachment = 0;
  std::uintptr_t referenced_object = 0;
  std::uint32_t referenced_object_auxiliary = 0;
  bool update_available = false;
  std::uint32_t latest_version = 0;
  std::uint32_t launch_phase = 0;
};

using SpinLockCookie = std::uint32_t;

class UpdateCheckServices {
 public:
  virtual ~UpdateCheckServices() = default;

  virtual void DetachNic(std::uintptr_t attachment) = 0;
  virtual SpinLockCookie AcquireStateLock() = 0;
  virtual void ReleaseStateLock(SpinLockCookie cookie) = 0;
  virtual void DereferenceObject(std::uintptr_t object) = 0;

  virtual int LoadImage(std::string_view path, std::uint32_t flags,
                        std::uint32_t minimum_version,
                        ModuleHandle* module) = 0;
  virtual int ResolveProcedure(ModuleHandle module, std::uint32_t ordinal,
                               Procedure* procedure) = 0;
  virtual bool InvokeVersionCheck(Procedure procedure,
                                  std::uint32_t* version) = 0;
  virtual void PrepareModuleForUnload(ModuleHandle module) = 0;
  virtual void UnloadImage(ModuleHandle module) = 0;
};

// Lift of LaunchUpdateCheck at 0x801fa798. The original always returns zero;
// its observable result is the synchronized UpdateCheckState mutation.
std::uint32_t LaunchUpdateCheck(UpdateCheckState* state,
                                UpdateCheckServices* services);

}  // namespace bandwagon::kernel_expansion
