#include "filesystem_name_node.hpp"

#include <cassert>
#include <cstddef>
#include <iostream>

using namespace bandwagon::kernel_expansion;

namespace {

class FakeAllocator final : public FileSystemNameNodeAllocator {
 public:
  FileSystemNameNode* AllocateNameNode(std::size_t byte_size,
                                       bool flag_bit_zero,
                                       bool initialize_list) override {
    observed_size = byte_size;
    observed_flag = flag_bit_zero;
    observed_initialize_list = initialize_list;
    return fail ? nullptr : &storage;
  }

  bool fail = false;
  std::size_t observed_size = 0;
  bool observed_flag = false;
  bool observed_initialize_list = false;
  FileSystemNameNode storage;
};

}  // namespace

int main() {
  FileSystemNameNode source;
  source.flags = 0x15;
  source.name_length = 4;
  source.name = {'T', 'e', 's', 't', 0, 'X'};

  FakeAllocator allocator;
  FileSystemNameNode* clone =
      CloneFileSystemNameNode(&source, true, &allocator);
  assert(clone == &allocator.storage);
  assert(allocator.observed_size == (4u + 0x54u) * 2u);
  assert(allocator.observed_flag);
  assert(allocator.observed_initialize_list);
  assert(clone->name_length == 4);
  assert((clone->name == std::vector<std::uint16_t>{'T', 'e', 's', 't'}));
  assert(clone->paired == &source);
  assert(source.paired == clone);

  FileSystemNameNode failure_source;
  failure_source.flags = 2;
  failure_source.name_length = 1;
  failure_source.name = {'Q'};
  FakeAllocator failure_allocator;
  failure_allocator.fail = true;
  assert(CloneFileSystemNameNode(&failure_source, false,
                                 &failure_allocator) == nullptr);
  assert(failure_allocator.observed_size == (1u + 0x54u) * 2u);
  assert(!failure_allocator.observed_flag);
  assert(!failure_allocator.observed_initialize_list);
  assert(failure_source.paired == nullptr);

  std::cout << "Kernel Expansion filesystem name-node tests passed\n";
}
