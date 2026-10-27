#include "editor_fonts.hpp"

namespace bandwagon::boot_edit {
namespace {

constexpr std::string_view kPackFailure =
    "UserInterface::Initialize: failed to load ui resource pack\n";
constexpr std::string_view kTextureFailure =
    "UserInterface::Initialize: failed to get texture %d from resource pack\n";
constexpr std::string_view kFontFailure =
    "UserInterface::Initialize: failed to load fonts from ui resource pack\n";

void Print(const EditorFontServices& services, std::string_view text,
           std::uint32_t value = 0) {
  if (services.debug_print) services.debug_print(text, value);
}

}  // namespace

EditorFontLoadResult LoadEditorFonts(EditorFontState* state,
                                     const EditorFontServices& services) {
  if (!state) return EditorFontLoadResult::kRuntimeInitializationFailed;
  if (!services.initialize_runtime || !services.initialize_runtime()) {
    return EditorFontLoadResult::kRuntimeInitializationFailed;
  }

  state->resource_pack = services.open_resource_pack
                             ? services.open_resource_pack(kUiResourcePack)
                             : 0;
  if (state->resource_pack == 0) {
    Print(services, kPackFailure);
    return EditorFontLoadResult::kResourcePackFailed;
  }

  for (std::size_t i = 0; i < kEditorIconPaths.size(); ++i) {
    EditorIcon icon;
    if (services.load_icon &&
        services.load_icon(state->resource_pack, kEditorIconPaths[i],
                           &icon.descriptor)) {
      icon.loaded = true;
      state->icons[i] = icon;
    } else {
      Print(services, kTextureFailure, static_cast<std::uint32_t>(i));
    }
  }

  const std::uint32_t regular_size =
      services.resource_size
          ? services.resource_size(state->resource_pack, kRegularFontPath)
          : 0;
  const ResourceData regular_data =
      services.resource_data
          ? services.resource_data(state->resource_pack, kRegularFontPath)
          : nullptr;
  const std::uint32_t semibold_size =
      services.resource_size
          ? services.resource_size(state->resource_pack, kSemiboldFontPath)
          : 0;
  const ResourceData semibold_data =
      services.resource_data
          ? services.resource_data(state->resource_pack, kSemiboldFontPath)
          : nullptr;

  // The target tests the returned data pointers. Size is passed through to
  // ImGui, but is not part of its missing-resource branch.
  if (!regular_data) {
    Print(services, kFontFailure);
    return EditorFontLoadResult::kRegularFontMissing;
  }
  if (!semibold_data) {
    Print(services, kFontFailure);
    return EditorFontLoadResult::kSemiboldFontMissing;
  }

  const GlyphRangesHandle ranges =
      services.default_glyph_ranges ? services.default_glyph_ranges() : 0;
  const auto add_font = [&](ResourceData data, std::uint32_t size,
                            float pixels, bool custom) {
    FontRequest request{data, size, pixels, ranges, custom};
    if (custom) {
      request.oversample_horizontal = 3;
      request.oversample_vertical = 3;
      request.pixel_snap_horizontal = true;
    }
    return services.add_font_from_memory
               ? services.add_font_from_memory(request)
               : FontHandle{0};
  };

  state->large_regular_font = add_font(regular_data, regular_size, 22.0f, true);
  state->regular_font = add_font(regular_data, regular_size, 17.0f, false);
  state->semibold_font =
      add_font(semibold_data, semibold_size, 23.0f, false);
  state->default_font = services.add_default_font ? services.add_default_font()
                                                  : FontHandle{0};
  if (services.build_font_atlas) services.build_font_atlas();

  const GlyphHandle space = services.find_glyph
                                ? services.find_glyph(
                                      state->large_regular_font, 32)
                                : GlyphHandle{0};
  if (space && services.add_glyph_advance) {
    services.add_glyph_advance(space, 2.0f);
  }
  if (services.set_current_font) {
    services.set_current_font(state->large_regular_font);
  }

  if (services.set_style_scalar) services.set_style_scalar(3, 8.0f);
  if (services.set_style_color) {
    services.set_style_color(2, 0xB30B0001);
    services.set_style_color(5, 0xA5000000);
  }
  state->renderer_state = services.acquire_renderer_state
                              ? services.acquire_renderer_state()
                              : 0;
  return EditorFontLoadResult::kLoaded;
}

}  // namespace bandwagon::boot_edit
