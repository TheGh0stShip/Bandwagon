#pragma once

#include "config_persistence.hpp"
#include "config_value.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace bandwagon::kernel_expansion {

inline constexpr std::size_t kConfigOutputBufferSize = 0x400;
inline constexpr std::size_t kMaximumIniLineSize = 0x200;

// Host boundary for the target's read/create/write/close sequence. ReadExisting
// is advisory: mode one continues with an empty input when the old file cannot
// be read, matching the recovered routine.
class ConfigFileWriterServices {
 public:
  virtual ~ConfigFileWriterServices() = default;

  virtual bool ReadExisting(std::string_view path, std::string* contents) = 0;
  virtual bool CreateForRewrite(std::string_view path,
                                bool object_manager_path) = 0;
  virtual bool Write(std::string_view bytes) = 0;
  virtual void Close() = 0;
};

struct ParsedIniLine {
  char* section = nullptr;
  char* key = nullptr;
  char* value = nullptr;
};

// Destructive INI-line tokenizer lifted from 0x801f7938. The input cursor is
// advanced to the next line, or set to null after a final unterminated line.
// Returned section/key/value pointers refer into the mutated input buffer.
bool ParseIniLine(char** cursor, ParsedIniLine* parsed);

using IniSectionCallback = bool (*)(void* context, const char* section);
using IniLineCallback = bool (*)(void* context, const char* normalized_line,
                                 std::size_t length,
                                 const char* current_section,
                                 const char* key, const char* value);

// Generic callback-driven INI walker at 0x801f73c8. It normalizes emitted
// lines to CRLF and rejects any formatted line that reaches 512 bytes.
bool RewriteIniFile(char* contents, IniSectionCallback visit_section,
                    IniSectionCallback finish_section,
                    IniLineCallback visit_line, void* context);

struct ConfigWriteContext {
  ConfigWriteContext(const std::uint8_t* value_data,
                     std::size_t value_data_size,
                     const StructuredConfigEntry* schema_entries,
                     std::uint32_t schema_entry_count,
                     ConfigFileWriterServices* output_services);

  const std::uint8_t* values;
  std::size_t values_size;
  const StructuredConfigEntry* entries;
  std::uint32_t entry_count;
  ConfigFileWriterServices* services;
  std::array<char, kConfigOutputBufferSize> output{};
  std::size_t output_size = 0;
  std::vector<std::uint8_t> emitted;
};

// Buffered formatter at 0x801f7608 and its safe literal companion. The target
// accidentally treated preserved INI text as a printf format string; the host
// lift keeps '%' bytes literal while retaining the normal binary behavior.
bool AppendFormattedConfigOutput(ConfigWriteContext* context,
                                 const char* format, ...);
bool AppendLiteralConfigOutput(ConfigWriteContext* context,
                               std::string_view text);
bool FlushConfigOutput(ConfigWriteContext* context);

// Typed schema serializer at 0x801f7730.
bool SerializeConfigEntry(ConfigWriteContext* context,
                          const StructuredConfigEntry* entry);

// INI callbacks recovered at 0x801f7228, 0x801f72f0, and 0x801f73c0.
bool RewriteKnownIniLine(void* context, const char* normalized_line,
                         std::size_t length,
                         const char* current_section, const char* key,
                         const char* value);
bool WriteMissingSectionEntries(void* context, const char* section);
bool AcceptIniSection(void* context, const char* section);

// Generic config-file transaction at 0x801f3f00. Mode one reads and merges the
// existing file; all other low-byte modes generate from the schema alone.
bool WriteStructuredConfigFile(
    ConfigFileWriterServices* services, std::string_view path,
    std::uint32_t mode, const std::uint8_t* values, std::size_t values_size,
    const StructuredConfigEntry* entries, std::uint32_t entry_count);

// Concrete bridge for the already recovered fixed-path persistence adapter.
class StructuredConfigFileWorkflow final : public ConfigFileWorkflow {
 public:
  explicit StructuredConfigFileWorkflow(ConfigFileWriterServices* services)
      : services_(services) {}

  std::uint8_t Process(std::string_view path, std::uint32_t mode,
                       PeerPressureConfig* config,
                       const StructuredConfigEntry* schema_entries,
                       std::uint32_t entry_count) override;

 private:
  ConfigFileWriterServices* services_;
};

}  // namespace bandwagon::kernel_expansion
