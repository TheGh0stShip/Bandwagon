#include "module_import.hpp"

#include "import_descriptor.hpp"
#include "import_stub.hpp"

#include <vector>

namespace bandwagon::aurora_fixer {

ModuleImportPatchResult PatchModuleImportOffline(
    const std::uint8_t* descriptor, std::size_t descriptor_size,
    std::string_view imported_module, std::uint32_t resolved_export,
    std::uint32_t replacement, std::uint32_t image_base,
    std::uint8_t* image, std::size_t image_size) {
  ModuleImportPatchResult result;
  if (!image) return result;

  std::vector<ImportLibrary> libraries;
  if (!ParseImportDescriptor(descriptor, descriptor_size, &libraries)) {
    return result;
  }
  const ImportLibrary* library = FindImportLibrary(libraries, imported_module);
  if (!library) return result;

  for (const std::uint32_t address : library->stub_addresses) {
    if (address < image_base) continue;
    const std::size_t offset = static_cast<std::size_t>(address - image_base);
    if (offset >= image_size) continue;
    const ImportPatchKind kind = PatchImportStub(
        image + offset, image_size - offset, resolved_export, replacement);
    if (kind == ImportPatchKind::kDirectPointer) {
      ++result.direct_pointers;
    } else if (kind == ImportPatchKind::kAbsoluteBranchStub) {
      ++result.branch_stubs;
    }
  }
  return result;
}

}  // namespace bandwagon::aurora_fixer
