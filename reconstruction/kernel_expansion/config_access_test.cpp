#include "config_access.hpp"

#include "config_hash.hpp"
#include "config_schema.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

using bandwagon::kernel_expansion::ConfigFileWorkflow;
using bandwagon::kernel_expansion::ConfigLockServices;
using bandwagon::kernel_expansion::PeerPressureConfig;
using bandwagon::kernel_expansion::StructuredConfigEntry;

class FakeLocks final : public ConfigLockServices {
 public:
  std::vector<std::string>* events = nullptr;

  void EnterCriticalRegion() override { events->emplace_back("enter"); }
  void AcquireShared() override { events->emplace_back("shared"); }
  void AcquireExclusive() override { events->emplace_back("exclusive"); }
  void Release() override { events->emplace_back("release"); }
  void LeaveCriticalRegion() override { events->emplace_back("leave"); }
};

class FakeWorkflow final : public ConfigFileWorkflow {
 public:
  std::vector<std::string>* events = nullptr;
  PeerPressureConfig* expected_config = nullptr;
  std::uint8_t result = 1;
  int calls = 0;

  std::uint8_t Process(
      std::string_view path, std::uint32_t mode, PeerPressureConfig* config,
      const StructuredConfigEntry* entries,
      std::uint32_t entry_count) override {
    using namespace bandwagon::kernel_expansion;
    assert(path == kPeerPressureConfigPath);
    assert(mode == 1);
    assert(config == expected_config);
    assert(entries == kPeerPressureConfigSchema.data());
    assert(entry_count == kPeerPressureConfigSchema.size());
    events->emplace_back("persist");
    ++calls;
    return result;
  }
};

void ExpectEvents(const std::vector<std::string>& actual,
                  std::initializer_list<const char*> expected) {
  assert(actual.size() == expected.size());
  std::size_t index = 0;
  for (const char* event : expected) {
    assert(actual[index] == event);
    ++index;
  }
}

}  // namespace

