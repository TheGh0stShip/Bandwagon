#include "theme_name_task.hpp"

#include <cassert>
#include <string>
#include <vector>

using namespace bandwagon::boot_edit;

namespace {

struct Harness {
  std::uint32_t start_status = kAsyncPending;
  KeyboardPollResult poll;
  std::optional<std::uint32_t> dialog_result;
  bool exists = false;
  std::vector<std::string> events;

  ThemeNameTaskServices Services() {
    ThemeNameTaskServices s;
    s.poll_dialog = [this](ThemeNameDialog) { return dialog_result; };
    s.start_keyboard = [this](std::string_view title, std::string_view prompt,
                              std::string_view current) {
      events.push_back(std::string(title) + ":" + std::string(prompt) + ":" +
                       std::string(current));
      return start_status;
    };
    s.poll_keyboard = [this] { return poll; };
    s.narrow = [](std::wstring_view value) {
      return std::string(value.begin(), value.end());
    };
    s.file_exists = [this](std::string_view path) {
      events.push_back("exists:" + std::string(path));
      return exists;
    };
    s.show_dialog = [this](ThemeNameDialog, std::string_view message,
                           std::string_view title) {
      events.push_back("dialog:" + std::string(title) + ":" +
                       std::string(message));
    };
    s.debug_print = [this](std::string_view text, std::uint32_t status) {
      events.push_back("debug:" + std::to_string(status) + ":" +
                       std::string(text));
    };
    return s;
  }
};

void Start(ThemeNameTaskState* state, Harness* h) {
  assert(!AdvanceThemeNameTask(state, "Old", h->Services()));
  assert(state->keyboard_in_progress);
}

void TestStartAndPollFailures() {
  ThemeNameTaskState state;
  Harness start;
  start.start_status = 5;
  assert(AdvanceThemeNameTask(&state, "Old", start.Services()));
  assert(state.result == 2 && state.completion_pending);

  state = {};
  Harness poll;
  Start(&state, &poll);
  poll.poll.status = 9;
  assert(AdvanceThemeNameTask(&state, "Old", poll.Services()));
  assert(state.result == 2);
}

void TestKeyboardOutcomesAndContinuation() {
  ThemeNameTaskState state;
  int continued = 0;
  state.continuation = [&continued](ThemeNameTaskState*) { ++continued; };
  Harness h;
  Start(&state, &h);
  h.poll = {0, kOperationCancelled, L""};
  assert(AdvanceThemeNameTask(&state, "Old", h.Services()));
  assert(state.result == 1 && continued == 1);

  state = {};
  Harness error;
  Start(&state, &error);
  error.poll = {0, 44, L""};
  assert(AdvanceThemeNameTask(&state, "Old", error.Services()));
  assert(state.result == 2);
}

void TestInvalidRetryAndCancel() {
  ThemeNameTaskState state;
  Harness h;
  Start(&state, &h);
  h.poll = {0, 0, L""};
  assert(!AdvanceThemeNameTask(&state, "Old", h.Services()));
  assert(state.dialog == ThemeNameDialog::kInvalidName);
  h.dialog_result = 1;
  assert(!AdvanceThemeNameTask(&state, "Old", h.Services()));
  assert(!state.keyboard_in_progress && state.dialog == ThemeNameDialog::kNone);

  state.dialog = ThemeNameDialog::kInvalidName;
  h.dialog_result = 4;
  assert(AdvanceThemeNameTask(&state, "Old", h.Services()));
  assert(state.result == 1);
}

void TestAvailableAndOverwriteNames() {
  ThemeNameTaskState state;
  Harness h;
  Start(&state, &h);
  h.poll = {0, 0, L"Blue"};
  assert(AdvanceThemeNameTask(&state, "Old", h.Services()));
  assert(state.result == 3 && state.name == "Blue" &&
         state.path == "Themes:\\Blue.xbat");

  state = {};
  Harness overwrite;
  overwrite.exists = true;
  Start(&state, &overwrite);
  overwrite.poll = {0, 0, L"Blue"};
  assert(!AdvanceThemeNameTask(&state, "Old", overwrite.Services()));
  assert(state.dialog == ThemeNameDialog::kOverwrite);
  overwrite.dialog_result = 3;
  assert(!AdvanceThemeNameTask(&state, "Old", overwrite.Services()));
  assert(!state.keyboard_in_progress);

  state.dialog = ThemeNameDialog::kOverwrite;
  overwrite.dialog_result = 2;
  assert(AdvanceThemeNameTask(&state, "Old", overwrite.Services()));
  assert(state.result == 3);
}

void TestEmbeddedRectHelpers() {
  const Rect outer{{0, 0}, {10, 10}};
  const Rect inner{{2, 2}, {8, 8}};
  const Rect crossing{{8, 8}, {12, 12}};
  assert(RectContains(outer, inner));
  assert(!RectContains(inner, outer));
  assert(RectOverlaps(outer, crossing));
  Rect value{{-1, 2}, {12, 8}};
  ClipRect(&value, outer);
  assert(value.minimum.x == 0 && value.minimum.y == 2 &&
         value.maximum.x == 10 && value.maximum.y == 8);
  Rect copied{};
  CopyRectIfDifferent(&copied, inner);
  assert(RectContains(copied, inner) && RectContains(inner, copied));
  CopyRect(&copied, outer);
  assert(RectContains(copied, outer) && RectContains(outer, copied));
}

}  // namespace

int main() {
  TestStartAndPollFailures();
  TestKeyboardOutcomesAndContinuation();
  TestInvalidRetryAndCancel();
  TestAvailableAndOverwriteNames();
  TestEmbeddedRectHelpers();
}
