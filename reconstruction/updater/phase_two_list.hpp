#pragma once

#include <array>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace bandwagon::updater {

constexpr std::size_t kPhaseTwoLineCapacity = 260;

struct PhaseTwoLine {
  std::array<char, kPhaseTwoLineCapacity> buffer{};
  std::size_t reported_write_size = 0;
  bool truncated = false;
};

// Models snprintf(buffer, 260, "%s\n", path). The closed code passes the
// returned length to its write helper even when truncation occurred.
PhaseTwoLine FormatPhaseTwoLine(std::string_view path);

// Produces the deterministic file contents for inputs on which the closed
// implementation does not read past its stack buffer. Returns false for the
// unsafe truncation case and leaves output unchanged.
bool SerializePhaseTwoFileList(const std::vector<std::string>& paths,
                               std::string* output);

}  // namespace bandwagon::updater
