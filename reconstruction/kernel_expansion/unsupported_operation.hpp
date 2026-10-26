#pragma once

#include <cstdint>

namespace bandwagon::kernel_expansion {

inline constexpr std::uint32_t kStatusNotImplemented = 0xc0000002u;

class UnsupportedOperationDiagnostics {
 public:
  virtual ~UnsupportedOperationDiagnostics() = default;
  virtual void DebugBreak() = 0;
};

// Lift of the device-operation vtable stub at 0x801f62c0.
std::uint32_t UnsupportedDeviceOperation(
    UnsupportedOperationDiagnostics* diagnostics);

}  // namespace bandwagon::kernel_expansion
