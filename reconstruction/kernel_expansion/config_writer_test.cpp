#include "config_writer.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

class FakeWriterServices final
    : public bandwagon::kernel_expansion::ConfigFileWriterServices {
 public:
  std::string existing;
  std::string output;
  std::string expected_path = "config.ini";
  std::vector<std::string> writes;
  std::vector<std::string> events;
  bool read_result = true;
  bool create_result = true;
  int fail_write_call = -1;
  int read_calls = 0;
  int create_calls = 0;
  int write_calls = 0;
  int close_calls = 0;
  bool object_manager_path = false;

  bool ReadExisting(std::string_view path, std::string* contents) override {
    assert(path == expected_path);
    ++read_calls;
    events.emplace_back("read");
    if (read_result) *contents = existing;
    return read_result;
  }

  bool CreateForRewrite(std::string_view path,
                        bool input_object_manager_path) override {
    assert(path == expected_path);
    ++create_calls;
    object_manager_path = input_object_manager_path;
    events.emplace_back("create");
    return create_result;
  }

  bool Write(std::string_view bytes) override {
    const int call = write_calls++;
    events.emplace_back("write");
    if (call == fail_write_call) return false;
    writes.emplace_back(bytes);
    output.append(bytes);
    return true;
  }

  void Close() override {
    ++close_calls;
    events.emplace_back("close");
  }
};

template <typename T>
void Store(std::array<std::uint8_t, 96>* data, std::size_t offset,
           const T& value) {
  std::memcpy(data->data() + offset, &value, sizeof(value));
}

struct Fixture {
  std::array<std::uint8_t, 96> values{};
  std::array<bandwagon::kernel_expansion::StructuredConfigEntry, 5> entries{};

  Fixture() {
    using namespace bandwagon::kernel_expansion;
    std::memcpy(values.data(), "Bandwagon", 9);
    values[16] = 1;
    const std::int32_t number = -42;
    Store(&values, 20, number);
    const std::array<float, 4> normalized{{
        1.0f, 128.0f / 255.0f, 64.0f / 255.0f, 0.0f}};
    const std::array<float, 4> raw{{255.0f, 128.0f, 64.0f, 1.0f}};
    std::memcpy(values.data() + 24, normalized.data(), sizeof(normalized));
    std::memcpy(values.data() + 40, raw.data(), sizeof(raw));

    entries = {{{"System", 0, "Name", 16,
                 StructuredConfigValueType::kString, 0},
                {"System", 0, "Enabled", 0,
                 StructuredConfigValueType::kBoolean, 16},
                {"System", 0, "Number", 0,
                 StructuredConfigValueType::kInteger, 20},
                {"Theme", 0, "Color", 0,
                 StructuredConfigValueType::kColor, 24},
                {"Theme", 0, "RawColor", 0x1111,
                 StructuredConfigValueType::kColor, 40}}};
  }
};

void TestTokenizer() {
  using namespace bandwagon::kernel_expansion;

  char text[] = " \tKey_Name junk = value \r\n[Section] ignored\n; note\n";
  char* cursor = text;
  ParsedIniLine parsed{};
  assert(ParseIniLine(&cursor, &parsed));
  assert(std::string(parsed.key) == "Key");
  assert(std::string(parsed.value) == "value ");
  assert(parsed.section == nullptr);

  assert(ParseIniLine(&cursor, &parsed));
  assert(std::string(parsed.section) == "Section");
  assert(parsed.key == nullptr && parsed.value == nullptr);

  assert(ParseIniLine(&cursor, &parsed));
  assert(parsed.section == nullptr && parsed.key == nullptr &&
         parsed.value == nullptr);
  assert(cursor != nullptr && *cursor == '\0');
  assert(!ParseIniLine(&cursor, &parsed));

  char empty_key[] = "=value";
  cursor = empty_key;
  assert(ParseIniLine(&cursor, &parsed));
  assert(std::string(parsed.key).empty());
  assert(std::string(parsed.value) == "value");
  assert(cursor == nullptr);

  char lone_cr[] = "\rbroken";
  cursor = lone_cr;
  assert(!ParseIniLine(&cursor, &parsed));
  assert(cursor == lone_cr);

  char unterminated_section[] = "[broken\n";
  cursor = unterminated_section;
  assert(!ParseIniLine(&cursor, &parsed));
  assert(cursor == unterminated_section);

  char whitespace_eof[] = "   ";
  cursor = whitespace_eof;
  assert(!ParseIniLine(&cursor, &parsed));
  assert(cursor == whitespace_eof);
}

