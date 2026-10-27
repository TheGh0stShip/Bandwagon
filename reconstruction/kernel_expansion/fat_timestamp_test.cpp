#include "fat_timestamp.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

using namespace bandwagon::kernel_expansion;

namespace {

class FakeServices final : public FatTimestampServices {
 public:
  bool TimeFieldsToTime(const TimeFields& value,
                        std::uint64_t* time) override {
    events.emplace_back("fields-to-time");
    converted_fields = value;
    *time = converted_time;
    return conversion_succeeds;
  }
  void QuerySystemTime(std::uint64_t* time) override {
    events.emplace_back("query-time");
    *time = queried_time;
  }
  void TimeToTimeFields(std::uint64_t time, TimeFields* value) override {
    events.emplace_back("time-to-fields");
    observed_time = time;
    *value = current_fields;
  }

  bool conversion_succeeds = true;
  std::uint64_t converted_time = 0x1122334455667788ull;
  std::uint64_t queried_time = 0x8877665544332211ull;
  std::uint64_t observed_time = 0;
  TimeFields converted_fields{};
  TimeFields current_fields{};
  std::vector<std::string> events;
};

}  // namespace

int main() {
  constexpr std::uint16_t date =
      static_cast<std::uint16_t>(((2024 - 1980) << 9) | (10 << 5) | 26);
  constexpr std::uint16_t time =
      static_cast<std::uint16_t>((14 << 11) | (37 << 5) | (58 / 2));

  FakeServices services;
  std::uint64_t converted = 0;
  FatDateTimeToSystemTime(date, time, &converted, &services);
  assert(converted == services.converted_time);
  assert(services.converted_fields.year == 2024);
  assert(services.converted_fields.month == 10);
  assert(services.converted_fields.day == 26);
  assert(services.converted_fields.hour == 14);
  assert(services.converted_fields.minute == 37);
  assert(services.converted_fields.second == 58);
  assert(services.converted_fields.milliseconds == 0);

  services.conversion_succeeds = false;
  converted = ~std::uint64_t{0};
  FatDateTimeToSystemTime(date, time, &converted, &services);
  assert(converted == 0);

  services.events.clear();
  services.current_fields = TimeFields{2024, 10, 26, 14, 37, 58, 999};
  assert(QueryCurrentFatTimestamp(&services) ==
         ((44u << 25) | (10u << 21) | (26u << 16) | (14u << 11) |
          (37u << 5) | 29u));
  assert(services.observed_time == services.queried_time);
  assert((services.events ==
          std::vector<std::string>{"query-time", "time-to-fields"}));

  std::cout << "Kernel Expansion FAT timestamp tests passed\n";
}
