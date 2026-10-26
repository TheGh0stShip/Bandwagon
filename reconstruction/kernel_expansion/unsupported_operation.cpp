#include "unsupported_operation.hpp"

namespace bandwagon::kernel_expansion {

std::uint32_t UnsupportedDeviceOperation(
    UnsupportedOperationDiagnostics* diagnostics) {
  if (diagnostics) diagnostics->DebugBreak();
  return kStatusNotImplemented;
}

}  // namespace bandwagon::kernel_expansion
