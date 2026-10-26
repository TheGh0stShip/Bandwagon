#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace bandwagon::kernel_expansion {

inline constexpr std::size_t kPeerPressureConfigSize = 0x170;
inline constexpr std::size_t kDefaultEnabledFlagOffset = 0x129;

// Only fields established by direct accesses are named. The remaining bytes
// stay explicit, indexed configuration storage until their schema entries are
// individually recovered.
struct PeerPressureConfig {
  std::array<std::uint8_t, kPeerPressureConfigSize> storage{};

  bool DefaultEnabledFlag() const {
    return storage[kDefaultEnabledFlagOffset] != 0;
  }
};

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
