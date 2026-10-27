#include "xui_scene_override.hpp"

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

using namespace bandwagon::kernel_expansion;

namespace {

struct SceneCall {
  std::u16string resource_base;
  std::u16string scene_name;
  std::uintptr_t init_data;
  std::uintptr_t scene_out;
};

class FakeServices final : public XuiSceneServices {
 public:
  std::int32_t CreateScene(std::u16string_view resource_base,
                           std::u16string_view scene_name,
                           std::uintptr_t init_data,
                           std::uintptr_t scene_out) override {
    calls.push_back({std::u16string(resource_base), std::u16string(scene_name),
                     init_data, scene_out});
    return statuses[calls.size() - 1];
  }

  std::vector<std::int32_t> statuses;
  std::vector<SceneCall> calls;
};

}  // namespace

int main() {
  for (const std::u16string_view scene : kOverriddenSceneNames) {
    FakeServices success;
    success.statuses = {0};
    assert(CreatePeerPressureScene(u"stock://", scene, 0x1111, 0x2222,
                                   &success) == 0);
    assert(success.calls.size() == 1);
    assert(success.calls[0].resource_base == kPeerPressureHudResource);
    assert(success.calls[0].scene_name == scene);
    assert(success.calls[0].init_data == 0x1111);
    assert(success.calls[0].scene_out == 0x2222);
  }

  FakeServices fallback;
  fallback.statuses = {-7, 23};
  assert(CreatePeerPressureScene(u"stock://", kOverriddenSceneNames[0],
                                 1, 2, &fallback) == 23);
  assert(fallback.calls.size() == 2);
  assert(fallback.calls[0].resource_base == kPeerPressureHudResource);
  assert(fallback.calls[1].resource_base == u"stock://");

  FakeServices ordinary;
  ordinary.statuses = {17};
  assert(CreatePeerPressureScene(u"stock://", u"OtherScene.xur", 3, 4,
                                 &ordinary) == 17);
  assert(ordinary.calls.size() == 1);
  assert(ordinary.calls[0].resource_base == u"stock://");

  // Binary comparison is case-sensitive.
  FakeServices wrong_case;
  wrong_case.statuses = {9};
  assert(CreatePeerPressureScene(u"stock://", u"hometabsignedin.xur", 0, 0,
                                 &wrong_case) == 9);
  assert(wrong_case.calls[0].resource_base == u"stock://");

  std::cout << "Kernel Expansion XUI scene override tests passed\n";
}
