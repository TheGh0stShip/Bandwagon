#include "config_value.hpp"

#include "config_hash.hpp"

#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>

namespace {

class FakeDiagnostics final
    : public bandwagon::kernel_expansion::ConfigValueDiagnostics {
 public:
  int calls = 0;
  std::string key;
  std::uint32_t hash = 0;

  void UnknownKey(const char* input_key, std::uint32_t input_hash) override {
    ++calls;
    key = input_key;
    hash = input_hash;
  }
};

std::int32_t ReadInteger(const std::uint8_t* data) {
  std::int32_t value = 0;
  std::memcpy(&value, data, sizeof(value));
  return value;
}

float ReadFloat(const std::uint8_t* data) {
  float value = 0;
  std::memcpy(&value, data, sizeof(value));
  return value;
}

bool Near(float left, float right) {
  return std::fabs(left - right) < 0.00001f;
}

}  // namespace

int main() {
  using namespace bandwagon::kernel_expansion;

  std::array<std::uint8_t, 64> storage{};
  const std::array<StructuredConfigEntry, 5> entries{{
      {"System", ComputeConfigKeyHash("Number"), "Number", 0,
       StructuredConfigValueType::kInteger, 0},
      {"System", ComputeConfigKeyHash("Enabled"), "Enabled", 0,
       StructuredConfigValueType::kBoolean, 4},
      {"System", ComputeConfigKeyHash("Name"), "Name", 6,
       StructuredConfigValueType::kString, 8},
      {"Theme", ComputeConfigKeyHash("Color"), "Color", 0,
       StructuredConfigValueType::kColor, 16},
      {"Theme", 0, "RawColor", 0x1111,
       StructuredConfigValueType::kColor, 32},
  }};
  const StructuredConfigDestination destination{storage.data(), storage.size()};
  FakeDiagnostics diagnostics;

  ApplyStructuredConfigValue(destination, entries.data(), entries.size(),
                             "system", "NUMBER", "-42suffix", &diagnostics);
  assert(ReadInteger(storage.data()) == -42);

  ApplyStructuredConfigValue(destination, entries.data(), entries.size(),
                             "SYSTEM", "enabled", "True", &diagnostics);
  assert(storage[4] == 1);
  ApplyStructuredConfigValue(destination, entries.data(), entries.size(),
                             "System", "Enabled", "yes", &diagnostics);
  assert(storage[4] == 0);
  ApplyStructuredConfigValue(destination, entries.data(), entries.size(),
                             "System", "Enabled", "1", &diagnostics);
  assert(storage[4] == 1);

  ApplyStructuredConfigValue(destination, entries.data(), entries.size(),
                             "System", "Name", "\"abc\"", &diagnostics);
  assert(std::strcmp(reinterpret_cast<const char*>(storage.data() + 8),
                     "abc") == 0);
  ApplyStructuredConfigValue(destination, entries.data(), entries.size(),
                             "System", "Name", "abcdef", &diagnostics);
  assert(std::memcmp(storage.data() + 8, "abcde\0", 6) == 0);

  ApplyStructuredConfigValue(destination, entries.data(), entries.size(),
                             "Theme", "Color", "0xFF804000", &diagnostics);
  assert(Near(ReadFloat(storage.data() + 16), 1.0f));
  assert(Near(ReadFloat(storage.data() + 20), 128.0f / 255.0f));
  assert(Near(ReadFloat(storage.data() + 24), 64.0f / 255.0f));
  assert(Near(ReadFloat(storage.data() + 28), 0.0f));

  ApplyStructuredConfigValue(destination, entries.data(), entries.size(),
                             "theme", "rawcolor", "0xff804000", &diagnostics);
  assert(Near(ReadFloat(storage.data() + 32), 255.0f));
  assert(Near(ReadFloat(storage.data() + 36), 128.0f));
  assert(Near(ReadFloat(storage.data() + 40), 64.0f));
  assert(Near(ReadFloat(storage.data() + 44), 0.0f));

  ApplyStructuredConfigValue(destination, entries.data(), entries.size(),
                             "System", "Missing", "1", &diagnostics);
  assert(diagnostics.calls == 1);
  assert(diagnostics.key == "Missing");
  assert(diagnostics.hash == ComputeConfigKeyHash("Missing"));

  const auto unchanged = storage;
  ApplyStructuredConfigValue(destination, entries.data(), entries.size(),
                             "WrongSection", "Number", "9", &diagnostics);
  assert(storage == unchanged && diagnostics.calls == 2);
  ApplyStructuredConfigValue(destination, entries.data(), entries.size(),
                             "System", nullptr, "9", &diagnostics);
  assert(storage == unchanged && diagnostics.calls == 2);

  const StructuredConfigEntry stale_hash{
      "System", ComputeConfigKeyHash("FormerName"), "RenamedValue", 0,
      StructuredConfigValueType::kInteger, 48};
  ApplyStructuredConfigValue(destination, &stale_hash, 1, "System",
                             "RenamedValue", "77", &diagnostics);
  assert(ReadInteger(storage.data() + 48) == 77);

  const StructuredConfigEntry hash_match{
      "System", ComputeConfigKeyHash("HashOnlyName"), "DifferentName", 0,
      StructuredConfigValueType::kInteger, 52};
  ApplyStructuredConfigValue(destination, &hash_match, 1, "System",
                             "HashOnlyName", "88", &diagnostics);
  assert(ReadInteger(storage.data() + 52) == 88);

  std::cout << "Kernel Expansion config value tests passed\n";
}
