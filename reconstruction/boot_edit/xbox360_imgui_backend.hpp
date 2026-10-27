#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace bandwagon::boot_edit::xbox_backend {

inline constexpr std::int32_t kOutOfMemory =
    static_cast<std::int32_t>(0x8007000eu);
inline constexpr std::size_t kRenderStateCount = 97;
inline constexpr std::array<int, 22> kKeyMap{
    9, 37, 39, 38, 40, 33, 34, 36, 35, 45, 46,
    8, 32, 13, 27, 13, 65, 67, 86, 88, 89, 90};

enum class ShaderObject { kVertexDeclaration, kVertexShader, kPixelShader };

struct BackendState {
  bool initialized = false;
  bool vertex_declaration = false;
  bool vertex_shader = false;
  bool pixel_shader = false;
  bool font_texture = false;
  bool vertex_buffer = false;
  bool index_buffer = false;
  std::uint32_t font_texture_id = 0;
  int vertex_capacity = 5000;
  int index_capacity = 10000;
  float display_width = 0.0f;
  float display_height = 0.0f;
  std::uint32_t backend_flags = 0;
  std::string_view platform_name;
  std::string_view renderer_name;
  std::array<int, 22> key_map{};
  std::array<std::uint32_t, kRenderStateCount> saved_render_state{};
  std::array<float, 24> navigation_inputs{};
};

struct ControllerState {
  bool available = false;
  // Nav slots 0,1,2,4,5,6,7,16,17,18,19 in that exact order.
  std::array<bool, 11> digital{};
  std::int16_t left_x = 0;
  std::int16_t left_y = 0;
  std::int16_t right_x = 0;
  std::int16_t right_y = 0;
};

struct FontAtlas {
  int width = 0;
  int height = 0;
  int bytes_per_pixel = 4;
  bool uses_color_pixels = false;
  std::vector<std::uint8_t> rgba_pixels;
};

struct RenderVertex {
  float x = 0.0f;
  float y = 0.0f;
  std::uint32_t color = 0;
  float u = 0.0f;
  float v = 0.0f;
};

struct XboxVertex {
  float x = 0.0f;
  float y = 0.0f;
  float z = 0.0f;
  std::uint32_t color = 0;
  float u = 0.0f;
  float v = 0.0f;
};

struct DrawCommand {
  std::array<float, 4> clip{};
  std::uint32_t texture_id = 0;
  std::uint32_t element_count = 0;
  std::uint32_t vertex_offset = 0;
  std::uint32_t index_offset = 0;
  bool reset_render_state = false;
  bool user_callback = false;
};

struct DrawList {
  std::vector<RenderVertex> vertices;
  std::vector<std::uint16_t> indices;
  std::vector<DrawCommand> commands;
};

struct DrawData {
  float display_width = 0.0f;
  float display_height = 0.0f;
  float display_x = 0.0f;
  float display_y = 0.0f;
  std::vector<DrawList> lists;
};

struct RenderStatePlan {
  int viewport_width = 0;
  int viewport_height = 0;
  std::array<float, 16> projection{};
  bool alpha_blend = true;
  bool depth_write = false;
  bool depth_test = false;
  bool cull = false;
  bool scissor = true;
  bool linear_filter = true;
};

struct DrawCall {
  std::array<std::int32_t, 4> scissor{};
  std::uint32_t texture_id = 0;
  std::uint32_t base_vertex = 0;
  std::uint32_t start_index = 0;
  std::uint32_t vertex_count = 0;
  std::uint32_t primitive_count = 0;
};

struct BackendServices {
  virtual ~BackendServices() = default;
  virtual bool CreateShaderObject(ShaderObject object) = 0;
  virtual std::uint32_t GetRenderState(std::size_t index) = 0;
  virtual void SetRenderState(std::size_t index, std::uint32_t value) = 0;
  virtual bool UploadTexture(int width, int height,
                             const std::vector<std::uint8_t>& bgra,
                             std::uint32_t* texture_id) = 0;
  virtual bool EnsureVertexBuffer(int capacity) = 0;
  virtual bool EnsureIndexBuffer(int capacity) = 0;
  virtual void ApplyRenderState(const RenderStatePlan& plan) = 0;
  virtual void InvokeUserCallback(std::size_t list_index,
                                  std::size_t command_index) = 0;
  virtual void Draw(const DrawCall& call) = 0;
};

std::int32_t CreateVertexDeclaration(BackendState* state,
                                     BackendServices* services);
std::int32_t CreateVertexShader(BackendState* state,
                                BackendServices* services);
std::int32_t CreatePixelShader(BackendState* state,
                               BackendServices* services);
void CaptureRenderStateTable(BackendState* state, BackendServices* services);
void CaptureRenderStateSlot(BackendState* state, std::size_t index,
                            BackendServices* services);
void RestoreRenderStateTable(const BackendState& state,
                             BackendServices* services);
bool CreateFontsTexture(BackendState* state, const FontAtlas& atlas,
                        BackendServices* services);
bool InitializeXbox360Backend(BackendState* state, float display_width,
                              float display_height,
                              BackendServices* services);
bool Xbox360BackendNewFrame(BackendState* state,
                            const ControllerState& controller,
                            const FontAtlas& atlas,
                            BackendServices* services);
RenderStatePlan BuildRenderStatePlan(const DrawData& draw_data);
void SetupXbox360RenderState(const DrawData& draw_data,
                             BackendServices* services);
bool RenderXbox360DrawData(BackendState* state, const DrawData& draw_data,
                           BackendServices* services,
                           std::vector<XboxVertex>* uploaded_vertices,
                           std::vector<std::uint16_t>* uploaded_indices);

}  // namespace bandwagon::boot_edit::xbox_backend
