#include "unsupported_operation.hpp"

#include <cassert>
#include <iostream>

namespace {

class FakeDiagnostics final
    : public bandwagon::kernel_expansion::UnsupportedOperationDiagnostics {
 public:
  int breaks = 0;
  void DebugBreak() override { ++breaks; }
};

}  // namespace

int main() {
  using namespace bandwagon::kernel_expansion;

  FakeDiagnostics diagnostics;
  assert(UnsupportedDeviceOperation(&diagnostics) == 0xc0000002u);
  assert(diagnostics.breaks == 1);
  assert(UnsupportedDeviceOperation(nullptr) == kStatusNotImplemented);

  std::cout << "Kernel Expansion unsupported operation tests passed\n";
}
