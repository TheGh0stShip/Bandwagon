#include "config_reader.hpp"

#include "config_writer.hpp"

#include <array>
#include <cstring>

namespace bandwagon::kernel_expansion {
namespace {

struct ConfigReadContext {
  StructuredConfigDestination destination;
  const StructuredConfigEntry* entries;
  std::uint32_t entry_count;
  ConfigValueDiagnostics* diagnostics;
};

void ApplyIniValue(void* opaque_context, const char* section,
                   const char* key, const char* value) {
  auto* context = static_cast<ConfigReadContext*>(opaque_context);
  ApplyStructuredConfigValue(
      context->destination, context->entries, context->entry_count,
      section, key, value, context->diagnostics);
}

}  // namespace

bool ParseIniDocument(char* mutable_text, IniValueCallback callback,
                      void* context) {
  if (!mutable_text || !callback) return false;

  std::array<char, 128> current_section{};
  char* cursor = mutable_text;
  while (cursor) {
    ParsedIniLine parsed{};
    if (!ParseIniLine(&cursor, &parsed)) {
      if (*cursor != '\0') return false;
      break;
    }

    if (parsed.section) {
      if (current_section[0] != '\0')
        callback(context, nullptr, nullptr, nullptr);
      std::strncpy(current_section.data(), parsed.section,
                   current_section.size());
      current_section.back() = '\0';
      callback(context, current_section.data(), nullptr, nullptr);
    } else if (parsed.key && parsed.value) {
      callback(context, current_section.data(), parsed.key, parsed.value);
    }
  }

  if (current_section[0] != '\0')
    callback(context, nullptr, nullptr, nullptr);
  return true;
}

bool ParseConfigFileBuffer(
    char* mutable_text, StructuredConfigDestination destination,
    const StructuredConfigEntry* entries, std::uint32_t entry_count,
    ConfigReaderDiagnostics* diagnostics) {
  if (!mutable_text || !destination.data || destination.size == 0 ||
      (!entries && entry_count != 0)) {
    return false;
  }
  ConfigReadContext context{destination, entries, entry_count, diagnostics};
  const bool parsed = ParseIniDocument(mutable_text, ApplyIniValue, &context);
  if (!parsed && diagnostics) diagnostics->ParseFailure();
  return parsed;
}

bool ConfigFileBufferParser::Parse(
    std::uint8_t* nul_terminated_text,
    StructuredConfigDestination destination,
    const StructuredConfigEntry* schema_entries,
    std::uint32_t entry_count) {
  return ParseConfigFileBuffer(
      reinterpret_cast<char*>(nul_terminated_text), destination,
      schema_entries, entry_count, diagnostics_);
}

}  // namespace bandwagon::kernel_expansion
