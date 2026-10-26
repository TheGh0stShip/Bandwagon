#pragma once

#include <cstddef>
#include <cstdint>

namespace bandwagon::aurora_fixer {

enum class ImportPatchKind {
  kNoMatch,
  kDirectPointer,
  kAbsoluteBranchStub,
};

// Models the two import record forms handled by PatchModuleImportByOrdinal.
// The caller remains responsible for cache maintenance on a live target.
ImportPatchKind PatchImportStub(std::uint8_t* stub, std::size_t size,
                                std::uint32_t resolved_export,
                                std::uint32_t replacement);

}  // namespace bandwagon::aurora_fixer
