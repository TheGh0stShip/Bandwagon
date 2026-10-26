#include "config_value.hpp"

#include "config_hash.hpp"
#include "decimal.hpp"

#include <algorithm>
#include <cstring>

namespace bandwagon::kernel_expansion {
namespace {

std::uint8_t FoldAsciiCase(std::uint8_t value) {
  if (value >= 'A' && value <= 'Z') return value + ('a' - 'A');
  return value;
}

bool EqualsIgnoringAsciiCase(const char* left, const char* right) {
  if (!left || !right) return false;
  while (*left != '\0' && *right != '\0') {
    if (FoldAsciiCase(static_cast<std::uint8_t>(*left)) !=
        FoldAsciiCase(static_cast<std::uint8_t>(*right))) {
      return false;
    }
    ++left;
    ++right;
  }
  return *left == *right;
}

void StoreWord(std::uint8_t* destination, std::uint32_t value) {
  std::memcpy(destination, &value, sizeof(value));
}

void StoreFloat(std::uint8_t* destination, float value) {
  std::memcpy(destination, &value, sizeof(value));
}

void ApplyString(std::uint8_t* destination, std::size_t available,
                 std::uint16_t configured_capacity, const char* value) {
  if (available == 0) return;
  std::size_t capacity = configured_capacity;
  if (capacity == 0 || capacity > available) capacity = available;

  if (*value == '"') ++value;
  std::size_t copied = 0;
  const std::size_t maximum = capacity - 1;
  while (copied < maximum && value[copied] != '\0') {
    destination[copied] = static_cast<std::uint8_t>(value[copied]);
    ++copied;
  }
  destination[copied] = 0;
  if (copied != 0 && destination[copied - 1] == '"')
    destination[copied - 1] = 0;
}

void ApplyColor(std::uint8_t* destination, std::uint16_t options,
                const char* value) {
  if (value[0] != '0' || value[1] != 'x') return;

  std::uint32_t packed = 0;
  std::uint32_t digits = 0;
  const char* cursor = value + 2;
  while (*cursor != '\0' && digits < 8) {
    const std::uint8_t character =
        FoldAsciiCase(static_cast<std::uint8_t>(*cursor));
    std::uint32_t nibble = 0;
    if (character >= '0' && character <= '9') {
      nibble = character & 0x0f;
    } else if (character >= 'a' && character <= 'f') {
      nibble = (character - 7) & 0x0f;
    } else {
      break;
    }
    if (digits != 0) packed <<= 4;
    packed |= nibble;
    ++digits;
    ++cursor;
  }

  constexpr float kNormalizeByte = 1.0f / 255.0f;
  for (std::uint32_t index = 0; index < 4; ++index) {
    const std::uint32_t shift = 24 - index * 8;
    const std::uint8_t component =
        static_cast<std::uint8_t>(packed >> shift);
    float output = static_cast<float>(component) * kNormalizeByte;
    const std::uint32_t raw_component_bit = 1u << (12 - index * 4);
    if ((options & raw_component_bit) != 0)
      output *= 255.0f;
    StoreFloat(destination + index * sizeof(float), output);
  }
}

}  // namespace

void ApplyStructuredConfigValue(
    StructuredConfigDestination destination,
    const StructuredConfigEntry* entries, std::uint32_t entry_count,
    const char* section, const char* key, const char* value,
    ConfigValueDiagnostics* diagnostics) {
  if (!destination.data || !entries || !section || !key || !value) return;

  const std::uint32_t key_hash = ComputeConfigKeyHash(key);
  const StructuredConfigEntry* match = nullptr;
  for (std::uint32_t index = 0; index < entry_count; ++index) {
    const StructuredConfigEntry& entry = entries[index];
    // A matching cached hash bypasses the key string comparison. A zero or
    // stale hash falls back to the full string, as required by the shipped
    // EjectButtonAction record whose cache still hashes its former name.
    if (entry.key_hash == 0 || entry.key_hash != key_hash) {
      if (!EqualsIgnoringAsciiCase(key, entry.key)) continue;
    }
    if (!EqualsIgnoringAsciiCase(section, entry.section)) continue;
    match = &entry;
    break;
  }

  if (!match) {
    if (diagnostics) diagnostics->UnknownKey(key, key_hash);
    return;
  }
  if (match->destination_offset >= destination.size) return;

  std::uint8_t* output = destination.data + match->destination_offset;
  const std::size_t available = destination.size - match->destination_offset;
  switch (match->type) {
    case StructuredConfigValueType::kString:
      ApplyString(output, available, match->options, value);
      return;
    case StructuredConfigValueType::kInteger:
      if (available >= sizeof(std::uint32_t))
        StoreWord(output, static_cast<std::uint32_t>(ParseSignedDecimal(value)));
      return;
    case StructuredConfigValueType::kBoolean:
      if (available != 0) {
        output[0] = value[0] == 't' || value[0] == 'T' || value[0] == '1';
      }
      return;
    case StructuredConfigValueType::kColor:
      if (available >= 4 * sizeof(float)) ApplyColor(output, match->options, value);
      return;
  }
}

}  // namespace bandwagon::kernel_expansion
