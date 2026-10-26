#include "xex_load_hooks.hpp"

#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>

namespace {

using namespace bandwagon::kernel_expansion;

class FakeXexLoadHookServices final : public XexLoadHookServices {
 public:
  const char* expected_path = nullptr;
  std::uint32_t expected_flags = 0;
  std::uintptr_t expected_argument3 = 0;
  std::uintptr_t expected_argument4 = 0;
  std::uint32_t result = 0;
  int calls = 0;

  std::uint32_t ForwardXexLoad(const char* path, std::uint32_t load_flags,
                               std::uintptr_t argument3,
                               std::uintptr_t argument4) override {
    ++calls;
    assert(path == expected_path);
    assert(load_flags == expected_flags);
    assert(argument3 == expected_argument3);
    assert(argument4 == expected_argument4);
    return result;
  }
};

}  // namespace

int main() {
  using namespace bandwagon::kernel_expansion;

  assert(IsBlockedXexLoadPath("launch.xex", 8));
  assert(IsBlockedXexLoadPath("LAUNCH.XEX", 8));
  assert(IsBlockedXexLoadPath("Hdd:\\Apps\\launch.xex", 8));
  assert(!IsBlockedXexLoadPath("aunch.xex", 8));
  assert(!IsBlockedXexLoadPath("launch.xex.bak", 8));
  for (const std::uint32_t flags : {0u, 9u, 0x10008u}) {
    assert(!IsBlockedXexLoadPath("launch.xex", flags));
  }

  for (const std::uint32_t flags : {0u, 8u, 9u, 0x10008u}) {
    assert(IsBlockedXexLoadPath("Plugins\\Nova.xex", flags));
    assert(IsBlockedXexLoadPath("Hdd:\\PLUGINS\\NOVA.XEX", flags));
  }
  assert(!IsBlockedXexLoadPath("lugins\\Nova.xex", 0));
  assert(!IsBlockedXexLoadPath("Plugins\\Nova.xex.extra", 0));
  assert(!IsBlockedXexLoadPath("default.xex", 8));
  assert(!IsBlockedXexLoadPath(nullptr, 8));

  FakeXexLoadHookServices services;
  constexpr std::uintptr_t kArgument3 = 0x123456789abcdef0ull;
  constexpr std::uintptr_t kArgument4 = 0xfedcba9876543210ull;
  const char* const allowed_path = "Hdd:\\default.xex";
  services.expected_path = allowed_path;
  services.expected_flags = 0x10008u;
  services.expected_argument3 = kArgument3;
  services.expected_argument4 = kArgument4;
  services.result = 0x81234567u;
  assert(FilterBlockedXexLoadPaths(allowed_path, 0x10008u, kArgument3,
                                   kArgument4, &services) ==
         services.result);
  assert(services.calls == 1);

  assert(FilterBlockedXexLoadPaths("launch.xex", 8, kArgument3, kArgument4,
                                   &services) ==
         kStatusObjectNameNotFound);
  assert(FilterBlockedXexLoadPaths("Plugins\\Nova.xex", 0, kArgument3,
                                   kArgument4, &services) ==
         kStatusObjectNameNotFound);
  assert(services.calls == 1);

  services.expected_path = nullptr;
  services.expected_flags = 8;
  services.result = 0;
  assert(FilterBlockedXexLoadPaths(nullptr, 8, kArgument3, kArgument4,
                                   &services) == 0);
  assert(services.calls == 2);
  assert(FilterBlockedXexLoadPaths("default.xex", 0, 0, 0, nullptr) ==
         kStatusObjectNameNotFound);

  std::cout << "Kernel Expansion XEX-load hook tests passed\n";
}
