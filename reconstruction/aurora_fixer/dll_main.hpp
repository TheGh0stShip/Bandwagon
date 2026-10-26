#pragma once

#include <cstdint>
#include <string_view>

namespace bandwagon::aurora_fixer {

constexpr std::uint32_t kDllProcessAttach = 1;
constexpr std::uint32_t kDbgBreakPointOrdinal = 1;

class PatchPlatform {
 public:
  virtual ~PatchPlatform() = default;
  virtual void Log(std::string_view message) = 0;
  virtual std::uintptr_t GetCurrentModule() = 0;
  virtual bool PatchImport(std::uintptr_t target_module,
                           std::string_view imported_module,
                           std::uint32_t ordinal,
                           std::uintptr_t replacement) = 0;
};

// Portable reconstruction of AuroraFixerDllMain. The replacement address is
// supplied by the platform layer because the original points at the local
// one-instruction ReturnImmediately routine.
bool AuroraFixerDllMain(std::uint32_t reason, std::uintptr_t replacement,
                        PatchPlatform& platform);

}  // namespace bandwagon::aurora_fixer
