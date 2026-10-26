#include "config.hpp"

namespace bandwagon::kernel_expansion {

PeerPressureConfig* GetPeerPressureConfig(ConfigurationState* state) {
  return state ? state->singleton : nullptr;
}

bool InitializeDefaultConfig(PeerPressureConfig* config,
                             ConfigServices* services) {
  if (!config || !services) return false;
  config->storage.fill(0);
  config->storage[kDefaultEnabledFlagOffset] = 1;
  services->RunLockedConfigPath(0);
  return false;
}

}  // namespace bandwagon::kernel_expansion
