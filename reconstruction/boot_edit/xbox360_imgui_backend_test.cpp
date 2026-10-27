#include "xbox360_imgui_backend.hpp"

#include <cassert>
#include <iostream>

namespace xb = bandwagon::boot_edit::xbox_backend;

namespace {
struct Services final : xb::BackendServices {
  std::array<bool, 3> object_ok{true, true, true};
  std::array<std::uint32_t, xb::kRenderStateCount> states{};
  int gets = 0, sets = 0, texture_uploads = 0, vertex_buffers = 0;
  int index_buffers = 0, apply_calls = 0, callbacks = 0;
  std::vector<std::uint8_t> texture_pixels;
  std::vector<xb::DrawCall> draws;
  xb::RenderStatePlan last_plan;
  bool CreateShaderObject(xb::ShaderObject object) override {
    return object_ok[static_cast<std::size_t>(object)];
  }
  std::uint32_t GetRenderState(std::size_t index) override {
    ++gets;
    return states[index];
  }
  void SetRenderState(std::size_t index, std::uint32_t value) override {
    ++sets;
    assert(states[index] == value);
  }
  bool UploadTexture(int width, int height,
                     const std::vector<std::uint8_t>& pixels,
                     std::uint32_t* texture_id) override {
    assert(width == 1 && height == 1);
    ++texture_uploads;
    texture_pixels = pixels;
    *texture_id = 77;
    return true;
  }
  bool EnsureVertexBuffer(int) override { ++vertex_buffers; return true; }
  bool EnsureIndexBuffer(int) override { ++index_buffers; return true; }
  void ApplyRenderState(const xb::RenderStatePlan& plan) override {
    ++apply_calls;
    last_plan = plan;
  }
  void InvokeUserCallback(std::size_t, std::size_t) override { ++callbacks; }
  void Draw(const xb::DrawCall& call) override { draws.push_back(call); }
};
}  // namespace

int main() {
  xb::BackendState state;
  Services services;
  for (std::size_t i = 0; i < services.states.size(); ++i)
    services.states[i] = static_cast<std::uint32_t>(i * 3 + 1);

  assert(xb::InitializeXbox360Backend(&state, 1280, 720, &services));
  assert(state.initialized && state.vertex_declaration && state.vertex_shader);
  assert(state.pixel_shader && state.vertex_capacity == 5000);
  assert(state.index_capacity == 10000 && state.backend_flags == 0x0a);
  assert(state.platform_name == "imgui_impl_xbox360");
  assert(state.renderer_name == "imgui_impl_dx9");
  assert(state.key_map == xb::kKeyMap);

  xb::BackendState failed;
  services.object_ok[1] = false;
  assert(!xb::InitializeXbox360Backend(&failed, 640, 480, &services));
  assert(failed.vertex_declaration && !failed.vertex_shader && !failed.initialized);
  services.object_ok[1] = true;

  xb::FontAtlas atlas{1, 1, 4, true, {0x11, 0x22, 0x33, 0x44}};
  xb::ControllerState controller;
  controller.available = true;
  controller.digital[0] = true;
  controller.digital[10] = true;
  controller.left_x = 32767;
  controller.left_y = -32768;
  assert(xb::Xbox360BackendNewFrame(&state, controller, atlas, &services));
  assert(state.font_texture && state.font_texture_id == 77);
  assert(services.texture_pixels == std::vector<std::uint8_t>({0x33, 0x22, 0x11, 0x44}));
  assert(state.navigation_inputs[0] == 1.0f);
  assert(state.navigation_inputs[19] == 1.0f);
  assert(state.navigation_inputs[8] > 0.99f);
  assert(state.navigation_inputs[10] > 0.99f);

  xb::DrawData data;
  data.display_width = 1280;
  data.display_height = 720;
  xb::DrawList list;
  list.vertices.push_back({1, 2, 0x11223344u, 0.25f, 0.75f});
  list.indices = {0, 0, 0};
  list.commands.push_back({{0, 0, 100, 100}, 9, 3, 0, 0, false, false});
  list.commands.push_back({{}, 0, 0, 0, 0, false, true});
  list.commands.push_back({{}, 0, 0, 0, 0, true, true});
  data.lists.push_back(list);
  std::vector<xb::XboxVertex> vertices;
  std::vector<std::uint16_t> indices;
  assert(xb::RenderXbox360DrawData(&state, data, &services, &vertices, &indices));
  assert(services.vertex_buffers == 1 && services.index_buffers == 1);
  assert(services.gets == 97 && services.sets == 97);
  assert(vertices.size() == 1 && vertices[0].z == 0.0f);
  assert(vertices[0].color == 0x11443322u);
  assert(indices == list.indices);
  assert(services.draws.size() == 1);
  assert(services.draws[0].primitive_count == 1);
  assert(services.callbacks == 1 && services.apply_calls == 2);
  assert(services.last_plan.viewport_width == 1280);

  xb::DrawData hidden;
  assert(!xb::RenderXbox360DrawData(&state, hidden, &services, &vertices, &indices));
  std::cout << "BootEdit Xbox backend reconstruction tests passed\n";
}
