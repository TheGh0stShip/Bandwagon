#pragma once

#include "config_value.hpp"
#include "runtime.hpp"

#include <cstdint>

namespace bandwagon::kernel_expansion {

using IniValueCallback = void (*)(void* context, const char* section,
                                  const char* key, const char* value);

class ConfigReaderDiagnostics : public ConfigValueDiagnostics {
 public:
  ~ConfigReaderDiagnostics() override = default;

  virtual void ParseFailure() = 0;
};

// Callback-driven read-side INI dispatcher at 0x801f44e0. Section entry and
// exit are both notifications with null key/value; exit uses a null section.
bool ParseIniDocument(char* mutable_text, IniValueCallback callback,
                      void* context);

// Typed, bounds-safe lift of the schema adapter at 0x801f30b0.
bool ParseConfigFileBuffer(
    char* mutable_text, StructuredConfigDestination destination,
    const StructuredConfigEntry* entries, std::uint32_t entry_count,
    ConfigReaderDiagnostics* diagnostics = nullptr);

class ConfigFileBufferParser final : public StructuredConfigParser {
 public:
  explicit ConfigFileBufferParser(ConfigReaderDiagnostics* diagnostics = nullptr)
      : diagnostics_(diagnostics) {}

  bool Parse(std::uint8_t* nul_terminated_text,
             StructuredConfigDestination destination,
             const StructuredConfigEntry* schema_entries,
             std::uint32_t entry_count) override;

 private:
  ConfigReaderDiagnostics* diagnostics_;
};

}  // namespace bandwagon::kernel_expansion
