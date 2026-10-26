#include "config_persistence.hpp"

#include "config_schema.hpp"

namespace bandwagon::kernel_expansion {

bool ProcessPeerPressureConfigFile(std::uint32_t mode,
                                   PeerPressureConfig* config,
                                   ConfigFileWorkflow* workflow) {
  if (!config || !workflow) return false;
  return workflow->Process(kPeerPressureConfigPath, mode, config,
                           kPeerPressureConfigSchema.data(),
                           kPeerPressureConfigSchema.size()) != 0;
}

bool RunLockedPeerPressureConfigFile(std::uint32_t mode,
                                     PeerPressureConfig* config,
                                     ConfigFileWorkflow* workflow,
                                     ConfigLockServices* locks) {
  if (!config || !workflow || !locks) return false;

  locks->EnterCriticalRegion();
  locks->AcquireExclusive();
  ProcessPeerPressureConfigFile(mode, config, workflow);
  locks->Release();
  locks->LeaveCriticalRegion();
  return false;
}

}  // namespace bandwagon::kernel_expansion
