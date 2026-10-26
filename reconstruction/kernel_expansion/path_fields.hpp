#pragma once

#include "runtime.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace bandwagon::kernel_expansion {

inline constexpr std::size_t kPathFieldCapacity = 0x100;

// Recovered portion of the destination object used at 0x801faac0. The first
// 0x10 bytes are not touched by this routine. The path and parent fields begin
// at the observed offsets 0x10 and 0x110.
struct PathFieldBlock {
  std::array<std::uint8_t, 0x10> opaque_prefix{};
  std::array<char, kPathFieldCapacity> path{};
  std::array<char, kPathFieldCapacity> parent{};
};

static_assert(offsetof(PathFieldBlock, path) == 0x10);
static_assert(offsetof(PathFieldBlock, parent) == 0x110);

struct ExistingPathOpenOptions {
  std::uint32_t desired_access = 0x80100080;
  std::uint32_t object_attributes = 0x40;
  std::uint32_t file_attributes = 0x80;
  std::uint32_t share_access = 1;
  std::uint32_t disposition = 1;
  std::uint32_t create_options = 0x60;
};

class PathProbeServices {
 public:
  virtual ~PathProbeServices() = default;
  virtual int OpenExisting(std::string_view path,
                           const ExistingPathOpenOptions& options,
                           FileHandle* handle) = 0;
  virtual void Close(FileHandle handle) = 0;
};

// Lift of the existence/accessibility probe at 0x801faa08. A successful open
// is always closed before true is returned; a failed open is not closed.
bool CanOpenExistingPath(PathProbeServices* services, std::string_view path);

// Copies at most 256 non-NUL bytes into path. capture_parent must equal one to
// also copy everything before the final backslash into parent; other values
// leave parent empty. The binary treats a leading backslash as no parent.
void CopyPathAndParentDirectory(PathFieldBlock* output, const char* source,
                                std::uint8_t capture_parent);

}  // namespace bandwagon::kernel_expansion
