#include "live_dll_main.hpp"

#include "dll_main.hpp"

namespace bandwagon::aurora_fixer {

bool AuroraFixerDllMainLive(std::uint32_t reason,
                            std::uint32_t replacement,
                            LiveDllPlatform* platform) {
  if (reason == kDllProcessAttach && platform) {
    platform->Log("Hello from DLL_PROCESS_ATTACH\n");
    PatchModuleImportByOrdinal(platform->GetCurrentModule(), "xboxkrnl.exe",
                               kDbgBreakPointOrdinal, replacement, platform);
  }
  return true;
}

void ReturnImmediately() {}

}  // namespace bandwagon::aurora_fixer
