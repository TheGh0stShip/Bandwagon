#pragma once

#include <cstdint>

namespace bandwagon::kernel_expansion {

enum class FileCreateDisposition : std::uint32_t {
  kSupersede = 0,
  kOpen = 1,
  kCreate = 2,
  kOpenIf = 3,
  kOverwrite = 4,
  kOverwriteIf = 5,
};

inline constexpr std::uint32_t kSupersedeOrOverwriteIfFlag = 0x08;
inline constexpr std::uint32_t kCreateFlag = 0x04;
inline constexpr std::uint32_t kOpenIfFlag = 0x10;

class FileDispositionDiagnostics {
 public:
  virtual ~FileDispositionDiagnostics() = default;
  virtual void DebugBreak() = 0;
};

// Lift of the create-disposition mapper at 0x801f92f8. Invalid unsigned values
// invoke the recovered DbgBreakPoint path and then map to zero.
std::uint32_t MapCreateDispositionToOpenFlags(
    std::uint32_t disposition, FileDispositionDiagnostics* diagnostics);

}  // namespace bandwagon::kernel_expansion
