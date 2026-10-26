#pragma once

#include <array>
#include <cstdint>

namespace bandwagon::smc {

enum class Motherboard {
  kCorona,
  kFalcon,
  kJasper,
  kTrinity,
  kXenon,
  kZephyr,
};

struct XbppHandshakePlan {
  bool recognized = false;

  // All six handlers perform these writes before comparing the four bytes.
  std::uint8_t initial_eb = 1;
  std::uint8_t initial_ea = 0x10;
  std::uint8_t initial_f5 = 0;

  std::uint8_t state_ram_address = 0;
  std::uint8_t state_value = 0;
  std::uint8_t staging_ram_address = 0;
  std::array<std::uint8_t, 3> staged_bytes{};

  bool rewrite_eb_for_transfer = false;
  std::uint8_t transfer_ea = 0;
  std::uint8_t transfer_f5 = 0;
  bool corona_followup_transfer = false;
  std::uint8_t corona_followup_ea = 0;
};

// Models the common SMC routine that recognizes SFR F1..F4 == "XBPP".
// state_before is the board's byte at RAM 0x3a or 0x3c before entry.
XbppHandshakePlan PlanXbppHandshake(
    Motherboard board, const std::array<std::uint8_t, 4>& inbound,
    std::uint8_t state_before);

}  // namespace bandwagon::smc
