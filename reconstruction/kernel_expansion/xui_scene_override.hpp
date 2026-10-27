#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace bandwagon::kernel_expansion {

inline constexpr std::u16string_view kPeerPressureHudResource =
    u"file://\\Device\\Harddisk0\\Partition1\\PeerPressure\\System\\HudResources.xzp#";

inline constexpr std::array<std::u16string_view, 4> kOverriddenSceneNames{{
    u"HomeTabSignedIn.xur",
    u"HomeTabSignedOut.xur",
    u"SettingsTabSignedIn.xur",
    u"SettingsTabSignedOut.xur",
}};

class XuiSceneServices {
 public:
  virtual ~XuiSceneServices() = default;
  virtual std::int32_t CreateScene(std::u16string_view resource_base,
                                   std::u16string_view scene_name,
                                   std::uintptr_t init_data,
                                   std::uintptr_t scene_out) = 0;
};

// Scene-resource override at 0x801f8570.
std::int32_t CreatePeerPressureScene(std::u16string_view resource_base,
                                     std::u16string_view scene_name,
                                     std::uintptr_t init_data,
                                     std::uintptr_t scene_out,
                                     XuiSceneServices* services);

}  // namespace bandwagon::kernel_expansion
