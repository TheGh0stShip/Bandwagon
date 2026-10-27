#include "boot_partition_handoff.hpp"

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

using namespace bandwagon::kernel_expansion;

namespace {

struct BindCall {
  std::u16string partition_name;
  std::string device_path;
  bool required;
};

class FakeServices final : public BootPartitionHandoffServices {
 public:
  bool BindPartition(BootPartitionLoaderState* observed_state,
                     std::u16string_view partition_name,
                     std::string_view device_path, bool required) override {
    assert(observed_state == state);
    calls.push_back(
        {std::u16string(partition_name), std::string(device_path), required});
    events.emplace_back("bind");
    return calls.size() != 1 || first_succeeds;
  }
  void DebugBreak() override {
    events.emplace_back("break");
    ++break_count;
  }
  void FreePhysicalMemory(std::uint32_t memory_type,
                          std::uintptr_t address) override {
    assert(memory_type == 2);
    freed_address = address;
    events.emplace_back("free");
  }

  BootPartitionLoaderState* state = nullptr;
  bool first_succeeds = true;
  int break_count = 0;
  std::uintptr_t freed_address = 0;
  std::vector<BindCall> calls;
  std::vector<std::string> events;
};

void VerifyCalls(const FakeServices& services) {
  assert(services.calls.size() == kBootPartitionBindings.size());
  for (std::size_t i = 0; i < services.calls.size(); ++i) {
    assert(services.calls[i].partition_name ==
           kBootPartitionBindings[i].partition_name);
    assert(services.calls[i].device_path ==
           kBootPartitionBindings[i].device_path);
    assert(services.calls[i].required == (i == 0));
  }
}

}  // namespace

int main() {
  BootPartitionLoaderState state{0x12345000u, 0x9000u, 7};
  FakeServices services;
  services.state = &state;
  HandOffBootPartitions(&state, &services);
  VerifyCalls(services);
  assert(services.break_count == 0);
  assert(services.freed_address == 0x12345000u);
  assert(state.physical_mapping == 0);
  assert(state.mapping_size == 0);
  assert(state.mapping_flags == 0);
  assert(services.events.back() == "free");

  BootPartitionLoaderState failed_state{0xabcdef00u, 1, 2};
  FakeServices failed;
  failed.state = &failed_state;
  failed.first_succeeds = false;
  HandOffBootPartitions(&failed_state, &failed);
  VerifyCalls(failed);
  assert(failed.break_count == 1);
  assert(failed.events[0] == "bind");
  assert(failed.events[1] == "break");
  assert(failed.events.back() == "free");
  assert(failed.freed_address == 0xabcdef00u);

  std::cout << "Kernel Expansion boot partition handoff tests passed\n";
}
