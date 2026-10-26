#pragma once

#include <cstdint>
#include <string_view>

namespace bandwagon::kernel_expansion {

inline constexpr std::string_view kBlockedLaunchXexSuffix = "launch.xex";
inline constexpr std::string_view kBlockedNovaXexSuffix =
    "Plugins\\Nova.xex";
inline constexpr std::uint32_t kXexDllLoadFlag = 8;
inline constexpr std::uint32_t kStatusObjectNameNotFound = 0xc0000034u;

class XexLoadHookServices {
 public:
  virtual ~XexLoadHookServices() = default;

  virtual std::uint32_t ForwardXexLoad(const char* path,
                                       std::uint32_t load_flags,
                                       std::uintptr_t argument3,
                                       std::uintptr_t argument4) = 0;
};

// Readable source model for the predicate at 0x801f3218. Matching is against
// case-insensitive path suffixes. The target assumes path is non-null; the
// null handling here is a host-only safety addition.
bool IsBlockedXexLoadPath(const char* path, std::uint32_t load_flags);

// Lift of 0x801f19c8. All four target arguments are forwarded unchanged when
// the path policy permits the load; blocked loads are hidden as not found.
std::uint32_t FilterBlockedXexLoadPaths(
    const char* path, std::uint32_t load_flags, std::uintptr_t argument3,
    std::uintptr_t argument4, XexLoadHookServices* services);

}  // namespace bandwagon::kernel_expansion
