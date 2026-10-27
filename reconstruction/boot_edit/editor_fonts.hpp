#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string_view>

namespace bandwagon::boot_edit {

inline constexpr std::string_view kUiResourcePack = "UIDATA";
inline constexpr std::string_view kRegularFontPath =
    "Fonts\\NotoSans17.ttf";
inline constexpr std::string_view kSemiboldFontPath =
    "Fonts\\NotoSans-SemiBold.ttf";

inline constexpr std::array<std::string_view, 10> kEditorIconPaths = {{
    "Icons\\X360\\Light\\A", "Icons\\X360\\Light\\B",
    "Icons\\X360\\Light\\X", "Icons\\X360\\Light\\Y",
    "Icons\\X360\\Light\\Start", "Icons\\X360\\Light\\Back",
    "Icons\\X360\\Light\\LeftBumper",
    "Icons\\X360\\Light\\LeftTrigger", "Icons\\X360\\Light\\L3",
    "Icons\\X360\\Light\\R3",
}};

using ResourcePackHandle = std::uintptr_t;
using ResourceData = const void*;
using FontHandle = std::uintptr_t;
using GlyphHandle = std::uintptr_t;
using GlyphRangesHandle = std::uintptr_t;

struct EditorIcon {
  std::uint32_t flags = 0x00030001;
  std::uint32_t reserved = 0;
  std::int32_t minimum = -1;
  std::int32_t maximum = -1;
  std::array<std::uint8_t, 24> descriptor{};
  bool loaded = false;
};

struct FontRequest {
  ResourceData data = nullptr;
  std::uint32_t data_size = 0;
  float pixel_size = 0.0f;
  GlyphRangesHandle glyph_ranges = 0;
  bool use_custom_config = false;
  int oversample_horizontal = 0;
  int oversample_vertical = 0;
  bool pixel_snap_horizontal = false;
};

struct EditorFontState {
  ResourcePackHandle resource_pack = 0;
  std::array<EditorIcon, 10> icons{};
  FontHandle default_font = 0;
  FontHandle large_regular_font = 0;
  FontHandle regular_font = 0;
  FontHandle semibold_font = 0;
  std::uintptr_t renderer_state = 0;
};

enum class EditorFontLoadResult {
  kLoaded,
  kRuntimeInitializationFailed,
  kResourcePackFailed,
  kRegularFontMissing,
  kSemiboldFontMissing,
};

struct EditorFontServices {
  std::function<bool()> initialize_runtime;
  std::function<ResourcePackHandle(std::string_view)> open_resource_pack;
  std::function<bool(ResourcePackHandle, std::string_view,
                     std::array<std::uint8_t, 24>*)>
      load_icon;
  std::function<std::uint32_t(ResourcePackHandle, std::string_view)>
      resource_size;
  std::function<ResourceData(ResourcePackHandle, std::string_view)>
      resource_data;
  std::function<void(std::string_view, std::uint32_t)> debug_print;
  std::function<GlyphRangesHandle()> default_glyph_ranges;
  std::function<FontHandle(const FontRequest&)> add_font_from_memory;
  std::function<FontHandle()> add_default_font;
  std::function<void()> build_font_atlas;
  std::function<GlyphHandle(FontHandle, std::uint32_t)> find_glyph;
  std::function<void(GlyphHandle, float)> add_glyph_advance;
  std::function<void(FontHandle)> set_current_font;
  std::function<void(std::uint32_t, float)> set_style_scalar;
  std::function<void(std::uint32_t, std::uint32_t)> set_style_color;
  std::function<std::uintptr_t()> acquire_renderer_state;
};

// Readable behavioral model of the complete 0x82050f60 inventory extent.
EditorFontLoadResult LoadEditorFonts(EditorFontState* state,
                                     const EditorFontServices& services);

}  // namespace bandwagon::boot_edit
