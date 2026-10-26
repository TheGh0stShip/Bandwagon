#pragma once

#include "config.hpp"
#include "runtime.hpp"

#include <cstdint>
#include <string_view>

namespace bandwagon::kernel_expansion {

inline constexpr std::string_view kPeerPressureConfigPath =
    "\\Device\\Harddisk0\\Partition1\\PeerPressure\\System\\Config.ini";
inline constexpr std::uint32_t kConfigSchemaEntryCount = 6;
inline constexpr std::uint32_t kConfigFatalErrorCode = 0x00012400;

struct ConfigAllocationRequest {
  std::uint32_t size = static_cast<std::uint32_t>(kPeerPressureConfigSize);
  std::uint32_t allocation_type = 0x1000;
  std::uint32_t protection = 4;
  std::uint32_t flags = 2;
};

class ConfigBootstrapServices {
 public:
  virtual ~ConfigBootstrapServices() = default;

  virtual void InitializeStateLock() = 0;
  virtual int AllocateConfig(const ConfigAllocationRequest& request,
                             PeerPressureConfig** config) = 0;
  virtual void DisplayFatalError(std::uint32_t code) = 0;
};

// Readable lift of LoadPeerPressureConfig at 0x801f0648. The first file load
// is an existence/readability probe retained until the workflow completes;
// LoadStructuredConfigFile performs the distinct second load used for parsing.
bool LoadPeerPressureConfig(ConfigurationState* state,
                            ConfigBootstrapServices* bootstrap,
                            FileServices* files,
                            StructuredConfigParser* parser,
                            ConfigServices* config_services,
                            const void* schema_entries);

}  // namespace bandwagon::kernel_expansion