void TestFreshFile() {
  using namespace bandwagon::kernel_expansion;
  Fixture fixture;
  FakeWriterServices files;
  assert(WriteStructuredConfigFile(
      &files, "config.ini", 0, fixture.values.data(), fixture.values.size(),
      fixture.entries.data(), fixture.entries.size()));
  assert(files.read_calls == 0);
  assert(files.create_calls == 1 && files.close_calls == 1);
  assert(!files.object_manager_path);
  assert(files.output ==
         "\r\n[System]\r\n"
         "Name = \"Bandwagon\"\r\n"
         "Enabled = true\r\n"
         "Number = -42\r\n"
         "\r\n[Theme]\r\n"
         "Color = 0xff804000\r\n"
         "RawColor = 0xff804001\r\n");
}

void TestPreservedFile() {
  using namespace bandwagon::kernel_expansion;
  Fixture fixture;
  FakeWriterServices files;
  files.existing =
      "; 100% literal\n"
      "[system]\n"
      "name = stale\n"
      "Unknown = 50% keep\n"
      "[Other]\n"
      "Flag = untouched\n";
  assert(WriteStructuredConfigFile(
      &files, "config.ini", 0x101, fixture.values.data(),
      fixture.values.size(), fixture.entries.data(), fixture.entries.size()));
  assert(files.read_calls == 1);
  assert(files.output ==
         "; 100% literal\r\n"
         "[system]\r\n"
         "Name = \"Bandwagon\"\r\n"
         "Unknown = 50% keep\r\n"
         "Enabled = true\r\n"
         "Number = -42\r\n"
         "\r\n"
         "[Other]\r\n"
         "Flag = untouched\r\n"
         "\r\n[Theme]\r\n"
         "Color = 0xff804000\r\n"
         "RawColor = 0xff804001\r\n");
}

void TestKeyOnlyAndDuplicateReplacement() {
  using namespace bandwagon::kernel_expansion;
  Fixture fixture;
  FakeWriterServices files;
  files.existing =
      "[Wrong]\n"
      "Name = first\n"
      "NAME = second\n";
  assert(WriteStructuredConfigFile(
      &files, "config.ini", 1, fixture.values.data(), fixture.values.size(),
      fixture.entries.data(), fixture.entries.size()));
  const std::string replacement = "Name = \"Bandwagon\"\r\n";
  const std::size_t first = files.output.find(replacement);
  assert(first != std::string::npos);
  assert(files.output.find(replacement, first + 1) != std::string::npos);
  assert(files.output.find("\r\n[System]\r\nEnabled") !=
         std::string::npos);
}

void TestFallbackModesAndFailures() {
  using namespace bandwagon::kernel_expansion;
  Fixture fixture;

  FakeWriterServices read_failure;
  read_failure.read_result = false;
  assert(WriteStructuredConfigFile(
      &read_failure, "config.ini", 1, fixture.values.data(),
      fixture.values.size(), fixture.entries.data(), fixture.entries.size()));
  assert(read_failure.read_calls == 1);
  assert(read_failure.output.rfind("\r\n[System]\r\n", 0) == 0);

  FakeWriterServices high_byte_only;
  assert(WriteStructuredConfigFile(
      &high_byte_only, "config.ini", 0x100, fixture.values.data(),
      fixture.values.size(), fixture.entries.data(), fixture.entries.size()));
  assert(high_byte_only.read_calls == 0);

  FakeWriterServices object_manager;
  object_manager.expected_path = "\\Device\\Config.ini";
  assert(WriteStructuredConfigFile(
      &object_manager, object_manager.expected_path, 0, nullptr, 0, nullptr,
      0));
  assert(object_manager.object_manager_path);

  FakeWriterServices create_failure;
  create_failure.create_result = false;
  assert(!WriteStructuredConfigFile(
      &create_failure, "config.ini", 0, fixture.values.data(),
      fixture.values.size(), fixture.entries.data(), fixture.entries.size()));
  assert(create_failure.close_calls == 0);

  FakeWriterServices malformed;
  malformed.existing = "[broken\n";
  assert(!WriteStructuredConfigFile(
      &malformed, "config.ini", 1, fixture.values.data(),
      fixture.values.size(), fixture.entries.data(), fixture.entries.size()));
  assert(malformed.close_calls == 1 && malformed.output.empty());

  FakeWriterServices write_failure;
  write_failure.fail_write_call = 0;
  assert(!WriteStructuredConfigFile(
      &write_failure, "config.ini", 0, fixture.values.data(),
      fixture.values.size(), fixture.entries.data(), fixture.entries.size()));
  assert(write_failure.close_calls == 1);
}

