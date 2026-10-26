#include "config_persistence.hpp"
#include "config_schema.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

class FakeWorkflow final
    : public bandwagon::kernel_expansion::ConfigFileWorkflow {
 public:
  std::uint8_t result = 1;
  std::uint32_t expected_mode = 0;
  bandwagon::kernel_expansion::PeerPressureConfig* expected_config = nullptr;
  const bandwagon::kernel_expansion::StructuredConfigEntry* expected_schema =
      nullptr;
  std::vector<std::string>* events = nullptr;
  int calls = 0;

  std::uint8_t Process(
      std::string_view path, std::uint32_t mode,
      bandwagon::kernel_expansion::PeerPressureConfig* config,
      const bandwagon::kernel_expansion::StructuredConfigEntry* schema_entries,
      std::uint32_t entry_count) override {
    using namespace bandwagon::kernel_expansion;
    assert(path == kPeerPressureConfigPath);
    assert(mode == expected_mode);
    assert(config == expected_config);
    assert(schema_entries == expected_schema);
    assert(entry_count == kConfigSchemaEntryCount);
    ++calls;
    if (events) events->emplace_back("process");
    return result;
  }
};

class FakeLocks final
    : public bandwagon::kernel_expansion::ConfigLockServices {
 public:
  std::vector<std::string> events;

  void EnterCriticalRegion() override { events.emplace_back("enter"); }
  void AcquireExclusive() override { events.emplace_back("acquire"); }
  void Release() override { events.emplace_back("release"); }
  void LeaveCriticalRegion() override { events.emplace_back("leave"); }
};

}  // namespace

int main() {
  using namespace bandwagon::kernel_expansion;

  PeerPressureConfig config{};

  FakeWorkflow success;
  success.result = 0x80;
  success.expected_mode = 1;
  success.expected_config = &config;
  success.expected_schema = kPeerPressureConfigSchema.data();
  assert(ProcessPeerPressureConfigFile(1, &config, &success));
  assert(success.calls == 1);

  FakeWorkflow failure;
  failure.result = 0;
  failure.expected_config = &config;
  failure.expected_schema = kPeerPressureConfigSchema.data();
  assert(!ProcessPeerPressureConfigFile(0, &config, &failure));
  assert(failure.calls == 1);

  FakeLocks locks;
  FakeWorkflow locked;
  locked.result = 1;
  locked.expected_config = &config;
  locked.expected_schema = kPeerPressureConfigSchema.data();
  locked.events = &locks.events;
  assert(!RunLockedPeerPressureConfigFile(0, &config, &locked, &locks));
  assert((locks.events == std::vector<std::string>{
      "enter", "acquire", "process", "release", "leave"}));
  assert(locked.calls == 1);

  assert(!ProcessPeerPressureConfigFile(0, nullptr, &success));
  assert(!ProcessPeerPressureConfigFile(0, &config, nullptr));
  assert(!RunLockedPeerPressureConfigFile(
      0, &config, &success, nullptr));

  std::cout << "Kernel Expansion config persistence tests passed\n";
}
