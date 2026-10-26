#include "phase_two_list.hpp"

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

int main() {
  using bandwagon::updater::FormatPhaseTwoLine;
  using bandwagon::updater::SerializePhaseTwoFileList;

  std::string serialized;
  assert(SerializePhaseTwoFileList(
      {"PPROOT:\\System\\HudResources.xzp", "PPROOT:\\OtherOS\\boot.ini"},
      &serialized));
  assert(serialized == "PPROOT:\\System\\HudResources.xzp\n"
                       "PPROOT:\\OtherOS\\boot.ini\n");
  assert(SerializePhaseTwoFileList({}, &serialized));
  assert(serialized.empty());

  std::string longest_safe(258, 'a');
  const auto safe = FormatPhaseTwoLine(longest_safe);
  assert(!safe.truncated);
  assert(safe.reported_write_size == 259);
  assert(safe.buffer[258] == '\n');
  assert(safe.buffer[259] == '\0');

  std::string unsafe(259, 'b');
  const auto truncated = FormatPhaseTwoLine(unsafe);
  assert(truncated.truncated);
  assert(truncated.reported_write_size == 260);
  assert(truncated.buffer[258] == 'b');
  assert(truncated.buffer[259] == '\0');
  serialized = "unchanged";
  assert(!SerializePhaseTwoFileList({unsafe}, &serialized));
  assert(serialized == "unchanged");
  assert(!SerializePhaseTwoFileList({}, nullptr));

  std::cout << "Updater phase-two list reconstruction tests passed\n";
}
