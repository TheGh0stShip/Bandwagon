#pragma once

#include "xbpp_handshake.hpp"

#include <array>
#include <cstdint>

namespace bandwagon::smc {

struct TrinityState {
  std::uint8_t state_3a = 0;
  std::uint8_t counter_30 = 0;
  std::uint8_t ram_3b = 0;
  std::uint8_t ram_54 = 0;
  std::uint8_t ram_5f = 0;
  std::uint8_t ram_60 = 0;
  bool bit_22_3 = false;
  bool bit_22_4 = false;
  bool bit_22_5 = false;
  bool bit_22_6 = false;

  std::uint8_t ea = 0;
  std::uint8_t eb = 0;
  std::uint8_t ec = 0;
  std::uint8_t ed = 0;
  std::uint8_t e3 = 0;
  std::uint8_t e4 = 0;
  std::uint8_t f5 = 0;
  std::array<std::uint8_t, 3> staged{};
};

struct TrinityHookPlan {
  std::array<std::uint16_t, 2> calls{};
  std::uint8_t call_count = 0;
  std::uint16_t tail_jump = 0;
  std::array<std::uint8_t, 2> call_accumulators{};
};

TrinityHookPlan RunTrinityBootHook(std::uint8_t accumulator,
                                   TrinityState* state);
TrinityHookPlan RunTrinityCommandHook(std::uint8_t command,
                                      std::uint8_t query_result,
                                      TrinityState* state);
TrinityHookPlan RunTrinityStatusHook(bool probe_carry, TrinityState* state);
TrinityHookPlan PlanTrinityTickWrapper();
void RunTrinityXbppReceive(const std::array<std::uint8_t, 4>& inbound,
                           TrinityState* state);
TrinityHookPlan RunTrinityEeRedirect(std::uint8_t ee, TrinityState* state);
TrinityHookPlan RunTrinitySender(std::uint8_t ee, TrinityState* state);
void RunTrinityTick(TrinityState* state);
TrinityHookPlan RunTrinityNotificationHook(TrinityState* state);
TrinityHookPlan RunTrinityStateGate(TrinityState* state);
TrinityHookPlan RunTrinityResetHook(TrinityState* state);

}  // namespace bandwagon::smc
