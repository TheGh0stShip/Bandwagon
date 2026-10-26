#include "xex_load_hooks.hpp"

#include <cstddef>
#include <cstring>

namespace bandwagon::kernel_expansion {
namespace {

unsigned char FoldAscii(unsigned char value) {
  if (value >= 'A' && value <= 'Z') {
    return static_cast<unsigned char>(value + ('a' - 'A'));
  }
  return value;
}

bool EndsWithCaseInsensitive(const char* path, std::size_t path_size,
                             std::string_view suffix) {
  if (suffix.size() > path_size) return false;
  const char* const candidate = path + path_size - suffix.size();
  for (std::size_t index = 0; index < suffix.size(); ++index) {
    if (FoldAscii(static_cast<unsigned char>(candidate[index])) !=
        FoldAscii(static_cast<unsigned char>(suffix[index]))) {
      return false;
    }
  }
  return true;
}

}  // namespace

bool IsBlockedXexLoadPath(const char* path, std::uint32_t load_flags) {
  if (!path) return false;

  const std::size_t path_size = std::strlen(path);
  if (EndsWithCaseInsensitive(path, path_size, kBlockedLaunchXexSuffix) &&
      load_flags == kXexDllLoadFlag) {
    return true;
  }
  return EndsWithCaseInsensitive(path, path_size, kBlockedNovaXexSuffix);
}

std::uint32_t FilterBlockedXexLoadPaths(
    const char* path, std::uint32_t load_flags, std::uintptr_t argument3,
    std::uintptr_t argument4, XexLoadHookServices* services) {
  if (IsBlockedXexLoadPath(path, load_flags)) {
    return kStatusObjectNameNotFound;
  }
  // The target has no injected-service concept. A missing host adapter is
  // represented by the same non-success status used for a blocked load.
  if (!services) return kStatusObjectNameNotFound;
  return services->ForwardXexLoad(path, load_flags, argument3, argument4);
}

}  // namespace bandwagon::kernel_expansion
