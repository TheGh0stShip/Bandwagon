#include "config_reader.hpp"

#include "config_schema.hpp"

#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

namespace {

struct Event {
  bool section_is_null;
  bool key_is_null;
  bool value_is_null;
  std::string section;
  std::string key;
  std::string value;
};

void RecordEvent(void* opaque, const char* section,
                 const char* key, const char* value) {
  auto* events = static_cast<std::vector<Event>*>(opaque);
  events->push_back({section == nullptr, key == nullptr, value == nullptr,
                     section ? section : "", key ? key : "",
                     value ? value : ""});
}

class FakeDiagnostics final
    : public bandwagon::kernel_expansion::ConfigReaderDiagnostics {
 public:
  int calls = 0;
  int parse_failures = 0;
  std::string key;
  std::uint32_t hash = 0;

  void UnknownKey(const char* input_key, std::uint32_t input_hash) override {
    ++calls;
    key = input_key;
    hash = input_hash;
  }

  void ParseFailure() override { ++parse_failures; }
};

void TestEventOrdering() {
  using namespace bandwagon::kernel_expansion;
  char text[] =
      "Before=zero\n"
      "; ignored\n"
      "[Alpha]\n"
      "A=1\n"
      "\n"
      "[Beta]\n"
      "B=2";
  std::vector<Event> events;
  assert(ParseIniDocument(text, RecordEvent, &events));
  assert(events.size() == 7);
  assert(!events[0].section_is_null && events[0].section.empty());
  assert(events[0].key == "Before" && events[0].value == "zero");
  assert(events[1].section == "Alpha" && events[1].key_is_null);
  assert(events[2].section == "Alpha" && events[2].key == "A" &&
         events[2].value == "1");
  assert(events[3].section_is_null && events[3].key_is_null);
  assert(events[4].section == "Beta" && events[4].key_is_null);
  assert(events[5].section == "Beta" && events[5].key == "B" &&
         events[5].value == "2");
  assert(events[6].section_is_null && events[6].key_is_null);
}

void TestSectionAndFailureEdges() {
  using namespace bandwagon::kernel_expansion;
  std::string long_text = "[" + std::string(140, 's') + "]\n";
  std::vector<Event> events;
  assert(ParseIniDocument(long_text.data(), RecordEvent, &events));
  assert(events.size() == 2);
  assert(events[0].section.size() == 127);
  assert(events[1].section_is_null);

  char empty_section[] = "[]\n";
  events.clear();
  assert(ParseIniDocument(empty_section, RecordEvent, &events));
  assert(events.size() == 1 && events[0].section.empty() &&
         !events[0].section_is_null);

  char empty[] = "";
  events.clear();
  assert(ParseIniDocument(empty, RecordEvent, &events));
  assert(events.empty());

  for (std::string invalid : {
           std::string("[broken\n"), std::string("MissingEquals\n"),
           std::string("\rbroken"), std::string("   ")}) {
    events.clear();
    assert(!ParseIniDocument(invalid.data(), RecordEvent, &events));
  }
  assert(!ParseIniDocument(nullptr, RecordEvent, &events));
  assert(!ParseIniDocument(empty, nullptr, &events));
}

void TestConcreteSchema() {
  using namespace bandwagon::kernel_expansion;
  std::string text =
      "[BootAnimation]\n"
      "BootAnimationType = 2\n"
      "BootAnimationThemeName = \"Metro\"\n"
      "[Dashboard]\n"
      "DashboardPath = Hdd:\\dash.xex\n"
      "[System]\n"
      "CheckForUpdatesOnStartup = false\n"
      "EjectButtonAction = -3tail\n"
      "OtherOSName = Xell\n"
      "Unknown = 1";
  PeerPressureConfig config{};
  FakeDiagnostics diagnostics;
  assert(ParseConfigFileBuffer(
      text.data(), {config.Data(), config.Size()},
      kPeerPressureConfigSchema.data(), kPeerPressureConfigSchema.size(),
      &diagnostics));
  assert(config.boot_animation_type == 2);
  assert(std::strcmp(config.boot_animation_theme_name.data(), "Metro") == 0);
  assert(std::strcmp(config.dashboard_path.data(), "Hdd:\\dash.xex") == 0);
  assert(!config.CheckForUpdatesOnStartup());
  assert(config.eject_button_action == -3);
  assert(std::strcmp(config.other_os_name.data(), "Xell") == 0);
  assert(diagnostics.calls == 1 && diagnostics.key == "Unknown");
  assert(diagnostics.parse_failures == 0);

  PeerPressureConfig through_interface{};
  ConfigFileBufferParser parser(&diagnostics);
  std::string one_value =
      "[System]\nCheckForUpdatesOnStartup = True\n";
  assert(parser.Parse(
      reinterpret_cast<std::uint8_t*>(one_value.data()),
      {through_interface.Data(), through_interface.Size()},
      kPeerPressureConfigSchema.data(), kPeerPressureConfigSchema.size()));
  assert(through_interface.CheckForUpdatesOnStartup());

  std::string malformed = "[broken\n";
  assert(!ParseConfigFileBuffer(
      malformed.data(), {config.Data(), config.Size()},
      kPeerPressureConfigSchema.data(), kPeerPressureConfigSchema.size(),
      &diagnostics));
  assert(diagnostics.parse_failures == 1);

  assert(!ParseConfigFileBuffer(
      nullptr, {config.Data(), config.Size()},
      kPeerPressureConfigSchema.data(), kPeerPressureConfigSchema.size(),
      &diagnostics));
  assert(diagnostics.parse_failures == 1);
}

}  // namespace

int main() {
  TestEventOrdering();
  TestSectionAndFailureEdges();
  TestConcreteSchema();
  std::cout << "Kernel Expansion config reader tests passed\n";
}