int main() {
  using namespace bandwagon::kernel_expansion;

  PeerPressureConfig config{};
  config.boot_animation_type = 3;
  std::strcpy(config.boot_animation_theme_name.data(), "Metro");
  config.check_for_updates_on_startup = 1;
  config.eject_button_action = 7;

  std::vector<std::string> events;
  FakeLocks locks;
  locks.events = &events;
  FakeWorkflow workflow;
  workflow.events = &events;
  workflow.expected_config = &config;

  std::uint32_t size = 0;
  assert(GetPeerPressureConfigValueByHash(
             &config, 0x529534db, nullptr, &size, &locks) ==
         kConfigStatusBufferTooSmall);
  assert(size == kBootAnimationThemeNameCapacity);
  ExpectEvents(events, {"enter", "shared", "release", "leave"});

  events.clear();
  std::array<char, kBootAnimationThemeNameCapacity + 4> text{};
  text.fill(static_cast<char>(0x5a));
  size = kBootAnimationThemeNameCapacity - 1;
  assert(GetPeerPressureConfigValueByHash(
             &config, 0x529534db, text.data(), &size, &locks) ==
         kConfigStatusBufferTooSmall);
  assert(size == kBootAnimationThemeNameCapacity && text[0] == 0x5a);

  text.fill(0);
  size = text.size();
  assert(GetPeerPressureConfigValueByHash(
             &config, 0x529534db, text.data(), &size, &locks) ==
         kConfigStatusSuccess);
  assert(size == kBootAnimationThemeNameCapacity);
  assert(std::strcmp(text.data(), "Metro") == 0);
  assert(std::all_of(text.begin() + 6,
                     text.begin() + kBootAnimationThemeNameCapacity,
                     [](char value) { return value == 0; }));

  std::uint32_t integer = 0;
  size = 99;
  assert(GetPeerPressureConfigValueByHash(
             &config, 0x74090f50, nullptr, &size, &locks) ==
         kConfigStatusBufferTooSmall);
  assert(size == 4);

  size = 8;
  assert(GetPeerPressureConfigValueByHash(
             &config, 0x74090f50, &integer, &size, &locks) ==
         kConfigStatusSuccess);
  assert(integer == 3 && size == 8);

  std::uint8_t boolean = 0;
  size = 0;
  assert(GetPeerPressureConfigValueByHash(
             &config, 0x6682f927, nullptr, &size, &locks) ==
         kConfigStatusBufferTooSmall);
  assert(size == 1);

  size = 2;
  assert(GetPeerPressureConfigValueByHash(
             &config, 0x6682f927, &boolean, &size, &locks) ==
         kConfigStatusSuccess);
  assert(boolean == 1 && size == 2);

  integer = 0xaaaaaaaa;
  size = 3;
  assert(GetPeerPressureConfigValueByHash(
             &config, 0x74090f50, &integer, &size, &locks) ==
         kConfigStatusBufferTooSmall);
  assert(integer == 0xaaaaaaaa && size == 4);

  size = sizeof(integer);
  assert(GetPeerPressureConfigValueByHash(
             &config, ComputeConfigKeyHash("EjectButtonAction"), &integer,
             &size, &locks) == kConfigStatusNotFound);
  assert(GetPeerPressureConfigValueByHash(
             &config, ComputeConfigKeyHash("EjectButtonBehavior"), &integer,
             &size, &locks) == kConfigStatusSuccess);
  assert(integer == 7);

  events.clear();
  assert(GetPeerPressureConfigValueByHash(
             &config, 0x12345678, &integer, &size, &locks) ==
         kConfigStatusNotFound);
  ExpectEvents(events, {"enter", "shared", "release", "leave"});

  events.clear();
  assert(GetPeerPressureConfigValueByHash(
             &config, 0x74090f50, &integer, nullptr, &locks) ==
         kConfigStatusInvalidParameter);
  assert(events.empty());
  assert(GetPeerPressureConfigValueByHash(
             &config, 0x74090f50, nullptr, nullptr, &locks) ==
         kConfigStatusInvalidParameter);
  assert(events.empty());

  events.clear();
  const std::int32_t new_action = -4;
  assert(SetPeerPressureConfigValueByHash(
             &config, ComputeConfigKeyHash("EjectButtonBehavior"),
             &new_action, sizeof(new_action), &workflow, &locks) ==
         kConfigStatusSuccess);
  assert(config.eject_button_action == -4 && workflow.calls == 1);
  ExpectEvents(events,
               {"enter", "exclusive", "persist", "release", "leave"});

  events.clear();
  const std::uint8_t disabled = 0;
  assert(SetPeerPressureConfigValueByHash(
             &config, 0x6682f927, &disabled, sizeof(disabled), &workflow,
             &locks) == kConfigStatusSuccess);
  assert(config.check_for_updates_on_startup == 0 && workflow.calls == 2);

  events.clear();
  assert(SetPeerPressureConfigValueByHash(
             &config, 0x6682f927, &disabled, 0, &workflow, &locks) ==
         kConfigStatusBufferTooSmall);
  assert(workflow.calls == 2);
  ExpectEvents(events, {"enter", "exclusive", "release", "leave"});

  events.clear();
  std::array<char, kBootAnimationThemeNameCapacity> exact_text{};
  std::memcpy(exact_text.data(), "Exact", 6);
  assert(SetPeerPressureConfigValueByHash(
             &config, 0x529534db, exact_text.data(), exact_text.size(),
             &workflow, &locks) == kConfigStatusSuccess);
  assert(std::strcmp(config.boot_animation_theme_name.data(), "Exact") == 0);
  assert(workflow.calls == 3);

  events.clear();
  std::array<char, 64> long_text{};
  long_text.fill('x');
  long_text.back() = 0;
  assert(SetPeerPressureConfigValueByHash(
             &config, 0x529534db, long_text.data(), long_text.size(),
             &workflow, &locks) == kConfigStatusSuccess);
  assert(config.boot_animation_theme_name.back() == 0);
  assert(std::strlen(config.boot_animation_theme_name.data()) ==
         kBootAnimationThemeNameCapacity - 1);
  assert(workflow.calls == 4);

  events.clear();
  const auto prior_theme = config.boot_animation_theme_name;
  assert(SetPeerPressureConfigValueByHash(
             &config, 0x529534db, exact_text.data(), exact_text.size() - 1,
             &workflow, &locks) == kConfigStatusBufferTooSmall);
  assert(config.boot_animation_theme_name == prior_theme);
  assert(workflow.calls == 4);
  ExpectEvents(events, {"enter", "exclusive", "release", "leave"});

  events.clear();
  assert(SetPeerPressureConfigValueByHash(
             &config, 0x74090f50, &integer, 3, &workflow, &locks) ==
         kConfigStatusBufferTooSmall);
  assert(workflow.calls == 4);

  events.clear();
  workflow.result = 0;
  const std::uint32_t new_type = 2;
  assert(SetPeerPressureConfigValueByHash(
             &config, 0x74090f50, &new_type, sizeof(new_type), &workflow,
             &locks) == kConfigStatusUnsuccessful);
  assert(config.boot_animation_type == 2);
  assert(workflow.calls == 5);
  ExpectEvents(events,
               {"enter", "exclusive", "persist", "release", "leave"});

  events.clear();
  assert(SetPeerPressureConfigValueByHash(
             &config, ComputeConfigKeyHash("EjectButtonAction"), &new_action,
             sizeof(new_action), &workflow, &locks) == kConfigStatusNotFound);
  assert(workflow.calls == 5);
  ExpectEvents(events, {"enter", "exclusive", "release", "leave"});

  events.clear();
  assert(SetPeerPressureConfigValueByHash(
             &config, 0x74090f50, nullptr, 4, &workflow, &locks) ==
         kConfigStatusInvalidParameter);
  assert(events.empty());

  std::cout << "Kernel Expansion config access tests passed\n";
}
