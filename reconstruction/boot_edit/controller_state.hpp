#pragma once

#include "modal_state.hpp"

#include <array>
#include <cstdint>
#include <functional>

namespace bandwagon::boot_edit {

inline constexpr std::uint32_t kControllerNotConnected = 1167;

struct ControllerRecord {
  bool connected = false;
  std::array<std::uint8_t, 16> previous{};
  std::array<std::uint8_t, 16> current{};
  std::array<std::uint8_t, 16> derived{};
};

using ControllerRecords = std::array<ControllerRecord, 4>;

struct ControllerServices {
  std::function<std::uint32_t(std::uint32_t,
                              std::array<std::uint8_t, 16>*)>
      poll;
  std::function<void(std::uint32_t)> fatal_error;
};

// Complete models of the 0x8204C450 initialization extent and the polling
// loop inside 0x8204C4D8.
void InitializeControllers(ControllerRecords* records,
                           const ControllerServices& services);
void UpdateControllers(ControllerRecords* records,
                       const ControllerServices& services);

using ControllerMatch = std::function<bool(
    ControllerRecord*, std::uint16_t, bool)>;

// Exact fixed-four-record scan at 0x82010c58. The adjacent per-record button
// predicate remains an injected boundary until its compound inventory extent
// is separated and attributed.
bool AnyControllerMatches(ControllerRecords* records, std::uint16_t mask,
                          bool consume, const ControllerMatch& matches);

// Embedded non-pdata leaves inside the 0x8204C4D8 inventory extent.
void ApplyBinaryModalChoice(std::uint32_t application_modal_type,
                            bool alternate_choice,
                            ModalCompletionState* modal);
void CompleteModalChoice(std::uint32_t application_modal_type,
                         ModalCompletionState* modal);

}  // namespace bandwagon::boot_edit
