#include "sata_udma.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace {

using namespace bandwagon::kernel_expansion;

class FakeSataUdmaServices final : public SataUdmaServices {
 public:
  std::uint8_t capabilities = 0;
  std::uint32_t first_identify_status = 0;
  std::uint32_t second_identify_status = 0;
  std::uint32_t set_status = 0;
  int identify_calls = 0;
  int set_calls = 0;
  int stall_calls = 0;
  std::uint8_t requested_mode = 0;
  std::vector<std::string> events;

  std::uint32_t IdentifyDevice(std::uintptr_t channel,
                               std::uint32_t output_size,
                               std::uint8_t* output) override {
    assert(channel == kSataChannelAddress);
    assert(output_size == kSataIdentifyDataSize);
    assert(output != nullptr);
    events.emplace_back("identify");
    ++identify_calls;
    if (identify_calls == 1) {
      output[kSataUltraDmaCapabilityOffset] = capabilities;
      return first_identify_status;
    }
    return second_identify_status;
  }

  std::uint32_t SetTransferMode(std::uintptr_t channel,
                                std::uint8_t transfer_mode) override {
    assert(channel == kSataChannelAddress);
    events.emplace_back("set");
    ++set_calls;
    requested_mode = transfer_mode;
    return set_status;
  }

  void StallExecutionProcessor(std::uint32_t microseconds) override {
    assert(microseconds == kSataTransferModeSettleMicroseconds);
    events.emplace_back("stall");
    ++stall_calls;
  }
};

void ExpectNoModeChange(std::uint8_t capabilities) {
  FakeSataUdmaServices services;
  services.capabilities = capabilities;
  ConfigureSataUltraDmaMode(&services);
  assert(services.identify_calls == 1);
  assert(services.set_calls == 0 && services.stall_calls == 0);
  assert((services.events == std::vector<std::string>{"identify"}));
}

void ExpectMode(std::uint8_t capabilities, std::uint8_t expected_mode) {
  FakeSataUdmaServices services;
  services.capabilities = capabilities;
  services.second_identify_status = 0xc0000001u;
  ConfigureSataUltraDmaMode(&services);
  assert(services.identify_calls == 2);
  assert(services.set_calls == 1 && services.stall_calls == 1);
  assert(services.requested_mode == expected_mode);
  assert((services.events ==
          std::vector<std::string>{"identify", "set", "stall", "identify"}));
}

}  // namespace

int main() {
  using namespace bandwagon::kernel_expansion;

  ConfigureSataUltraDmaMode(nullptr);

  FakeSataUdmaServices identify_failure;
  identify_failure.first_identify_status = 1;
  identify_failure.capabilities = 0xff;
  ConfigureSataUltraDmaMode(&identify_failure);
  assert(identify_failure.identify_calls == 1);
  assert(identify_failure.set_calls == 0);

  FakeSataUdmaServices negative_identify_failure;
  negative_identify_failure.first_identify_status = 0xc0000001u;
  negative_identify_failure.capabilities = 0xff;
  ConfigureSataUltraDmaMode(&negative_identify_failure);
  assert(negative_identify_failure.identify_calls == 1);
  assert(negative_identify_failure.set_calls == 0);

  ExpectNoModeChange(0x00);
  ExpectNoModeChange(0x01);
  ExpectNoModeChange(0x03);
  ExpectMode(0x04, 0x42);
  ExpectMode(0x29, 0x45);
  ExpectMode(0x40, 0x46);
  ExpectMode(0x80, 0x46);

  FakeSataUdmaServices set_failure;
  set_failure.capabilities = 0x20;
  set_failure.set_status = 0x40000001u;
  ConfigureSataUltraDmaMode(&set_failure);
  assert(set_failure.identify_calls == 1);
  assert(set_failure.set_calls == 1 && set_failure.requested_mode == 0x45);
  assert(set_failure.stall_calls == 0);
  assert((set_failure.events ==
          std::vector<std::string>{"identify", "set"}));

  std::cout << "Kernel Expansion SATA UDMA tests passed\n";
}
