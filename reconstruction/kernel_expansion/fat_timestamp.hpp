#pragma once

#include <cstdint>

namespace bandwagon::kernel_expansion {

struct TimeFields {
  std::uint16_t year = 0;
  std::uint16_t month = 0;
  std::uint16_t day = 0;
  std::uint16_t hour = 0;
  std::uint16_t minute = 0;
  std::uint16_t second = 0;
  std::uint16_t milliseconds = 0;
};

class FatTimestampServices {
 public:
  virtual ~FatTimestampServices() = default;
  virtual bool TimeFieldsToTime(const TimeFields& fields,
                                std::uint64_t* time) = 0;
  virtual void QuerySystemTime(std::uint64_t* time) = 0;
  virtual void TimeToTimeFields(std::uint64_t time, TimeFields* fields) = 0;
};

// Convert the two on-disk FAT date/time words to Xbox system time. Invalid
// field combinations are represented as zero, matching 0x801fa1d0.
void FatDateTimeToSystemTime(std::uint16_t date, std::uint16_t time,
                             std::uint64_t* system_time,
                             FatTimestampServices* services);

// FatFs get_fattime platform adapter at 0x801fbde0.
std::uint32_t QueryCurrentFatTimestamp(FatTimestampServices* services);

}  // namespace bandwagon::kernel_expansion
