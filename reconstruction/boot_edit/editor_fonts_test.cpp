#include "editor_fonts.hpp"

#include <cassert>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

using namespace bandwagon::boot_edit;

namespace {

struct Harness {
  bool runtime_ok = true;
  ResourcePackHandle pack = 7;
  bool regular_present = true;
  bool semibold_present = true;
  std::size_t missing_icon = 99;
  std::vector<std::string> events;
  std::vector<FontRequest> fonts;

  EditorFontServices Services() {
    EditorFontServices s;
    s.initialize_runtime = [this] {
      events.push_back("runtime");
      return runtime_ok;
    };
    s.open_resource_pack = [this](std::string_view name) {
      events.push_back("pack:" + std::string(name));
      return pack;
    };
    s.load_icon = [this](ResourcePackHandle, std::string_view path,
                         std::array<std::uint8_t, 24>* descriptor) {
      const std::size_t index = events.size() - 2;
      events.push_back("icon:" + std::string(path));
      if (index == missing_icon) return false;
      descriptor->fill(static_cast<std::uint8_t>(index + 1));
      return true;
    };
    s.resource_size = [](ResourcePackHandle, std::string_view path) {
      return path == kRegularFontPath ? 101u : 202u;
    };
    s.resource_data = [this](ResourcePackHandle, std::string_view path) {
      if (path == kRegularFontPath)
        return regular_present ? reinterpret_cast<ResourceData>(0x1000)
                               : nullptr;
      return semibold_present ? reinterpret_cast<ResourceData>(0x2000)
                              : nullptr;
    };
    s.debug_print = [this](std::string_view text, std::uint32_t value) {
      events.push_back("print:" + std::to_string(value) + ":" +
                       std::string(text));
    };
    s.default_glyph_ranges = [this] {
      events.push_back("ranges");
      return GlyphRangesHandle{55};
    };
    s.add_font_from_memory = [this](const FontRequest& request) {
      fonts.push_back(request);
      events.push_back("font");
      return FontHandle{100 + fonts.size()};
    };
    s.add_default_font = [this] {
      events.push_back("default");
      return FontHandle{200};
    };
    s.build_font_atlas = [this] { events.push_back("build"); };
    s.find_glyph = [this](FontHandle font, std::uint32_t codepoint) {
      assert(font == 101 && codepoint == 32);
      events.push_back("glyph");
      return GlyphHandle{300};
    };
    s.add_glyph_advance = [this](GlyphHandle glyph, float amount) {
      assert(glyph == 300 && amount == 2.0f);
      events.push_back("advance");
    };
    s.set_current_font = [this](FontHandle font) {
      assert(font == 101);
      events.push_back("current");
    };
    s.set_style_scalar = [this](std::uint32_t index, float value) {
      assert(index == 3 && value == 8.0f);
      events.push_back("scalar");
    };
    s.set_style_color = [this](std::uint32_t index, std::uint32_t value) {
      events.push_back("color:" + std::to_string(index) + ":" +
                       std::to_string(value));
    };
    s.acquire_renderer_state = [this] {
      events.push_back("renderer");
      return std::uintptr_t{400};
    };
    return s;
  }
};

void TestSuccessAndExactConfiguration() {
  Harness h;
  h.missing_icon = 3;
  EditorFontState state;
  assert(LoadEditorFonts(&state, h.Services()) ==
         EditorFontLoadResult::kLoaded);
  assert(state.resource_pack == 7);
  assert(state.icons[0].loaded && !state.icons[3].loaded &&
         state.icons[9].loaded);
  assert(state.icons[0].flags == 0x00030001);
  assert(state.icons[0].minimum == -1 && state.icons[0].maximum == -1);
  assert(h.fonts.size() == 3);
  assert(h.fonts[0].data == reinterpret_cast<ResourceData>(0x1000));
  assert(h.fonts[0].data_size == 101 && h.fonts[0].pixel_size == 22.0f);
  assert(h.fonts[0].glyph_ranges == 55 && h.fonts[0].use_custom_config);
  assert(h.fonts[0].oversample_horizontal == 3 &&
         h.fonts[0].oversample_vertical == 3 &&
         h.fonts[0].pixel_snap_horizontal);
  assert(h.fonts[1].pixel_size == 17.0f &&
         !h.fonts[1].use_custom_config);
  assert(h.fonts[2].data == reinterpret_cast<ResourceData>(0x2000));
  assert(h.fonts[2].data_size == 202 && h.fonts[2].pixel_size == 23.0f);
  assert(state.large_regular_font == 101 && state.regular_font == 102 &&
         state.semibold_font == 103 && state.default_font == 200);
  assert(state.renderer_state == 400);
  assert(h.events.back() == "renderer");
}

void TestFatalBranches() {
  EditorFontState state;
  Harness runtime;
  runtime.runtime_ok = false;
  assert(LoadEditorFonts(&state, runtime.Services()) ==
         EditorFontLoadResult::kRuntimeInitializationFailed);
  assert(runtime.events.size() == 1);

  Harness pack;
  pack.pack = 0;
  assert(LoadEditorFonts(&state, pack.Services()) ==
         EditorFontLoadResult::kResourcePackFailed);
  assert(pack.events.back().find("failed to load ui resource pack") !=
         std::string::npos);

  Harness regular;
  regular.regular_present = false;
  assert(LoadEditorFonts(&state, regular.Services()) ==
         EditorFontLoadResult::kRegularFontMissing);
  assert(regular.fonts.empty());

  Harness semibold;
  semibold.semibold_present = false;
  assert(LoadEditorFonts(&state, semibold.Services()) ==
         EditorFontLoadResult::kSemiboldFontMissing);
  assert(semibold.fonts.empty());
}

}  // namespace

int main() {
  TestSuccessAndExactConfiguration();
  TestFatalBranches();
}
