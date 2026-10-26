#include "parent_directories.hpp"

namespace bandwagon::updater {

std::vector<std::string> EnumerateParentDirectories(std::string_view path) {
  std::vector<std::string> result;
  // The closed helper treats the first backslash as the root separator and
  // begins creating prefixes at the following slash of either kind.
  const std::size_t root_separator = path.find('\\');
  if (root_separator == std::string_view::npos) return result;
  std::size_t separator = path.find_first_of("\\/", root_separator + 1);
  while (separator != std::string_view::npos) {
    result.emplace_back(path.substr(0, separator));
    separator = path.find_first_of("\\/", separator + 1);
  }
  return result;
}

}  // namespace bandwagon::updater
