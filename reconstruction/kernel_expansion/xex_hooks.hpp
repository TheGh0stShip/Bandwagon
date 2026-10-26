#pragma once

#include <cstddef>
#include <cstdint>

namespace bandwagon::kernel_expansion {

inline constexpr std::uint32_t kRetailKernelImageBase = 0x80040000;
inline constexpr std::uint32_t kFirstXexDataHandler = 0x801f3120;
inline constexpr std::uint32_t kSecondXexDataHandler = 0x801f31a0;

class XexHookServices {
 public:
  virtual ~XexHookServices() = default;

  virtual bool FindNtHeader(const std::uint8_t* mapped_image,
                            std::size_t image_size,
                            std::size_t* nt_header_offset) = 0;
  virtual void FlushCpuCache(std::size_t image_offset,
                             std::size_t size) = 0;
  virtual std::uint64_t PhysicalAddress(std::size_t image_offset) = 0;
  virtual void FlushHypervisorDataCache(std::uint64_t physical_address,
                                        std::size_t size) = 0;
};

// Lift of the routine at 0x801f1790, previously named only for its first
// action. It locates the mapped retail kernel's .xedata section, installs two
// relocated handler words in its native big-endian table, updates the active
// count, and performs both recovered cache-maintenance operations.
bool InstallKernelXexDataHandlers(
    std::uint8_t* mapped_image, std::size_t image_size,
    XexHookServices* services,
    std::uint32_t first_handler = kFirstXexDataHandler,
    std::uint32_t second_handler = kSecondXexDataHandler);

}  // namespace bandwagon::kernel_expansion
