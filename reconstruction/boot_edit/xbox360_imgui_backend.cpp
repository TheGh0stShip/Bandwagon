#include "xbox360_imgui_backend.hpp"

#include <algorithm>

namespace bandwagon::boot_edit::xbox_backend {
namespace {

std::int32_t CreateObject(BackendState* state, BackendServices* services,
                          ShaderObject object, bool BackendState::*field) {
  if (!state || !services) return kOutOfMemory;
  state->*field = services->CreateShaderObject(object);
  return state->*field ? 0 : kOutOfMemory;
}

float PositiveAxis(std::int16_t value, int dead_zone, float scale) {
  return std::clamp((static_cast<int>(value) - dead_zone) * scale, 0.0f, 1.0f);
}

float NegativeAxis(std::int16_t value, int dead_zone, float scale) {
  return std::clamp((-static_cast<int>(value) - dead_zone) * scale, 0.0f, 1.0f);
}

std::uint32_t RgbaToBgra(std::uint32_t color) {
  return (color & 0xff00ff00u) | ((color & 0x00ff0000u) >> 16) |
         ((color & 0x000000ffu) << 16);
}

}  // namespace

std::int32_t CreateVertexDeclaration(BackendState* state,
                                     BackendServices* services) {
  return CreateObject(state, services, ShaderObject::kVertexDeclaration,
                      &BackendState::vertex_declaration);
}

std::int32_t CreateVertexShader(BackendState* state,
                                BackendServices* services) {
  return CreateObject(state, services, ShaderObject::kVertexShader,
                      &BackendState::vertex_shader);
}

std::int32_t CreatePixelShader(BackendState* state,
                               BackendServices* services) {
  return CreateObject(state, services, ShaderObject::kPixelShader,
                      &BackendState::pixel_shader);
}

void CaptureRenderStateTable(BackendState* state, BackendServices* services) {
  if (!state || !services) return;
  for (std::size_t index = 0; index < kRenderStateCount; ++index)
    CaptureRenderStateSlot(state, index, services);
}

void CaptureRenderStateSlot(BackendState* state, std::size_t index,
                            BackendServices* services) {
  if (!state || !services || index >= kRenderStateCount) return;
  state->saved_render_state[index] = services->GetRenderState(index);
}

void RestoreRenderStateTable(const BackendState& state,
                             BackendServices* services) {
  if (!services) return;
  for (std::size_t index = 0; index < kRenderStateCount; ++index)
    services->SetRenderState(index, state.saved_render_state[index]);
}

bool CreateFontsTexture(BackendState* state, const FontAtlas& atlas,
                        BackendServices* services) {
  if (!state || !services || atlas.width <= 0 || atlas.height <= 0 ||
      atlas.bytes_per_pixel != 4 ||
      atlas.rgba_pixels.size() !=
          static_cast<std::size_t>(atlas.width * atlas.height * 4))
    return false;
  std::vector<std::uint8_t> pixels = atlas.rgba_pixels;
  if (atlas.uses_color_pixels) {
    for (std::size_t offset = 0; offset < pixels.size(); offset += 4)
      std::swap(pixels[offset], pixels[offset + 2]);
  }
  std::uint32_t texture = 0;
  if (!services->UploadTexture(atlas.width, atlas.height, pixels, &texture))
    return false;
  state->font_texture = true;
  state->font_texture_id = texture;
  return true;
}

bool InitializeXbox360Backend(BackendState* state, float display_width,
                              float display_height,
                              BackendServices* services) {
  if (!state || !services) return false;
  *state = {};
  state->vertex_capacity = 5000;
  state->index_capacity = 10000;
  state->display_width = display_width;
  state->display_height = display_height;
  state->platform_name = "imgui_impl_xbox360";
  state->renderer_name = "imgui_impl_dx9";
  state->backend_flags = 0x0au;
  state->key_map = kKeyMap;
  if (CreateVertexDeclaration(state, services) < 0 ||
      CreateVertexShader(state, services) < 0 ||
      CreatePixelShader(state, services) < 0)
    return false;
  state->initialized = true;
  return true;
}

bool Xbox360BackendNewFrame(BackendState* state,
                            const ControllerState& controller,
                            const FontAtlas& atlas,
                            BackendServices* services) {
  if (!state || !services) return false;
  if (!state->font_texture && !CreateFontsTexture(state, atlas, services))
    return false;
  state->navigation_inputs.fill(0.0f);
  if (!controller.available) return true;
  constexpr std::array<std::size_t, 11> slots{
      0, 1, 2, 4, 5, 6, 7, 16, 17, 18, 19};
  for (std::size_t index = 0; index < slots.size(); ++index)
    state->navigation_inputs[slots[index]] = controller.digital[index] ? 1.0f : 0.0f;

  // Exact dead zones and reciprocal ranges recovered from the Xbox routine.
  state->navigation_inputs[8] = PositiveAxis(controller.left_x, 7849, 0.000041531689930707216f);
  state->navigation_inputs[9] = NegativeAxis(controller.left_x, 7849, 0.000040131631976692006f);
  state->navigation_inputs[10] = NegativeAxis(controller.left_y, 7849, 0.000040130020352080464f);
  state->navigation_inputs[11] = PositiveAxis(controller.left_y, 7849, 0.000040131631976692006f);
  state->navigation_inputs[12] = PositiveAxis(controller.right_x, 8689, 0.000041529965528752655f);
  state->navigation_inputs[13] = NegativeAxis(controller.right_x, 8689, 0.000041531689930707216f);
  state->navigation_inputs[14] = NegativeAxis(controller.right_y, 8689, 0.000041531689930707216f);
  state->navigation_inputs[15] = PositiveAxis(controller.right_y, 8689, 0.000041529965528752655f);
  return true;
}

RenderStatePlan BuildRenderStatePlan(const DrawData& draw_data) {
  RenderStatePlan plan;
  plan.viewport_width = static_cast<int>(draw_data.display_width);
  plan.viewport_height = static_cast<int>(draw_data.display_height);
  const float left = draw_data.display_x + 0.5f;
  const float right = draw_data.display_x + draw_data.display_width + 0.5f;
  const float top = draw_data.display_y + 0.5f;
  const float bottom = draw_data.display_y + draw_data.display_height + 0.5f;
  plan.projection = {2.0f / (right - left), 0, 0, 0,
                     0, 2.0f / (top - bottom), 0, 0,
                     0, 0, 0.5f, 0,
                     (left + right) / (left - right),
                     (top + bottom) / (bottom - top), 0.5f, 1.0f};
  return plan;
}

void SetupXbox360RenderState(const DrawData& draw_data,
                             BackendServices* services) {
  if (services) services->ApplyRenderState(BuildRenderStatePlan(draw_data));
}

bool RenderXbox360DrawData(BackendState* state, const DrawData& draw_data,
                           BackendServices* services,
                           std::vector<XboxVertex>* uploaded_vertices,
                           std::vector<std::uint16_t>* uploaded_indices) {
  if (!state || !services || !uploaded_vertices || !uploaded_indices ||
      draw_data.display_width <= 0.0f || draw_data.display_height <= 0.0f)
    return false;

  int total_vertices = 0;
  int total_indices = 0;
  for (const auto& list : draw_data.lists) {
    total_vertices += static_cast<int>(list.vertices.size());
    total_indices += static_cast<int>(list.indices.size());
  }
  if (!state->vertex_buffer || state->vertex_capacity < total_vertices) {
    state->vertex_capacity = total_vertices + 5000;
    if (!services->EnsureVertexBuffer(state->vertex_capacity)) return false;
    state->vertex_buffer = true;
  }
  if (!state->index_buffer || state->index_capacity < total_indices) {
    state->index_capacity = total_indices + 10000;
    if (!services->EnsureIndexBuffer(state->index_capacity)) return false;
    state->index_buffer = true;
  }

  CaptureRenderStateTable(state, services);
  uploaded_vertices->clear();
  uploaded_indices->clear();
  for (const auto& list : draw_data.lists) {
    for (const auto& source : list.vertices)
      uploaded_vertices->push_back({source.x, source.y, 0.0f,
                                    RgbaToBgra(source.color), source.u, source.v});
    uploaded_indices->insert(uploaded_indices->end(), list.indices.begin(),
                             list.indices.end());
  }
  SetupXbox360RenderState(draw_data, services);

  std::uint32_t global_vertex = 0;
  std::uint32_t global_index = 0;
  for (std::size_t list_index = 0; list_index < draw_data.lists.size(); ++list_index) {
    const auto& list = draw_data.lists[list_index];
    for (std::size_t command_index = 0; command_index < list.commands.size(); ++command_index) {
      const auto& command = list.commands[command_index];
      if (command.user_callback) {
        if (command.reset_render_state)
          SetupXbox360RenderState(draw_data, services);
        else
          services->InvokeUserCallback(list_index, command_index);
        continue;
      }
      const float x1 = command.clip[0] - draw_data.display_x;
      const float y1 = command.clip[1] - draw_data.display_y;
      const float x2 = command.clip[2] - draw_data.display_x;
      const float y2 = command.clip[3] - draw_data.display_y;
      if (x2 <= x1 || y2 <= y1) continue;
      services->Draw({{static_cast<std::int32_t>(x1), static_cast<std::int32_t>(y1),
                       static_cast<std::int32_t>(x2), static_cast<std::int32_t>(y2)},
                      command.texture_id, command.vertex_offset + global_vertex,
                      command.index_offset + global_index,
                      static_cast<std::uint32_t>(list.vertices.size()),
                      command.element_count / 3});
    }
    global_vertex += static_cast<std::uint32_t>(list.vertices.size());
    global_index += static_cast<std::uint32_t>(list.indices.size());
  }
  RestoreRenderStateTable(*state, services);
  return true;
}

}  // namespace bandwagon::boot_edit::xbox_backend
