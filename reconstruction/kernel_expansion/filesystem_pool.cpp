#include "filesystem_pool.hpp"

namespace bandwagon::kernel_expansion {

PoolAllocation AllocateFileSystemPool(std::uint32_t size,
                                      FileSystemPoolServices* services) {
  if (!services) return 0;
  return services->Allocate(size, kFileSystemPoolTag, kFileSystemPoolType);
}

void FreeFileSystemPool(PoolAllocation allocation,
                        FileSystemPoolServices* services) {
  if (services) services->Free(allocation);
}

}  // namespace bandwagon::kernel_expansion
