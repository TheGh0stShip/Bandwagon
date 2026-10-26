#pragma once

#include <cstdint>

namespace bandwagon::kernel_expansion {

inline constexpr std::uint32_t kFileSystemPoolTag = 0x66467845;
inline constexpr std::uint32_t kFileSystemPoolType = 2;
using PoolAllocation = std::uintptr_t;

class FileSystemPoolServices {
 public:
  virtual ~FileSystemPoolServices() = default;
  virtual PoolAllocation Allocate(std::uint32_t size, std::uint32_t tag,
                                  std::uint32_t pool_type) = 0;
  virtual void Free(PoolAllocation allocation) = 0;
};

// Lifts of the fixed-tag allocation wrapper at 0x801fafe0 and direct free
// wrapper at 0x801fb4e8.
PoolAllocation AllocateFileSystemPool(std::uint32_t size,
                                      FileSystemPoolServices* services);
void FreeFileSystemPool(PoolAllocation allocation,
                        FileSystemPoolServices* services);

}  // namespace bandwagon::kernel_expansion
