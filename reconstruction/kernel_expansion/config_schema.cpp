#include "config_schema.hpp"

#include <cstddef>

namespace bandwagon::kernel_expansion {

const std::array<StructuredConfigEntry, kConfigSchemaEntryCount>
    kPeerPressureConfigSchema{{
        {"BootAnimation", 0x74090f50, "BootAnimationType", 0,
         StructuredConfigValueType::kInteger,
         offsetof(PeerPressureConfig, boot_animation_type)},
        {"BootAnimation", 0x529534db, "BootAnimationThemeName",
         kBootAnimationThemeNameCapacity, StructuredConfigValueType::kString,
         offsetof(PeerPressureConfig, boot_animation_theme_name)},
        {"Dashboard", 0xa7a2d18e, "DashboardPath", kDashboardPathCapacity,
         StructuredConfigValueType::kString,
         offsetof(PeerPressureConfig, dashboard_path)},
        {"System", 0x6682f927, "CheckForUpdatesOnStartup", 0,
         StructuredConfigValueType::kBoolean,
         offsetof(PeerPressureConfig, check_for_updates_on_startup)},
        // The shipped cache hashes the former EjectButtonBehavior name. Keep
        // it byte-faithful: ApplyStructuredConfigValue falls back to this
        // record's current key string when the hash does not match.
        {"System", 0x090a48f5, "EjectButtonAction", 0,
         StructuredConfigValueType::kInteger,
         offsetof(PeerPressureConfig, eject_button_action)},
        {"System", 0xe6faaef3, "OtherOSName", kOtherOsNameCapacity,
         StructuredConfigValueType::kString,
         offsetof(PeerPressureConfig, other_os_name)},
    }};

}  // namespace bandwagon::kernel_expansion
