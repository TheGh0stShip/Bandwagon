#include "config_writer.hpp"

#include "runtime.hpp"

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <cstring>

namespace bandwagon::kernel_expansion {
namespace {

bool EqualsIgnoringAsciiCase(const char* left, const char* right) {
  if (!left || !right) return false;
  while (*left != '\0' && *right != '\0') {
    auto left_byte = static_cast<std::uint8_t>(*left);
    auto right_byte = static_cast<std::uint8_t>(*right);
    if (left_byte >= 'A' && left_byte <= 'Z') left_byte += 'a' - 'A';
    if (right_byte >= 'A' && right_byte <= 'Z') right_byte += 'a' - 'A';
    if (left_byte != right_byte) return false;
    ++left;
    ++right;
  }
  return *left == *right;
}

std::size_t BoundedLength(const char* text, std::size_t maximum) {
  std::size_t length = 0;
  while (length < maximum && text[length] != '\0') ++length;
  return length;
}

bool AppendFormattedConfigOutputV(ConfigWriteContext* context,
                                  const char* format, std::va_list arguments) {
  if (!context || !context->services || !format) return false;

  std::va_list measured_arguments;
  va_copy(measured_arguments, arguments);
  const int measured = std::vsnprintf(nullptr, 0, format, measured_arguments);
  va_end(measured_arguments);
  if (measured < 0 ||
      static_cast<std::size_t>(measured) >= kConfigOutputBufferSize) {
    return false;
  }

  const std::size_t formatted_size = static_cast<std::size_t>(measured);
  if (context->output_size + formatted_size >= kConfigOutputBufferSize &&
      !FlushConfigOutput(context)) {
    return false;
  }

  std::va_list write_arguments;
  va_copy(write_arguments, arguments);
  const int written = std::vsnprintf(
      context->output.data() + context->output_size,
      context->output.size() - context->output_size, format, write_arguments);
  va_end(write_arguments);
  if (written < 0 || static_cast<std::size_t>(written) != formatted_size)
    return false;
  context->output_size += formatted_size;
  return true;
}

bool FormatIniLine(char* destination, std::size_t capacity,
                   std::size_t* length, const char* format, ...) {
  std::va_list arguments;
  va_start(arguments, format);
  const int result = std::vsnprintf(destination, capacity, format, arguments);
  va_end(arguments);
  if (result < 0 || static_cast<std::size_t>(result) >= capacity) return false;
  *length = static_cast<std::size_t>(result);
  return true;
}

}  // namespace

ConfigWriteContext::ConfigWriteContext(
    const std::uint8_t* value_data, std::size_t value_data_size,
    const StructuredConfigEntry* schema_entries,
    std::uint32_t schema_entry_count,
    ConfigFileWriterServices* output_services)
    : values(value_data),
      values_size(value_data_size),
      entries(schema_entries),
      entry_count(schema_entry_count),
      services(output_services),
      emitted(schema_entry_count, 0) {}

bool ParseIniLine(char** cursor, ParsedIniLine* parsed) {
  if (!cursor || !*cursor || !parsed) return false;
  parsed->section = nullptr;
  parsed->key = nullptr;
  parsed->value = nullptr;

  char* scan = *cursor;
  while (*scan == ' ' || *scan == '\t') ++scan;
  const char first = *scan;
  if (first == '\0') return false;

  auto advance_to_next_line = [&](char* position) {
    while (*position != '\0' && *position != '\n') ++position;
    *cursor = *position == '\n' ? position + 1 : nullptr;
  };

  if (first == ';' || first == '\n' || first == '\r') {
    if (first == '\r' && scan[1] != '\n') return false;
    advance_to_next_line(scan);
    return true;
  }

  char* content = scan;
  if (first == '[') {
    parsed->section = scan + 1;
    ++scan;
    while (*scan != ']') {
      if (*scan == '\0' || *scan == '\n') return false;
      ++scan;
    }
    *scan = '\0';
    content = scan + 1;
  } else {
    parsed->key = scan;
    while (IsAsciiAlphaNumeric(static_cast<std::uint8_t>(*scan))) ++scan;
    char* key_end = scan;
    char* equals = scan;
    while (*equals != '=') {
      if (*equals == '\0' || *equals == '\n') return false;
      ++equals;
    }
    *key_end = '\0';
    scan = equals + 1;
    while (*scan == ' ' || *scan == '\t') ++scan;
    if (*scan == '\0') return false;
    parsed->value = scan;
    content = scan;
  }

  char* line_end = content;
  while (*line_end != '\0' && *line_end != '\n') ++line_end;
  if (*line_end == '\0') {
    *cursor = nullptr;
  } else {
    if (line_end != content && line_end[-1] == '\r') {
      line_end[-1] = '\0';
    } else {
      *line_end = '\0';
    }
    *cursor = line_end + 1;
  }
  return true;
}

bool RewriteIniFile(char* contents, IniSectionCallback visit_section,
                    IniSectionCallback finish_section,
                    IniLineCallback visit_line, void* context) {
  if (!contents) return true;
  if (!visit_line) return false;

  std::array<char, 128> current_section{};
  std::array<char, kMaximumIniLineSize> normalized{};
  if (visit_section && !visit_section(context, current_section.data()))
    return false;

  char* cursor = contents;
  while (cursor) {
    char* original = cursor;
    ParsedIniLine parsed{};
    if (!ParseIniLine(&cursor, &parsed)) return *original == '\0';

    std::size_t length = 0;
    if (parsed.section) {
      if (finish_section &&
          !finish_section(context, current_section.data())) {
        return false;
      }
      std::strncpy(current_section.data(), parsed.section,
                   current_section.size());
      current_section.back() = '\0';
      if (!FormatIniLine(normalized.data(), normalized.size(), &length,
                         "[%s]\r\n", current_section.data())) {
        return false;
      }
    } else if (parsed.key && parsed.value) {
      if (!FormatIniLine(normalized.data(), normalized.size(), &length,
                         "%s = %s\r\n", parsed.key, parsed.value)) {
        return false;
      }
    } else {
      // The target assumes raw/comment lines end in LF. A final unterminated
      // comment can make its pointer subtraction wrap; the host lift safely
      // derives the same intended slice from the terminating NUL.
      std::size_t source_length =
          cursor ? static_cast<std::size_t>(cursor - original)
                 : std::strlen(original);
      if (source_length != 0 && original[source_length - 1] == '\n')
        --source_length;
      if (source_length != 0 && original[source_length - 1] == '\r')
        --source_length;
      if (!FormatIniLine(normalized.data(), normalized.size(), &length,
                         "%.*s\r\n", static_cast<int>(source_length),
                         original)) {
        return false;
      }
    }

    if (!visit_line(context, normalized.data(), length,
                    current_section.data(), parsed.key, parsed.value)) {
      return false;
    }
    if (parsed.section && visit_section &&
        !visit_section(context, parsed.section)) {
      return false;
    }
  }

  return !finish_section || finish_section(context, current_section.data());
}

bool FlushConfigOutput(ConfigWriteContext* context) {
  if (!context || !context->services) return false;
  if (context->output_size == 0) return true;
  const bool result = context->services->Write(
      std::string_view(context->output.data(), context->output_size));
  if (result) context->output_size = 0;
  return result;
}

bool AppendLiteralConfigOutput(ConfigWriteContext* context,
                               std::string_view text) {
  if (!context || !context->services ||
      text.size() >= kConfigOutputBufferSize) {
    return false;
  }
  if (context->output_size + text.size() >= kConfigOutputBufferSize &&
      !FlushConfigOutput(context)) {
    return false;
  }
  std::copy(text.begin(), text.end(),
            context->output.begin() + context->output_size);
  context->output_size += text.size();
  return true;
}

bool AppendFormattedConfigOutput(ConfigWriteContext* context,
                                 const char* format, ...) {
  std::va_list arguments;
  va_start(arguments, format);
  const bool result =
      AppendFormattedConfigOutputV(context, format, arguments);
  va_end(arguments);
  return result;
}

bool SerializeConfigEntry(ConfigWriteContext* context,
                          const StructuredConfigEntry* entry) {
  if (!context || !entry || !entry->key) return false;
  if (entry->destination_offset >= context->values_size || !context->values)
    return false;

  const std::uint8_t* value = context->values + entry->destination_offset;
  const std::size_t available =
      context->values_size - entry->destination_offset;
  switch (entry->type) {
    case StructuredConfigValueType::kString: {
      const std::size_t length = BoundedLength(
          reinterpret_cast<const char*>(value), available);
      if (length == available) return false;
      return AppendFormattedConfigOutput(
          context, "%s = \"%s\"\r\n", entry->key,
          reinterpret_cast<const char*>(value));
    }
    case StructuredConfigValueType::kInteger: {
      if (available < sizeof(std::int32_t)) return false;
      std::int32_t integer = 0;
      std::memcpy(&integer, value, sizeof(integer));
      return AppendFormattedConfigOutput(context, "%s = %d\r\n",
                                         entry->key, integer);
    }
    case StructuredConfigValueType::kBoolean:
      return available != 0 && AppendFormattedConfigOutput(
          context, "%s = %s\r\n", entry->key,
          value[0] == 0 ? "false" : "true");
    case StructuredConfigValueType::kColor: {
      if (available < 4 * sizeof(float)) return false;
      std::array<std::uint8_t, 4> components{};
      for (std::size_t index = 0; index < components.size(); ++index) {
        float component = 0.0f;
        std::memcpy(&component, value + index * sizeof(float),
                    sizeof(component));
        const std::uint32_t raw_component_bit =
            1u << (12 - static_cast<std::uint32_t>(index) * 4);
        if ((entry->options & raw_component_bit) != 0)
          component *= 1.0f / 255.0f;
        component *= 255.0f;
        components[index] = static_cast<std::uint8_t>(
            static_cast<std::int64_t>(component));
      }
      return AppendFormattedConfigOutput(
          context, "%s = 0x%02x%02x%02x%02x\r\n", entry->key,
          static_cast<unsigned>(components[0]),
          static_cast<unsigned>(components[1]),
          static_cast<unsigned>(components[2]),
          static_cast<unsigned>(components[3]));
    }
  }
  return true;
}

bool RewriteKnownIniLine(void* opaque_context,
                         const char* normalized_line, std::size_t,
                         const char*, const char* key,
                         const char* value) {
  auto* context = static_cast<ConfigWriteContext*>(opaque_context);
  if (!context || !normalized_line) return false;
  if (key && value) {
    for (std::uint32_t index = 0; index < context->entry_count; ++index) {
      const StructuredConfigEntry& entry = context->entries[index];
      if (!EqualsIgnoringAsciiCase(key, entry.key)) continue;
      if (!SerializeConfigEntry(context, &entry)) return false;
      context->emitted[index] = 1;
      return true;
    }
  }
  return AppendLiteralConfigOutput(context, normalized_line);
}

bool WriteMissingSectionEntries(void* opaque_context, const char* section) {
  auto* context = static_cast<ConfigWriteContext*>(opaque_context);
  if (!context || !section) return false;
  bool wrote_entry = false;
  for (std::uint32_t index = 0; index < context->entry_count; ++index) {
    if (context->emitted[index] != 0) continue;
    const StructuredConfigEntry& entry = context->entries[index];
    if (!EqualsIgnoringAsciiCase(section, entry.section)) continue;
    if (!SerializeConfigEntry(context, &entry)) return false;
    context->emitted[index] = 1;
    wrote_entry = true;
  }
  return !wrote_entry || AppendLiteralConfigOutput(context, "\r\n");
}

bool AcceptIniSection(void*, const char*) { return true; }

bool WriteStructuredConfigFile(
    ConfigFileWriterServices* services, std::string_view path,
    std::uint32_t mode, const std::uint8_t* values, std::size_t values_size,
    const StructuredConfigEntry* entries, std::uint32_t entry_count) {
  if (!services || path.empty() || (!values && values_size != 0) ||
      (!entries && entry_count != 0)) {
    return false;
  }

  std::string existing;
  bool have_existing = false;
  if (static_cast<std::uint8_t>(mode) == 1)
    have_existing = services->ReadExisting(path, &existing);

  ConfigWriteContext context(values, values_size, entries, entry_count,
                             services);
  if (!services->CreateForRewrite(path, path.front() == '\\')) return false;
  bool success = true;

  if (have_existing) {
    // C++17 guarantees a writable trailing NUL for non-const data().
    success = RewriteIniFile(existing.data(), AcceptIniSection,
                             WriteMissingSectionEntries,
                             RewriteKnownIniLine, &context);
  }

  for (std::uint32_t index = 0; success && index < entry_count; ++index) {
    if (context.emitted[index] != 0) continue;
    const StructuredConfigEntry& first = entries[index];
    if (!first.section || !first.key) {
      success = false;
      break;
    }
    success = AppendFormattedConfigOutput(&context, "\r\n[%s]\r\n",
                                          first.section) &&
              SerializeConfigEntry(&context, &first);
    if (!success) break;
    context.emitted[index] = 1;

    for (std::uint32_t candidate = index + 1;
         success && candidate < entry_count; ++candidate) {
      if (context.emitted[candidate] != 0 ||
          !EqualsIgnoringAsciiCase(first.section,
                                   entries[candidate].section)) {
        continue;
      }
      success = SerializeConfigEntry(&context, &entries[candidate]);
      if (success) context.emitted[candidate] = 1;
    }
  }

  if (success) success = FlushConfigOutput(&context);
  services->Close();
  return success;
}

std::uint8_t StructuredConfigFileWorkflow::Process(
    std::string_view path, std::uint32_t mode, PeerPressureConfig* config,
    const StructuredConfigEntry* schema_entries,
    std::uint32_t entry_count) {
  if (!config) return 0;
  return WriteStructuredConfigFile(
             services_, path, mode, config->Data(), config->Size(),
             schema_entries, entry_count)
             ? 1
             : 0;
}

}  // namespace bandwagon::kernel_expansion
