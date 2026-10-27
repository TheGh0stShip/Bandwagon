#include "controller_state.hpp"

namespace bandwagon::boot_edit {
namespace {

void ApplyPollResult(ControllerRecord* record, std::uint32_t status,
                     const ControllerServices& services) {
  if (status == 0) {
    record->connected = true;
  } else if (status == kControllerNotConnected) {
    record->connected = false;
  } else if (services.fatal_error) {
    services.fatal_error(status);
  }
}

}  // namespace

void InitializeControllers(ControllerRecords* records,
                           const ControllerServices& services) {
  if (!records) return;
  for (std::uint32_t index = 0; index < records->size(); ++index) {
    (*records)[index] = {};
    const std::uint32_t status =
        services.poll ? services.poll(index, &(*records)[index].previous)
                      : kControllerNotConnected;
    ApplyPollResult(&(*records)[index], status, services);
  }
}

void UpdateControllers(ControllerRecords* records,
                       const ControllerServices& services) {
  if (!records) return;
  for (std::uint32_t index = 0; index < records->size(); ++index) {
    ControllerRecord& record = (*records)[index];
    record.previous = record.current;
    const std::uint32_t status =
        services.poll ? services.poll(index, &record.current)
                      : kControllerNotConnected;
    ApplyPollResult(&record, status, services);
  }
}

bool AnyControllerMatches(ControllerRecords* records, std::uint16_t mask,
                          bool consume, const ControllerMatch& matches) {
  if (!records || !matches) return false;
  for (ControllerRecord& record : *records) {
    if (matches(&record, mask, consume)) return true;
  }
  return false;
}

void ApplyBinaryModalChoice(std::uint32_t application_modal_type,
                            bool alternate_choice,
                            ModalCompletionState* modal) {
  if (!modal || application_modal_type != 2) return;
  modal->selected_action = alternate_choice ? 2u : 1u;
}

void CompleteModalChoice(std::uint32_t application_modal_type,
                         ModalCompletionState* modal) {
  if (!modal) return;
  modal->result = application_modal_type == 3 ? 3u : 1u;
  modal->completion_pending = true;
  if (modal->continuation)
    modal->continuation(modal, modal->continuation_argument);
}

}  // namespace bandwagon::boot_edit
