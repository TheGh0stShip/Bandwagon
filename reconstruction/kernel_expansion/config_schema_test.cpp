#include "config_schema.hpp"

#include "config_hash.hpp"

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string_view>

int main() {
  using namespace bandwagon::kernel_expansion;

  static_assert(sizeof(PeerPressureConfig) == 0x170);
  static_assert(offsetof(PeerPressureConfig, boot_animation_type) == 0x000);
  static_assert(offsetof(PeerPressureConfig, boot_animation_theme_name) ==
                0x004);
  static_assert(offsetof(PeerPressureConfig, dashboard_path) == 0x029);
  static_assert(offsetof(PeerPressureConfig, check_for_updates_on_startup) ==
                0x129);
  static_assert(offsetof(PeerPressureConfig, eject_button_action) == 0x12c);
  static_assert(offsetof(PeerPressureConfig, other_os_name) == 0x130);

  constexpr std::array<std::string_view, kConfigSchemaEntryCount> sections{{
      "BootAnimation", "BootAnimation", "Dashboard",
      "System", "System", "System",
  }};
  constexpr std::array<std::string_view, kConfigSchemaEntryCount> keys{{
      "BootAnimationType", "BootAnimationThemeName", "DashboardPath",
      "CheckForUpdatesOnStartup", "EjectButtonAction", "OtherOSName",
  }};
  constexpr std::array<std::uint32_t, kConfigSchemaEntryCount> hashes{{
      0x74090f50, 0x529534db, 0xa7a2d18e,
      0x6682f927, 0x090a48f5, 0xe6faaef3,
  }};
  constexpr std::array<std::uint16_t, kConfigSchemaEntryCount> options{{
      0, 0x25, 0x100, 0, 0, 0x40,
  }};
  constexpr std::array<StructuredConfigValueType, kConfigSchemaEntryCount>
      types{{
          StructuredConfigValueType::kInteger,
          StructuredConfigValueType::kString,
          StructuredConfigValueType::kString,
          StructuredConfigValueType::kBoolean,
          StructuredConfigValueType::kInteger,
          StructuredConfigValueType::kString,
      }};
  constexpr std::array<std::uint32_t, kConfigSchemaEntryCount> offsets{{
      0x000, 0x004, 0x029, 0x129, 0x12c, 0x130,
  }};

  for (std::size_t index = 0; index < kConfigSchemaEntryCount; ++index) {
    const StructuredConfigEntry& entry = kPeerPressureConfigSchema[index];
    assert(entry.section == sections[index]);
    assert(entry.key == keys[index]);
    assert(entry.key_hash == hashes[index]);
    if (index != 4)
      assert(entry.key_hash == ComputeConfigKeyHash(entry.key));
    assert(entry.options == options[index]);
    assert(entry.type == types[index]);
    assert(entry.destination_offset == offsets[index]);
  }

  assert(ComputeConfigKeyHash("EjectButtonAction") == 0x1fd224bb);
  assert(kPeerPressureConfigSchema[4].key_hash ==
         ComputeConfigKeyHash("EjectButtonBehavior"));

  std::cout << "Kernel Expansion config schema tests passed\n";
}
