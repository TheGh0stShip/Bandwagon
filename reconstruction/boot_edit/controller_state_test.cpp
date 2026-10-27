#include "controller_state.hpp"

#include <cassert>
#include <vector>

using namespace bandwagon::boot_edit;

int main() {
  ControllerRecords records;
  std::vector<std::uint32_t> polls;
  std::vector<std::uint32_t> fatal;
  ControllerServices services;
  services.poll = [&](std::uint32_t index,
                      std::array<std::uint8_t, 16>* state) {
    polls.push_back(index);
    (*state)[0] = static_cast<std::uint8_t>(index + 1);
    if (index == 1) return kControllerNotConnected;
    if (index == 3) return 5u;
    return 0u;
  };
  services.fatal_error = [&](std::uint32_t status) { fatal.push_back(status); };
  InitializeControllers(&records, services);
  assert((polls == std::vector<std::uint32_t>{0, 1, 2, 3}));
  assert(records[0].connected && !records[1].connected);
  assert(records[2].connected && !records[3].connected);
  assert(records[2].previous[0] == 3);
  assert((fatal == std::vector<std::uint32_t>{5}));

  polls.clear();
  fatal.clear();
  for (std::size_t i = 0; i < records.size(); ++i)
    records[i].current[1] = static_cast<std::uint8_t>(10 + i);
  UpdateControllers(&records, services);
  assert(records[2].previous[1] == 12);
  assert(records[2].current[0] == 3);
  assert((polls == std::vector<std::uint32_t>{0, 1, 2, 3}));

  std::vector<std::size_t> matched_indices;
  assert(AnyControllerMatches(
      &records, 0x20, true,
      [&](ControllerRecord* record, std::uint16_t mask, bool consume) {
        assert(mask == 0x20 && consume);
        const std::size_t index = static_cast<std::size_t>(record - &records[0]);
        matched_indices.push_back(index);
        return index == 2;
      }));
  assert((matched_indices == std::vector<std::size_t>{0, 1, 2}));
  matched_indices.clear();
  assert(!AnyControllerMatches(
      &records, 0x40, false,
      [&](ControllerRecord* record, std::uint16_t mask, bool consume) {
        assert(mask == 0x40 && !consume);
        matched_indices.push_back(
            static_cast<std::size_t>(record - &records[0]));
        return false;
      }));
  assert((matched_indices == std::vector<std::size_t>{0, 1, 2, 3}));
  assert(!AnyControllerMatches(nullptr, 1, false, {}));

  ModalCompletionState modal;
  ApplyBinaryModalChoice(1, true, &modal);
  assert(modal.selected_action == 0);
  ApplyBinaryModalChoice(2, false, &modal);
  assert(modal.selected_action == 1);
  ApplyBinaryModalChoice(2, true, &modal);
  assert(modal.selected_action == 2);

  int continuations = 0;
  modal.continuation = [&](ModalCompletionState*, std::uintptr_t) {
    ++continuations;
  };
  CompleteModalChoice(3, &modal);
  assert(modal.result == 3 && modal.completion_pending);
  CompleteModalChoice(8, &modal);
  assert(modal.result == 1 && continuations == 2);
}
