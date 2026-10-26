#include "parent_directories.hpp"

namespace bandwagon::updater {

std::vector<std::string> EnumerateParentDirectories(std::string_view path) {
  std::vector<std::string> result;
  std::size_t separator = path.find('\\', 1);
  while (separator != std::string_view::npos) {
    result.emplace_back(path.substr(0, separator));
    separator = path.find('\\', separator + 1);
  }
  return result;
}

}  // namespace bandwagon::updater
