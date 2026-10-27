#include "xui_scene_override.hpp"

namespace bandwagon::kernel_expansion {

std::int32_t CreatePeerPressureScene(std::u16string_view resource_base,
                                     std::u16string_view scene_name,
                                     std::uintptr_t init_data,
                                     std::uintptr_t scene_out,
                                     XuiSceneServices* services) {
  for (const std::u16string_view overridden_name : kOverriddenSceneNames) {
    if (scene_name != overridden_name) continue;
    const std::int32_t status =
        services->CreateScene(kPeerPressureHudResource, scene_name, init_data,
                              scene_out);
    if (status == 0) return status;
    break;
  }
  return services->CreateScene(resource_base, scene_name, init_data, scene_out);
}

}  // namespace bandwagon::kernel_expansion
