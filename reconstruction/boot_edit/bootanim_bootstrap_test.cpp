#include "bootanim_bootstrap.hpp"

#include <algorithm>
#include <cassert>
#include <string>
#include <utility>
#include <vector>

using namespace bandwagon::boot_edit;

int main() {
  std::vector<std::string> events;
  BootAnimationServices services;
  services.initialize_runtime = [&] { events.push_back("init"); };
  services.translate_symbolic_link = [&](std::string_view link) {
    assert(link == kPeerRootLink);
    events.push_back("translate");
    return std::string("\\Device\\Harddisk0\\Partition1\\PeerPressure");
  };
  services.replace_symbolic_link = [&](std::string_view link,
                                       std::string_view target) {
    assert(link == kThemesLink);
    assert(target ==
           "\\Device\\Harddisk0\\Partition1\\PeerPressure\\Themes\\BootAnim\\9199");
    events.push_back("link");
  };
  services.load_image = [&](std::string_view path, std::uint32_t flags,
                            ImageHandle* image) {
    assert(path == kBootAnimationImage);
    assert(flags == 9);
    *image = 0x1234;
    events.push_back("load");
    return 0;
  };
  services.prepare_hooks = [&] { events.push_back("prepare"); };
  services.create_thread = [&](ImageHandle image, std::uint32_t flags) {
    assert(image == 0x1234);
    assert(flags == 0x020000A0);
    events.push_back("thread");
    return 0;
  };
  services.unload_image = [&](ImageHandle) { events.push_back("unload"); };
  services.debug_print = [&](std::string_view message, std::uint32_t status) {
    assert(message == "Failed to load boot animation (0x%08x)\n");
    assert(status == 0);
    events.push_back("log");
  };
  assert(BootstrapBootAnimation(services) == 0);
  assert((events == std::vector<std::string>{"init", "translate", "link",
                                              "load", "prepare", "thread",
                                              "log"}));

  events.clear();
  services.load_image = [&](std::string_view, std::uint32_t, ImageHandle*) {
    events.push_back("load-fail");
    return 0xC0000001u;
  };
  services.debug_print = [&](std::string_view message, std::uint32_t status) {
    assert(message == "Failed to load boot animation (0x%08x)\n");
    assert(status == 0xC0000001u);
    events.push_back("load-log");
  };
  BootstrapBootAnimation(services);
  assert(events.back() == "load-log");
  assert(std::find(events.begin(), events.end(), "prepare") == events.end());

  events.clear();
  services.load_image = [&](std::string_view, std::uint32_t,
                            ImageHandle* image) {
    *image = 0x5678;
    return 0;
  };
  services.create_thread = [&](ImageHandle, std::uint32_t) { return 0x55; };
  services.unload_image = [&](ImageHandle image) {
    assert(image == 0x5678);
    events.push_back("unload");
  };
  services.debug_print = [&](std::string_view message, std::uint32_t status) {
    if (message == "Error creating boot animation thread (0x%08x)\n") {
      assert(status == 0x55);
      events.push_back("thread-log");
    } else {
      assert(status == 0x55);
      events.push_back("load-log");
    }
  };
  BootstrapBootAnimation(services);
  assert((events == std::vector<std::string>{"init", "translate", "link",
                                              "prepare", "unload", "thread-log",
                                              "load-log"}));

  bool started = false;
  RunBootAnimationThread(
      0x4321, 0x8765,
      [&](ImageHandle image, std::uint32_t ordinal,
          BootAnimationStart* start) {
        assert(image == 0x4321);
        assert(ordinal == 1);
        *start = [&](ImageHandle start_image, std::uintptr_t state) {
          assert(start_image == 0x4321);
          assert(state == 0x8765);
          started = true;
        };
        return 0;
      },
      {});
  assert(started);

  bool resolve_logged = false;
  RunBootAnimationThread(
      1, 2,
      [](ImageHandle, std::uint32_t, BootAnimationStart*) { return 0x22; },
      [&](std::string_view message, std::uint32_t status) {
        assert(message ==
               "Failed to resolve bootanim start routine (0x%08x)\n");
        assert(status == 0x22);
        resolve_logged = true;
      });
  assert(resolve_logged);

  ModalCompletionState modal;
  int callbacks = 0;
  modal.continuation_argument = 7;
  modal.continuation = [&](ModalCompletionState* self, std::uintptr_t argument) {
    assert(self == &modal);
    assert(argument == 7);
    ++callbacks;
  };
  CompleteSingleChoiceModal(1, &modal);
  assert(!modal.dialog_open && !modal.completion_pending && callbacks == 0);
  modal.dialog_open = true;
  CompleteSingleChoiceModal(0, &modal);
  assert(modal.result == 1 && modal.completion_pending && callbacks == 1);
  modal = ModalCompletionState{};
  modal.continuation = [&](ModalCompletionState*, std::uintptr_t) { ++callbacks; };
  CompleteThreeChoiceModal(2, &modal);
  assert(modal.result == 3 && modal.completion_pending && callbacks == 2);
  modal = ModalCompletionState{};
  CompleteThreeChoiceModal(3, &modal);
  assert(!modal.dialog_open && !modal.completion_pending);
  modal = ModalCompletionState{};
  CompleteThreeChoiceModal(9, &modal);
  assert(modal.result == 1 && modal.completion_pending);
}
