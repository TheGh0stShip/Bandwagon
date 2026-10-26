#include "dll_main.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>

namespace {

class FakePlatform final : public bandwagon::aurora_fixer::PatchPlatform {
 public:
  void Log(std::string_view message) override { log.append(message); }
  std::uintptr_t GetCurrentModule() override {
    ++module_queries;
    return 0x88000000u;
  }
  bool PatchImport(std::uintptr_t target_module,
                   std::string_view imported_module, std::uint32_t ordinal,
                   std::uintptr_t replacement) override {
    ++patch_calls;
    observed_target = target_module;
    observed_module = imported_module;
    observed_ordinal = ordinal;
    observed_replacement = replacement;
    return patch_result;
  }

  bool patch_result = false;
  int module_queries = 0;
  int patch_calls = 0;
  std::uintptr_t observed_target = 0;
  std::string observed_module;
  std::uint32_t observed_ordinal = 0;
  std::uintptr_t observed_replacement = 0;
  std::string log;
};

}  // namespace

int main() {
  FakePlatform platform;
  assert(bandwagon::aurora_fixer::AuroraFixerDllMain(
      bandwagon::aurora_fixer::kDllProcessAttach, 0x880102f8u, platform));
  assert(platform.log == "Hello from DLL_PROCESS_ATTACH\n");
  assert(platform.module_queries == 1);
  assert(platform.patch_calls == 1);
  assert(platform.observed_target == 0x88000000u);
  assert(platform.observed_module == "xboxkrnl.exe");
  assert(platform.observed_ordinal == 1);
  assert(platform.observed_replacement == 0x880102f8u);

  // Patch failure is intentionally ignored by the original entry routine.
  assert(!platform.patch_result);
  FakePlatform detach;
  assert(bandwagon::aurora_fixer::AuroraFixerDllMain(0, 0x880102f8u,
                                                     detach));
  assert(detach.log.empty());
  assert(detach.module_queries == 0);
  assert(detach.patch_calls == 0);

  std::cout << "AuroraFixer DllMain reconstruction tests passed\n";
}
