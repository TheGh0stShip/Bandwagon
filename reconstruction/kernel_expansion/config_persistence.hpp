#pragma once

#include "config.hpp"
#include "config_value.hpp"

#include <cstdint>
#include <string_view>

namespace bandwagon::kernel_expansion {

class ConfigFileWorkflow {
 public:
  virtual ~ConfigFileWorkflow() = default;

  // The concrete source lift is StructuredConfigFileWorkflow. Low-byte mode
  // zero regenerates from the schema; mode one preserves and updates an
  // existing INI document.
  virtual std::uint8_t Process(std::string_view path, std::uint32_t mode,
                               PeerPressureConfig* config,
                               const StructuredConfigEntry* schema_entries,
                               std::uint32_t entry_count) = 0;
};

// Lift of the fixed-argument adapter at 0x801f3000. Only the low byte of the
// generic engine's result participates in the returned Boolean value.
bool ProcessPeerPressureConfigFile(std::uint32_t mode,
                                   PeerPressureConfig* config,
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
                                     ConfigFileWorkflow* workflow,
                                     ConfigLockServices* locks);

}  // namespace bandwagon::kernel_expansion