void TestBufferingAndLimits() {
  using namespace bandwagon::kernel_expansion;
  FakeWriterServices files;
  std::string expected;
  for (int index = 0; index < 16; ++index) {
    const std::string line = ";" + std::string(95, 'a' + index % 20);
    files.existing += line + "\n";
    expected += line + "\r\n";
  }
  assert(WriteStructuredConfigFile(
      &files, "config.ini", 1, nullptr, 0, nullptr, 0));
  assert(files.output == expected);
  assert(files.writes.size() >= 2);

  FakeWriterServices partial_write;
  partial_write.existing = files.existing;
  partial_write.fail_write_call = 1;
  assert(!WriteStructuredConfigFile(
      &partial_write, "config.ini", 1, nullptr, 0, nullptr, 0));
  assert(!partial_write.output.empty());
  assert(partial_write.output.size() < expected.size());
  assert(partial_write.close_calls == 1);

  FakeWriterServices maximum_line;
  maximum_line.existing = ";" + std::string(508, 'm') + "\n";
  assert(WriteStructuredConfigFile(
      &maximum_line, "config.ini", 1, nullptr, 0, nullptr, 0));
  assert(maximum_line.output.size() == 511);

  FakeWriterServices long_line;
  long_line.existing = "Key = " + std::string(510, 'x') + "\n";
  assert(!WriteStructuredConfigFile(
      &long_line, "config.ini", 1, nullptr, 0, nullptr, 0));
  assert(long_line.close_calls == 1);

  FakeWriterServices long_section;
  long_section.existing = "[" + std::string(140, 's') + "]\n";
  assert(WriteStructuredConfigFile(
      &long_section, "config.ini", 1, nullptr, 0, nullptr, 0));
  assert(long_section.output ==
         "[" + std::string(127, 's') + "]\r\n");

  FakeWriterServices direct;
  ConfigWriteContext context(nullptr, 0, nullptr, 0, &direct);
  const std::string maximum(1023, 'x');
  assert(AppendLiteralConfigOutput(&context, maximum));
  assert(!AppendLiteralConfigOutput(&context, std::string(1024, 'x')));
  assert(FlushConfigOutput(&context));
  assert(direct.output == maximum);
}

void TestUnknownTypeAndWorkflowBridge() {
  using namespace bandwagon::kernel_expansion;
  std::array<std::uint8_t, 1> value{{0}};
  const StructuredConfigEntry unknown{
      "Section", 0, "Unknown", 0,
      static_cast<StructuredConfigValueType>(99), 0};
  FakeWriterServices direct;
  ConfigWriteContext context(value.data(), value.size(), &unknown, 1, &direct);
  assert(SerializeConfigEntry(&context, &unknown));
  assert(context.output_size == 0);

  Fixture fixture;
  PeerPressureConfig config{};
  std::copy(fixture.values.begin(), fixture.values.end(),
            config.Data());
  FakeWriterServices workflow_files;
  StructuredConfigFileWorkflow workflow(&workflow_files);
  assert(workflow.Process("config.ini", 0, &config, fixture.entries.data(),
                          fixture.entries.size()) == 1);
  assert(workflow_files.output.find("Name = \"Bandwagon\"") !=
         std::string::npos);
}

}  // namespace

int main() {
  TestTokenizer();
  TestFreshFile();
  TestPreservedFile();
  TestKeyOnlyAndDuplicateReplacement();
  TestFallbackModesAndFailures();
  TestBufferingAndLimits();
  TestUnknownTypeAndWorkflowBridge();
  std::cout << "Kernel Expansion config writer tests passed\n";
}
