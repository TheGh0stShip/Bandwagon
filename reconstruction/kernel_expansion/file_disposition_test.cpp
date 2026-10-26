#include "file_disposition.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>

namespace {

class FakeDiagnostics final
    : public bandwagon::kernel_expansion::FileDispositionDiagnostics {
 public:
  int breaks = 0;
  void DebugBreak() override { ++breaks; }
};

}  // namespace

int main() {
  using namespace bandwagon::kernel_expansion;

  FakeDiagnostics diagnostics;
  assert(MapCreateDispositionToOpenFlags(0, &diagnostics) == 0x08);
  assert(MapCreateDispositionToOpenFlags(1, &diagnostics) == 0x00);
  assert(MapCreateDispositionToOpenFlags(2, &diagnostics) == 0x04);
  assert(MapCreateDispositionToOpenFlags(3, &diagnostics) == 0x10);
  assert(MapCreateDispositionToOpenFlags(4, &diagnostics) == 0x00);
  assert(MapCreateDispositionToOpenFlags(5, &diagnostics) == 0x08);
  assert(diagnostics.breaks == 0);

  assert(MapCreateDispositionToOpenFlags(6, &diagnostics) == 0);
  assert(MapCreateDispositionToOpenFlags(0xffffffffu, &diagnostics) == 0);
  assert(diagnostics.breaks == 2);
  assert(MapCreateDispositionToOpenFlags(99, nullptr) == 0);

  std::cout << "Kernel Expansion file disposition tests passed\n";
}
