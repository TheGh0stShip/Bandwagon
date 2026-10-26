#pragma once

#include <cstdint>

namespace bandwagon::kernel_expansion {

struct BootstrapMappingRelease {
  std::uintptr_t region = 0x80170880;
  std::uint32_t opaque_argument = 0x7a020000;
  std::uint32_t size = 0x0007c000;
  std::uint32_t flags = 0;
};

class BootstrapMappingServices {
 public:
  virtual ~BootstrapMappingServices() = default;
  virtual std::uint32_t FreeMappedMemory(
      const BootstrapMappingRelease& request) = 0;
};

// Lift of 0x801f2db8, called by the low-level bootstrap transition after its
// stack relocation. The second MiFreeMappedMemory argument has no recovered
// public type and therefore remains explicitly opaque.
std::uint32_t ReleaseBootstrapMapping(BootstrapMappingServices* services);

}  // namespace bandwagon::kernel_expansion
