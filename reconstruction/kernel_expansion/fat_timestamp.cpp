#include "fat_timestamp.hpp"

namespace bandwagon::kernel_expansion {

void FatDateTimeToSystemTime(std::uint16_t date, std::uint16_t time,
                             std::uint64_t* system_time,
                             FatTimestampServices* services) {
  const TimeFields fields{
      static_cast<std::uint16_t>(1980u + ((date >> 9) & 0x7fu)),
      static_cast<std::uint16_t>((date >> 5) & 0x0fu),
      static_cast<std::uint16_t>(date & 0x1fu),
      static_cast<std::uint16_t>((time >> 11) & 0x1fu),
      static_cast<std::uint16_t>((time >> 5) & 0x3fu),
      static_cast<std::uint16_t>((time & 0x1fu) << 1),
      0};
  if (!services->TimeFieldsToTime(fields, system_time)) {
    *system_time = 0;
  }
}

std::uint32_t QueryCurrentFatTimestamp(FatTimestampServices* services) {
  std::uint64_t system_time = 0;
  TimeFields fields{};
  services->QuerySystemTime(&system_time);
  services->TimeToTimeFields(system_time, &fields);
  return (static_cast<std::uint32_t>(fields.year - 1980u) << 25) |
         (static_cast<std::uint32_t>(fields.month) << 21) |
         (static_cast<std::uint32_t>(fields.day) << 16) |
         (static_cast<std::uint32_t>(fields.hour) << 11) |
         (static_cast<std::uint32_t>(fields.minute) << 5) |
         (static_cast<std::uint32_t>(fields.second) >> 1);
}

}  // namespace bandwagon::kernel_expansion
