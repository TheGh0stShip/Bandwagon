#pragma once

#include "config.hpp"

#include <array>
#include <cstdint>
#include <functional>
#include <string_view>

namespace bandwagon::kernel_expansion {

inline constexpr std::uint32_t kBootAnimationThemeVersion = 9199;
inline constexpr std::uint32_t kBootAnimationThemeSchemaEntryCount = 41;
inline constexpr std::size_t kBootAnimationThemePayloadSize = 0x278;
inline constexpr std::string_view kBootAnimationThemePathPrefix =
    "\\Device\\Harddisk0\\Partition1\\PeerPressure\\Themes\\BootAnim\\9199\\";
inline constexpr std::string_view kBootAnimationThemeExtension = ".xbat";

struct BootAnimationThemePayload {
  std::uint32_t version = 0;
  std::array<std::uint8_t, kBootAnimationThemePayloadSize - sizeof(version)>
      data{};
};

static_assert(sizeof(BootAnimationThemePayload) ==
              kBootAnimationThemePayloadSize);

using BootAnimationThemeInitializer =
    std::function<void(BootAnimationThemePayload*)>;
using BootAnimationThemeDecoder = std::function<bool(
    std::string_view path, BootAnimationThemePayload* destination,
    std::uint32_t schema_entry_count)>;

// Exact policy wrapper at 0x801f1fc0. The target initializes a local payload,
// builds the fixed version-9199 theme path with a 37-byte name precision,
// decodes with the 41-entry schema, validates version 9199, and publishes all
// 632 bytes only after both checks succeed.
bool LoadBootAnimationTheme(
    const PeerPressureConfig& config, BootAnimationThemePayload* active_theme,
    const BootAnimationThemeInitializer& initialize,
    const BootAnimationThemeDecoder& decode);

}  // namespace bandwagon::kernel_expansion
