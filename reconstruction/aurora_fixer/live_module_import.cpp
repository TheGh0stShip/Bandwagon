#include "live_module_import.hpp"

#include "import_descriptor.hpp"
#include "import_stub.hpp"

#include <array>

namespace bandwagon::aurora_fixer {

LiveImportPatchResult PatchModuleImportByOrdinal(
    std::uint32_t target_module, std::string_view imported_module,
    std::uint32_t ordinal, std::uint32_t replacement,
    LiveImportPlatform* platform) {
  LiveImportPatchResult result;
  if (!platform) {
    result.status = LiveImportPatchStatus::kDescriptorUnavailable;
    return result;
  }

  std::vector<std::uint8_t> descriptor;
  if (!platform->GetImportDescriptor(target_module, &descriptor)) {
    platform->Log("could not obtain XEX import descriptor");
    result.status = LiveImportPatchStatus::kDescriptorUnavailable;
    return result;
  }

  std::uint32_t resolved_export = 0;
  if (!platform->ResolveExport(imported_module, ordinal, &resolved_export)) {
    platform->Log("could not resolve imported ordinal");
    result.status = LiveImportPatchStatus::kExportUnavailable;
    return result;
  }

  std::vector<ImportLibrary> libraries;
  if (!ParseImportDescriptor(descriptor.data(), descriptor.size(), &libraries)) {
    platform->Log("malformed XEX import descriptor");
    result.status = LiveImportPatchStatus::kMalformedDescriptor;
    return result;
  }
  const ImportLibrary* library = FindImportLibrary(libraries, imported_module);
  if (!library) {
    platform->Log("module is not present in XEX import descriptor");
    result.status = LiveImportPatchStatus::kLibraryNotImported;
    return result;
  }

  for (const std::uint32_t address : library->stub_addresses) {
    std::array<std::uint8_t, 16> bytes{};
    platform->ReadMemory(address, bytes.data(), bytes.size());
    const ImportPatchKind kind = PatchImportStub(
        bytes.data(), bytes.size(), resolved_export, replacement);
    if (kind == ImportPatchKind::kDirectPointer) {
      platform->WriteMemory(address, bytes.data(), 4);
      platform->MmGetPhysicalAddress(address);
      platform->KeFlushCacheRange(address, 4);
      platform->HvxFlushDcacheRange(address, 4);
      ++result.direct_pointers;
    } else if (kind == ImportPatchKind::kAbsoluteBranchStub) {
      PatchAbsoluteBranch(address, replacement, false, nullptr, platform);
      ++result.branch_stubs;
    }
  }
  return result;
}

}  // namespace bandwagon::aurora_fixer
