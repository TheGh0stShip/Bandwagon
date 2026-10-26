#include "phase_two_list.hpp"

#include <algorithm>

namespace bandwagon::updater {

PhaseTwoLine FormatPhaseTwoLine(std::string_view path) {
  PhaseTwoLine result;
  result.reported_write_size = path.size() + 1;  // The trailing newline.
  result.truncated = result.reported_write_size >= result.buffer.size();

  const std::size_t copied =
      std::min(path.size(), result.buffer.size() - 1);
  std::copy_n(path.begin(), copied, result.buffer.begin());
  if (copied < result.buffer.size() - 1) result.buffer[copied] = '\n';
  // Value-initialization supplies snprintf's terminating NUL and models the
  // explicit stack-buffer clearing observed before the loop.
  return result;
}

bool SerializePhaseTwoFileList(const std::vector<std::string>& paths,
                               std::string* output) {
  if (!output) return false;
  std::string serialized;
  for (const auto& path : paths) {
    const PhaseTwoLine line = FormatPhaseTwoLine(path);
    if (line.truncated) return false;
    serialized.append(line.buffer.data(), line.reported_write_size);
  }
  *output = std::move(serialized);
  return true;
}

}  // namespace bandwagon::updater
