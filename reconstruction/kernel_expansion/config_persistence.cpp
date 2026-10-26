#include "config_persistence.hpp"

namespace bandwagon::kernel_expansion {

bool ProcessPeerPressureConfigFile(std::uint32_t mode,
                                   PeerPressureConfig* config,
                                   const StructuredConfigEntry* schema_entries,
                                   ConfigFileWorkflow* workflow) {
  if (!config || !workflow) return false;
  return workflow->Process(kPeerPressureConfigPath, mode, config,
                           schema_entries, kConfigSchemaEntryCount) != 0;
}

bool RunLockedPeerPressureConfigFile(std::uint32_t mode,
                                     PeerPressureConfig* config,
                                     const StructuredConfigEntry* schema_entries,
                                     ConfigFileWorkflow* workflow,
                                     ConfigLockServices* locks) {
  if (!config || !workflow || !locks) return false;

  locks->EnterCriticalRegion();
  locks->AcquireExclusive();
  ProcessPeerPressureConfigFile(mode, config, schema_entries, workflow);
  locks->Release();
  locks->LeaveCriticalRegion();
  return false;
}

}  // namespace bandwagon::kernel_expansion
