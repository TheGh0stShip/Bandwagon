#include "boot_animation_theme.hpp"

#include <cassert>
#include <cstring>
#include <string>

using namespace bandwagon::kernel_expansion;

int main() {
  PeerPressureConfig config{};
  std::memcpy(config.boot_animation_theme_name.data(), "Metro", 6);
  BootAnimationThemePayload active{};
  active.version = 0xaaaaaaaa;
  std::string observed_path;
  std::uint32_t observed_count = 0;

  const bool loaded = LoadBootAnimationTheme(
      config, &active,
      [](BootAnimationThemePayload* payload) {
        payload->version = 1;
        payload->data.fill(0x11);
      },
      [&](std::string_view path, BootAnimationThemePayload* payload,
          std::uint32_t count) {
        observed_path = path;
        observed_count = count;
        payload->version = kBootAnimationThemeVersion;
        payload->data.fill(0x5a);
        return true;
      });
  assert(loaded);
  assert(observed_path ==
         "\\Device\\Harddisk0\\Partition1\\PeerPressure\\Themes\\BootAnim\\9199\\Metro.xbat");
  assert(observed_count == 41);
  assert(active.version == 9199);
  assert(active.data.front() == 0x5a && active.data.back() == 0x5a);

  BootAnimationThemePayload unchanged = active;
  assert(!LoadBootAnimationTheme(
      config, &active, [](BootAnimationThemePayload*) {},
      [](std::string_view, BootAnimationThemePayload*, std::uint32_t) {
        return false;
      }));
  assert(active.version == unchanged.version && active.data == unchanged.data);

  assert(!LoadBootAnimationTheme(
      config, &active, [](BootAnimationThemePayload*) {},
      [](std::string_view, BootAnimationThemePayload* payload,
         std::uint32_t) {
        payload->version = 9200;
        return true;
      }));
  assert(active.version == unchanged.version && active.data == unchanged.data);

  config.boot_animation_theme_name.fill('X');
  assert(LoadBootAnimationTheme(
      config, &active, [](BootAnimationThemePayload*) {},
      [](std::string_view path, BootAnimationThemePayload* payload,
         std::uint32_t) {
        assert(path.size() == kBootAnimationThemePathPrefix.size() +
                                  kBootAnimationThemeNameCapacity +
                                  kBootAnimationThemeExtension.size());
        payload->version = 9199;
        return true;
      }));
}
