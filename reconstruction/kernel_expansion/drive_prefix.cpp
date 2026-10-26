#include "drive_prefix.hpp"

namespace bandwagon::kernel_expansion {

std::int32_t ParseDriveNumberPrefix(const char16_t** cursor) {
  if (!cursor || !*cursor) return -1;

  const char16_t* const start = *cursor;
  const char16_t* scan = start;
  while (*scan >= 0x20 && *scan != u':') ++scan;

  if (*scan != u':') return 0;
  if (*start < u'0' || *start > u'9') return -1;
  if (scan != start + 1) return -1;

  *cursor = scan + 1;
  return static_cast<std::int32_t>(*start - u'0');
}

}  // namespace bandwagon::kernel_expansion
