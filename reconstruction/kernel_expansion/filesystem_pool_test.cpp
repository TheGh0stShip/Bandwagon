#include "filesystem_pool.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

namespace {

class FakePool final
    : public bandwagon::kernel_expansion::FileSystemPoolServices {
 public:
  std::vector<std::uintptr_t> events;

  bandwagon::kernel_expansion::PoolAllocation Allocate(
      std::uint32_t size, std::uint32_t tag,
      std::uint32_t pool_type) override {
    using namespace bandwagon::kernel_expansion;
    assert(size == 0x1234);
    assert(tag == kFileSystemPoolTag);
    assert(tag == 0x66467845);
    assert(pool_type == kFileSystemPoolType);
    events.push_back(size);
    return 0xabcdef00;
  }

  void Free(bandwagon::kernel_expansion::PoolAllocation allocation) override {
    assert(allocation == 0xabcdef00);
    events.push_back(allocation);
  }
};

}  // namespace

int main() {
  using namespace bandwagon::kernel_expansion;

  FakePool pool;
  const PoolAllocation allocation = AllocateFileSystemPool(0x1234, &pool);
  assert(allocation == 0xabcdef00);
  FreeFileSystemPool(allocation, &pool);
  assert((pool.events ==
          std::vector<std::uintptr_t>{0x1234, 0xabcdef00}));

  assert(AllocateFileSystemPool(1, nullptr) == 0);
  FreeFileSystemPool(0, nullptr);

  std::cout << "Kernel Expansion file-system pool tests passed\n";
}
