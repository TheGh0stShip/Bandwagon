#include "bootanim_bootstrap.hpp"

namespace bandwagon::boot_edit {
namespace {

constexpr std::string_view kLoadDiagnostic =
    "Failed to load boot animation (0x%08x)\n";
constexpr std::string_view kThreadDiagnostic =
    "Error creating boot animation thread (0x%08x)\n";
constexpr std::string_view kResolveDiagnostic =
    "Failed to resolve bootanim start routine (0x%08x)\n";

void PublishModalResult(ModalCompletionState* modal, std::uint32_t result) {
  modal->result = result;
  modal->completion_pending = true;
  if (modal->continuation) {
    modal->continuation(modal, modal->continuation_argument);
  }
}

}  // namespace

ImageHandle ResolveKernelBootAnimationRoutine(
    const std::function<std::uint32_t(std::string_view, ImageHandle*)>&
        get_module_handle,
    const std::function<std::uint32_t(ImageHandle, std::uint32_t,
                                      ImageHandle*)>& get_procedure) {
  if (!get_module_handle || !get_procedure) return 0;
  ImageHandle kernel = 0;
  if (get_module_handle("xboxkrnl.exe", &kernel) != 0) return 0;
  ImageHandle routine = 0;
  if (get_procedure(kernel, kKernelBootAnimationOrdinal, &routine) != 0) {
    return 0;
  }
  return routine;
}

std::uint32_t BootstrapBootAnimation(const BootAnimationServices& services) {
  if (services.initialize_runtime) services.initialize_runtime();

  std::string peer_root;
  if (services.translate_symbolic_link) {
    peer_root = services.translate_symbolic_link(kPeerRootLink);
  }
  const std::string themes_target =
      peer_root + std::string(kBootAnimationDirectory);
  if (services.replace_symbolic_link) {
    services.replace_symbolic_link(kThemesLink, themes_target);
  }

  ImageHandle image = kInvalidImageHandle;
  std::uint32_t status = 0;
  if (services.load_image) {
    status = services.load_image(kBootAnimationImage, kBootAnimationLoadFlags,
                                 &image);
  }

  if (status == 0 && image != kInvalidImageHandle) {
    if (services.prepare_hooks) services.prepare_hooks();
    const std::uint32_t thread_status = services.create_thread
                                            ? services.create_thread(
                                                  image,
                                                  kBootAnimationThreadFlags)
                                            : 1;
    if (thread_status != 0) {
      if (services.unload_image) services.unload_image(image);
      if (services.debug_print) {
        services.debug_print(kThreadDiagnostic, thread_status);
      }
      status = thread_status;
    }
  }

  // This diagnostic is reached by every path in the original extent. On a
  // successful ExCreateThread call its variadic argument lives in a volatile
  // register, so the portable model supplies the last defined status (zero).
  if (services.debug_print) services.debug_print(kLoadDiagnostic, status);
  return 0;
}

void RunBootAnimationThread(
    ImageHandle image, std::uintptr_t shared_state,
    const std::function<std::uint32_t(ImageHandle, std::uint32_t,
                                      BootAnimationStart*)>& get_procedure,
    const std::function<void(std::string_view, std::uint32_t)>& debug_print) {
  BootAnimationStart start;
  const std::uint32_t status =
      get_procedure ? get_procedure(image, kBootAnimationStartOrdinal, &start)
                    : 1;
  if (status != 0) {
    if (debug_print) debug_print(kResolveDiagnostic, status);
    return;
  }
  if (start) start(image, shared_state);
}

void CompleteSingleChoiceModal(std::uint32_t selected_state,
                               ModalCompletionState* modal) {
  if (!modal) return;
  if (selected_state == 1) {
    modal->dialog_open = false;
    return;
  }
  PublishModalResult(modal, 1);
}

void CompleteThreeChoiceModal(std::uint32_t selected_state,
                              ModalCompletionState* modal) {
  if (!modal) return;
  if (selected_state == 2) {
    PublishModalResult(modal, 3);
  } else if (selected_state == 3) {
    modal->dialog_open = false;
  } else {
    PublishModalResult(modal, 1);
  }
}

}  // namespace bandwagon::boot_edit
