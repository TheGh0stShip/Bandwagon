#include "xconfig_hooks.hpp"

#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>

namespace {

using namespace bandwagon::kernel_expansion;

class FakeXConfigHookServices final : public XConfigHookServices {
 public:
  std::uint32_t status = 0;
  std::uint32_t calls = 0;
  std::uint32_t expected_category = 0;
  std::uint32_t expected_setting = 0;
  void* expected_output = nullptr;
  std::uint32_t expected_output_size = 0;
  std::uint32_t* expected_required_size = nullptr;

  std::uint32_t GetXConfigSetting(
      std::uint32_t category, std::uint32_t setting, void* output,
      std::uint32_t output_size, std::uint32_t* required_size) override {
    ++calls;
    assert(category == expected_category);
    assert(setting == expected_setting);
    assert(output == expected_output);
    assert(output_size == expected_output_size);
    assert(required_size == expected_required_size);
    return status;
  }
};

std::uint32_t ReadWord(const std::uint8_t* bytes) {
  std::uint32_t value = 0;
  std::memcpy(&value, bytes, sizeof(value));
  return value;
}

}  // namespace

int main() {
  using namespace bandwagon::kernel_expansion;

  std::array<std::uint8_t, 12> storage;
  storage.fill(0xa5);
  std::uint32_t required_size = 0x12345678u;
  FakeXConfigHookServices services;
  services.expected_category = 2;
  services.expected_setting = 4;
  services.expected_output = storage.data() + 4;
  services.expected_output_size = 4;
  services.expected_required_size = &required_size;

  assert(ForceDvdRegionFreeHook(2, 4, storage.data() + 4, 4,
                                &required_size, &services) == 0);
  assert(services.calls == 1);
  assert(ReadWord(storage.data() + 4) == kRegionFreeDvdRegion);
  for (std::size_t index : {0u, 1u, 2u, 3u, 8u, 9u, 10u, 11u}) {
    assert(storage[index] == 0xa5);
  }

  storage.fill(0x5a);
  services.expected_category = 0xabcd0002u;
  services.expected_setting = 0x12340004u;
  assert(ForceDvdRegionFreeHook(0xabcd0002u, 0x12340004u,
                                storage.data() + 4, 4, &required_size,
                                &services) == 0);
  assert(ReadWord(storage.data() + 4) == kRegionFreeDvdRegion);

  for (const auto keys :
       {std::array<std::uint32_t, 2>{3, 4},
        std::array<std::uint32_t, 2>{2, 5},
        std::array<std::uint32_t, 2>{0x00010003u, 0x00010004u}}) {
    storage.fill(0x3c);
    services.expected_category = keys[0];
    services.expected_setting = keys[1];
    assert(ForceDvdRegionFreeHook(keys[0], keys[1], storage.data() + 4, 4,
                                  &required_size, &services) == 0);
    for (std::uint8_t byte : storage) assert(byte == 0x3c);
  }

  storage.fill(0x77);
  services.expected_category = 2;
  services.expected_setting = 4;
  services.status = 0xc0000001u;
  assert(ForceDvdRegionFreeHook(2, 4, storage.data() + 4, 4,
                                &required_size, &services) ==
         0xc0000001u);
  for (std::uint8_t byte : storage) assert(byte == 0x77);

  services.status = 0;
  services.expected_output = nullptr;
  assert(ForceDvdRegionFreeHook(2, 4, nullptr, 4, &required_size,
                                &services) == 0);
  assert(ForceDvdRegionFreeHook(2, 4, storage.data(), 4, &required_size,
                                nullptr) ==
         kXConfigInvalidParameterStatus);

  std::cout << "Kernel Expansion XConfig hook tests passed\n";
}
