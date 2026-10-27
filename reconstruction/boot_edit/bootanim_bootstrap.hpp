#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <string_view>

namespace bandwagon::boot_edit {

inline constexpr std::string_view kPeerRootLink = "\\??\\PPROOT:";
inline constexpr std::string_view kThemesLink = "\\??\\Themes:";
inline constexpr std::string_view kBootAnimationDirectory =
    "\\Themes\\BootAnim\\9199";
inline constexpr std::string_view kBootAnimationImage =
    "PPROOT:\\Themes\\BootAnim\\9199\\bootanim.xex";
inline constexpr std::uint32_t kBootAnimationLoadFlags = 9;
inline constexpr std::uint32_t kBootAnimationThreadFlags = 0x020000A0;
inline constexpr std::uint32_t kBootAnimationStartOrdinal = 1;

using ImageHandle = std::uintptr_t;
inline constexpr ImageHandle kInvalidImageHandle =
    static_cast<ImageHandle>(-1);

struct BootAnimationServices {
  std::function<void()> initialize_runtime;
  std::function<std::string(std::string_view)> translate_symbolic_link;
  std::function<void(std::string_view, std::string_view)> replace_symbolic_link;
  std::function<std::uint32_t(std::string_view, std::uint32_t, ImageHandle*)>
      load_image;
  std::function<void()> prepare_hooks;
  std::function<std::uint32_t(ImageHandle, std::uint32_t)> create_thread;
  std::function<void(ImageHandle)> unload_image;
  std::function<void(std::string_view, std::uint32_t)> debug_print;
};

// Readable model of the complete 0x820110d0 inventory extent. The function
// returns zero on every path, matching the target.
std::uint32_t BootstrapBootAnimation(const BootAnimationServices& services);

using BootAnimationStart = std::function<void(ImageHandle, std::uintptr_t)>;

// Model of the thread entry at 0x82011068. The XEX procedure lookup is an
// injected boundary; ordinal 1 is the loaded bootanim image's start routine.
void RunBootAnimationThread(
    ImageHandle image, std::uintptr_t shared_state,
    const std::function<std::uint32_t(ImageHandle, std::uint32_t,
                                      BootAnimationStart*)>& get_procedure,
    const std::function<void(std::string_view, std::uint32_t)>& debug_print);

struct ModalCompletionState {
  bool completion_pending = false;  // byte +4
  std::uint32_t result = 0;         // word +8
  bool dialog_open = true;          // byte +304
  std::uintptr_t continuation_argument = 0;
  std::function<void(ModalCompletionState*, std::uintptr_t)> continuation;
};

// The two non-pdata leaves at 0x82011228 and 0x82011268 are part of the same
// inventory extent and update the shared modal completion object.
void CompleteSingleChoiceModal(std::uint32_t selected_state,
                               ModalCompletionState* modal);
void CompleteThreeChoiceModal(std::uint32_t selected_state,
                              ModalCompletionState* modal);

}  // namespace bandwagon::boot_edit
