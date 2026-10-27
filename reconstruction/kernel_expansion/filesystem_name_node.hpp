#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace bandwagon::kernel_expansion {

struct FileSystemNameNode {
  std::uint32_t flags = 0;          // target offset +0x08
  FileSystemNameNode* paired = nullptr;  // target offset +0x10
  std::uint32_t name_length = 0;    // target offset +0xa0, UTF-16 units
  std::vector<std::uint16_t> name;  // target offset +0xa4
};

class FileSystemNameNodeAllocator {
 public:
  virtual ~FileSystemNameNodeAllocator() = default;
  virtual FileSystemNameNode* AllocateNameNode(std::size_t byte_size,
                                                bool flag_bit_zero,
                                                bool initialize_list) = 0;
};

// Clone-and-pair helper at 0x801faf60. The allocation size preserves the
// target's (name_length + 0x54) * 2 calculation.
FileSystemNameNode* CloneFileSystemNameNode(
    FileSystemNameNode* source, bool initialize_list,
    FileSystemNameNodeAllocator* allocator);

}  // namespace bandwagon::kernel_expansion
