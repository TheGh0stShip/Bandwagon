#include "boot_animation_theme.hpp"

#include <algorithm>
#include <string>

namespace bandwagon::kernel_expansion {

bool LoadBootAnimationTheme(
    const PeerPressureConfig& config, BootAnimationThemePayload* active_theme,
    const BootAnimationThemeInitializer& initialize,
    const BootAnimationThemeDecoder& decode) {
  if (!active_theme || !initialize || !decode) return false;

  BootAnimationThemePayload candidate{};
  initialize(&candidate);

  const auto end = std::find(config.boot_animation_theme_name.begin(),
                             config.boot_animation_theme_name.end(), '\0');
  const std::size_t name_size = static_cast<std::size_t>(
      end - config.boot_animation_theme_name.begin());
  std::string path{kBootAnimationThemePathPrefix};
  path.append(config.boot_animation_theme_name.data(), name_size);
  path.append(kBootAnimationThemeExtension);

  if (!decode(path, &candidate, kBootAnimationThemeSchemaEntryCount) ||
      candidate.version != kBootAnimationThemeVersion) {
    return false;
  }
  *active_theme = candidate;
  return true;
}

}  // namespace bandwagon::kernel_expansion
