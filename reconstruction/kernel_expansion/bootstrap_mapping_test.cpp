#include "bootstrap_mapping.hpp"

#include <cassert>
#include <iostream>

namespace {

class FakeMappingServices final
    : public bandwagon::kernel_expansion::BootstrapMappingServices {
 public:
  bool called = false;
  std::uint32_t result = 0x80000001;

  std::uint32_t FreeMappedMemory(
      const bandwagon::kernel_expansion::BootstrapMappingRelease& request)
      override {
    assert(request.region == 0x80170880);
    assert(request.opaque_argument == 0x7a020000);
    assert(request.size == 0x0007c000);
    assert(request.flags == 0);
    called = true;
    return result;
  }
};

}  // namespace

int main() {
  using namespace bandwagon::kernel_expansion;

  FakeMappingServices services;
  assert(ReleaseBootstrapMapping(&services) == services.result);
  assert(services.called);
  assert(ReleaseBootstrapMapping(nullptr) == 0);

  std::cout << "Kernel Expansion bootstrap-mapping tests passed\n";
}
