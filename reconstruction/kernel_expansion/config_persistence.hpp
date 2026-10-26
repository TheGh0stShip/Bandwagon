#pragma once

#include "config.hpp"

#include <cstdint>
#include <string_view>

namespace bandwagon::kernel_expansion {

class ConfigFileWorkflow {
 public:
  virtual ~ConfigFileWorkflow() = default;

  // This is the still-generic engine at 0x801f3f00. Its mode-dependent file
  // format is intentionally not guessed here; the two recovered callers pass
  // mode zero while establishing defaults and mode one after a field update.
  virtual std::uint8_t Process(std::string_view path, std::uint32_t mode,
                               PeerPressureConfig* config,
                               const void* schema_entries,
                               std::uint32_t entry_count) = 0;
};

// Lift of the fixed-argument adapter at 0x801f3000. Only the low byte of the
// generic engine's result participates in the returned Boolean value.
bool ProcessPeerPressureConfigFile(std::uint32_t mode,
                                   PeerPressureConfig* config,
                                   const void* schema_entries,
                                   ConfigFileWorkflow* workflow);

class ConfigLockServices {
 public:
  virtual ~ConfigLockServices() = default;

  virtual void EnterCriticalRegion() = 0;
  virtual void AcquireExclusive() = 0;
  virtual void Release() = 0;
  virtual void LeaveCriticalRegion() = 0;
};

// Lift of the synchronization wrapper at 0x801f3050. The target discards the
// file workflow result and returns zero after releasing the lock.
bool RunLockedPeerPressureConfigFile(std::uint32_t mode,
                                     PeerPressureConfig* config,
                                     const void* schema_entries,
                                     ConfigFileWorkflow* workflow,
                                     ConfigLockServices* locks);

}  // namespace bandwagon::kernel_expansion
