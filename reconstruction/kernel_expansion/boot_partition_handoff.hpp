#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace bandwagon::kernel_expansion {

struct BootPartitionBinding {
  std::u16string_view partition_name;
  std::string_view device_path;
};

inline constexpr std::array<BootPartitionBinding, 8> kBootPartitionBindings{{
    {u"Partition1", "\\Device\\Harddisk0\\Partition1"},
    {u"Cache0", "\\Device\\Harddisk0\\Cache0"},
    {u"Cache1", "\\Device\\Harddisk0\\Cache1"},
    {u"SystemPartition", "\\Device\\Harddisk0\\SystemPartition"},
    {u"TitleURLCachePartition",
     "\\Device\\Harddisk0\\TitleURLCachePartition"},
    {u"SystemURLCachePartition",
     "\\Device\\Harddisk0\\SystemURLCachePartition"},
    {u"SystemExtPartition", "\\Device\\Harddisk0\\SystemExtPartition"},
    {u"SystemAuxPartition", "\\Device\\Harddisk0\\SystemAuxPartition"},
}};

struct BootPartitionLoaderState {
  std::uintptr_t physical_mapping = 0;  // target offset +0x10
  std::uint32_t mapping_size = 0;       // target offset +0x14
  std::uint32_t mapping_flags = 0;      // target offset +0x18
};

class BootPartitionHandoffServices {
 public:
  virtual ~BootPartitionHandoffServices() = default;
  virtual bool BindPartition(BootPartitionLoaderState* state,
                             std::u16string_view partition_name,
                             std::string_view device_path,
                             bool required) = 0;
  virtual void DebugBreak() = 0;
  virtual void FreePhysicalMemory(std::uint32_t memory_type,
                                  std::uintptr_t address) = 0;
};

// Boot partition binding and loader-mapping teardown at 0x801f0568.
void HandOffBootPartitions(BootPartitionLoaderState* state,
                           BootPartitionHandoffServices* services);

}  // namespace bandwagon::kernel_expansion
