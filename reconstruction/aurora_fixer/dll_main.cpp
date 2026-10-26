#include "dll_main.hpp"

namespace bandwagon::aurora_fixer {

bool AuroraFixerDllMain(std::uint32_t reason, std::uintptr_t replacement,
                        PatchPlatform& platform) {
  if (reason == kDllProcessAttach) {
    platform.Log("Hello from DLL_PROCESS_ATTACH\n");
    const std::uintptr_t module = platform.GetCurrentModule();
    // The closed DLL deliberately ignores failure and always reports a
    // successful attach to its CRT dispatcher.
    platform.PatchImport(module, "xboxkrnl.exe", kDbgBreakPointOrdinal,
                         replacement);
  }
  return true;
}

}  // namespace bandwagon::aurora_fixer
