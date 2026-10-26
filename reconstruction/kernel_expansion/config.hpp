#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace bandwagon::kernel_expansion {

inline constexpr std::size_t kPeerPressureConfigSize = 0x170;
inline constexpr std::size_t kBootAnimationThemeNameCapacity = 0x25;
inline constexpr std::size_t kDashboardPathCapacity = 0x100;
inline constexpr std::size_t kOtherOsNameCapacity = 0x40;
inline constexpr std::size_t kCheckForUpdatesOnStartupOffset = 0x129;
inline constexpr std::string_view kPeerPressureConfigPath =
    "\\Device\\Harddisk0\\Partition1\\PeerPressure\\System\\Config.ini";
inline constexpr std::uint32_t kConfigSchemaEntryCount = 6;

struct PeerPressureConfig {
  std::uint32_t boot_animation_type = 0;                         // +0x000
  std::array<char, kBootAnimationThemeNameCapacity>
      boot_animation_theme_name{};                              // +0x004
  std::array<char, kDashboardPathCapacity> dashboard_path{};    // +0x029
  std::uint8_t check_for_updates_on_startup = 0;                 // +0x129
  std::array<std::uint8_t, 2> reserved_12a{};                    // +0x12a
  std::int32_t eject_button_action = 0;                          // +0x12c
  std::array<char, kOtherOsNameCapacity> other_os_name{};        // +0x130

  std::uint8_t* Data() {
    return reinterpret_cast<std::uint8_t*>(this);
  }
  const std::uint8_t* Data() const {
    return reinterpret_cast<const std::uint8_t*>(this);
  }
  static constexpr std::size_t Size() { return kPeerPressureConfigSize; }

  bool CheckForUpdatesOnStartup() const {
    return check_for_updates_on_startup != 0;
  }
};

static_assert(sizeof(PeerPressureConfig) == kPeerPressureConfigSize);
static_assert(offsetof(PeerPressureConfig, boot_animation_type) == 0x000);
static_assert(offsetof(PeerPressureConfig, boot_animation_theme_name) == 0x004);
static_assert(offsetof(PeerPressureConfig, dashboard_path) == 0x029);
static_assert(offsetof(PeerPressureConfig, check_for_updates_on_startup) ==
              kCheckForUpdatesOnStartupOffset);
static_assert(offsetof(PeerPressureConfig, eject_button_action) == 0x12c);
static_assert(offsetof(PeerPressureConfig, other_os_name) == 0x130);

struct ConfigurationState {
  PeerPressureConfig* singleton = nullptr;
};

class ConfigServices {
 public:
  virtual ~ConfigServices() = default;

  // The callee at 0x801f3050 acquires the recovered global read/write lock,
  // enters the common config path with argument zero, releases the lock, and
  // returns zero. Its deeper read/write policy is still under classification.
  virtual void RunLockedConfigPath(std::uint32_t mode) = 0;
};

// Lift of the singleton getter at 0x801f09c0.
PeerPressureConfig* GetPeerPressureConfig(ConfigurationState* state);

// Lift of InitializeDefaultConfig at 0x801f16a8. The observed binary return is
// always false even though it initializes the singleton and runs the locked
// config path; callers rely on that exact result.
bool InitializeDefaultConfig(PeerPressureConfig* config,
                             ConfigServices* services);

}  // namespace bandwagon::kernel_expansion
