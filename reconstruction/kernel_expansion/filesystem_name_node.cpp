#include "filesystem_name_node.hpp"

namespace bandwagon::kernel_expansion {

FileSystemNameNode* CloneFileSystemNameNode(
    FileSystemNameNode* source, bool initialize_list,
    FileSystemNameNodeAllocator* allocator) {
  const std::size_t byte_size =
      static_cast<std::size_t>(source->name_length + 0x54u) * 2u;
  FileSystemNameNode* const clone = allocator->AllocateNameNode(
      byte_size, (source->flags & 1u) != 0, initialize_list);
  if (clone == nullptr) return nullptr;

  clone->name_length = source->name_length;
  clone->name.assign(source->name.begin(),
                     source->name.begin() + source->name_length);
  clone->paired = source;
  source->paired = clone;
  return clone;
}

}  // namespace bandwagon::kernel_expansion
