#include "path_fields.hpp"

namespace bandwagon::kernel_expansion {

bool CanOpenExistingPath(PathProbeServices* services, std::string_view path) {
  if (!services) return false;

  FileHandle handle = 0;
  const ExistingPathOpenOptions options;
  if (services->OpenExisting(path, options, &handle) != 0) return false;

  services->Close(handle);
  return true;
}

void CopyPathAndParentDirectory(PathFieldBlock* output, const char* source,
                                std::uint8_t capture_parent) {
  if (!output || !source) return;

  std::size_t length = 0;
  std::size_t final_backslash = 0;
  while (length < kPathFieldCapacity && source[length] != '\0') {
    output->path[length] = source[length];
    if (source[length] == '\\') final_backslash = length;
    ++length;
  }

  // At the exact 256-byte limit, the observed contiguous write lands at the
  // first byte of parent rather than beyond the object.
  if (length < kPathFieldCapacity) {
    output->path[length] = '\0';
  } else {
    output->parent[0] = '\0';
  }

  std::size_t parent_length = 0;
  if (capture_parent == 1 && final_backslash > 0) {
    parent_length = final_backslash;
    for (std::size_t i = 0; i < parent_length; ++i) {
      output->parent[i] = source[i];
    }
  }
  output->parent[parent_length] = '\0';
}

}  // namespace bandwagon::kernel_expansion
