#pragma once

#include <cstddef>
#include <cstdint>

namespace bandwagon::kernel_expansion {

enum class StructuredConfigValueType : std::uint16_t {
  kString = 0,
  kInteger = 1,
  kBoolean = 2,
  kColor = 3,
};

// Logical form of the target's 20-byte schema record. On the 32-bit target the
// three pointer/word fields, two halfwords, and destination offset total 0x14.
struct StructuredConfigEntry {
  const char* section = nullptr;       // target offset 0x00
  std::uint32_t key_hash = 0;          // target offset 0x04
  const char* key = nullptr;           // target offset 0x08
  std::uint16_t options = 0;           // target offset 0x0c
  StructuredConfigValueType type{};    // target offset 0x0e
  std::uint32_t destination_offset = 0; // target offset 0x10
};

struct StructuredConfigDestination {
  std::uint8_t* data = nullptr;
  std::size_t size = 0;
};

class ConfigValueDiagnostics {
 public:
  virtual ~ConfigValueDiagnostics() = default;
  virtual void UnknownKey(const char* key, std::uint32_t key_hash) = 0;
};

// Lift of the typed schema dispatcher at 0x801f41e8. Host bounds are explicit;
// the target relies on trusted schema offsets and capacities.
void ApplyStructuredConfigValue(
    StructuredConfigDestination destination,
    const StructuredConfigEntry* entries, std::uint32_t entry_count,
    const char* section, const char* key, const char* value,
    ConfigValueDiagnostics* diagnostics);

}  // namespace bandwagon::kernel_expansion
