#include "config.hpp"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iostream>

namespace {

class FakeConfigServices final
    : public bandwagon::kernel_expansion::ConfigServices {
 public:
  int calls = 0;

  void RunLockedConfigPath(std::uint32_t mode) override {
    assert(mode == 0);
    ++calls;
  }
};

}  // namespace

int main() {
  using namespace bandwagon::kernel_expansion;

  PeerPressureConfig config;
  config.storage.fill(0xa5);
  ConfigurationState state{&config};
  assert(GetPeerPressureConfig(&state) == &config);
  assert(GetPeerPressureConfig(nullptr) == nullptr);

  FakeConfigServices services;
  assert(!InitializeDefaultConfig(&config, &services));
  assert(services.calls == 1);
  assert(config.DefaultEnabledFlag());
  assert(std::count(config.storage.begin(), config.storage.end(), 0) ==
         static_cast<std::ptrdiff_t>(kPeerPressureConfigSize - 1));
  assert(config.storage.front() == 0);
  assert(config.storage.back() == 0);

  assert(!InitializeDefaultConfig(nullptr, &services));
  assert(!InitializeDefaultConfig(&config, nullptr));
  assert(services.calls == 1);

  std::cout << "Kernel Expansion config reconstruction tests passed\n";
}
