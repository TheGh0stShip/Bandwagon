#include "config.hpp"

namespace bandwagon::kernel_expansion {

PeerPressureConfig* GetPeerPressureConfig(ConfigurationState* state) {
  return state ? state->singleton : nullptr;
}

bool InitializeDefaultConfig(PeerPressureConfig* config,
                             ConfigServices* services) {
  if (!config || !services) return false;
  *config = PeerPressureConfig{};
  config->check_for_updates_on_startup = 1;
  services->RunLockedConfigPath(0);
  return false;
}

}  // namespace bandwagon::kernel_expansion
