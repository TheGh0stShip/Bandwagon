#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace bandwagon::aurora_fixer {

struct ModuleImportPatchResult {
  std::size_t direct_pointers = 0;
  std::size_t branch_stubs = 0;

  bool patched() const { return direct_pointers != 0 || branch_stubs != 0; }
};

// Offline reconstruction of the mutation portion of
// PatchModuleImportByOrdinal after the loader has resolved an ordinal.
ModuleImportPatchResult PatchModuleImportOffline(
    const std::uint8_t* descriptor, std::size_t descriptor_size,
    std::string_view imported_module, std::uint32_t resolved_export,
    std::uint32_t replacement, std::uint32_t image_base,
    std::uint8_t* image, std::size_t image_size);

}  // namespace bandwagon::aurora_fixer
