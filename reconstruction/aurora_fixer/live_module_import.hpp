#pragma once

#include "patch_runtime.hpp"

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace bandwagon::aurora_fixer {

enum class LiveImportPatchStatus {
  kSuccess,
  kDescriptorUnavailable,
  kExportUnavailable,
  kMalformedDescriptor,
  kLibraryNotImported,
};

struct LiveImportPatchResult {
  LiveImportPatchStatus status = LiveImportPatchStatus::kSuccess;
  std::size_t direct_pointers = 0;
  std::size_t branch_stubs = 0;

  bool patched() const { return direct_pointers != 0 || branch_stubs != 0; }
};

class LiveImportPlatform : public PatchRuntimePlatform {
 public:
  virtual bool GetImportDescriptor(std::uint32_t target_module,
                                   std::vector<std::uint8_t>* descriptor) = 0;
  virtual bool ResolveExport(std::string_view imported_module,
                             std::uint32_t ordinal,
                             std::uint32_t* address) = 0;
  virtual void Log(std::string_view message) = 0;
};

// Source-level lift of PatchModuleImportByOrdinal. Like the original, this
// scans every import record rather than stopping after the first match.
LiveImportPatchResult PatchModuleImportByOrdinal(
    std::uint32_t target_module, std::string_view imported_module,
    std::uint32_t ordinal, std::uint32_t replacement,
    LiveImportPlatform* platform);

}  // namespace bandwagon::aurora_fixer
