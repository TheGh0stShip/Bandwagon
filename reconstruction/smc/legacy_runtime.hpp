#pragma once

#include "xbpp_handshake.hpp"

#include <array>
#include <cstdint>

namespace bandwagon::smc {

struct LegacySmcProfile {
  Motherboard board;
  std::uint8_t counter_address;
  std::uint8_t staging_address;
  std::uint8_t mode_ram_address;
  std::uint8_t inbound_copy_address;
  std::uint8_t final_ram_address;
  std::uint8_t passthrough_command;
  bool notification_leading_zero;
  std::uint16_t boot_tail;
  std::uint16_t state_gate_tail;
  std::uint16_t notification_call;
  std::uint16_t notification_finalize;
  std::uint16_t passthrough_tail;
  std::uint16_t command_setup_call;
  std::uint16_t command_query_call;
  std::uint16_t status_call;
  std::uint16_t status_tail;
  std::uint16_t reset_tail;
  std::uint16_t tick_call;
  std::uint16_t sender_call;
  std::uint16_t tick_entry;
  std::uint16_t sender_entry;
  std::uint16_t ee_clear_tail;
};

const LegacySmcProfile* GetLegacySmcProfile(Motherboard board);

struct LegacySmcState {
  std::uint8_t state = 0;
  std::uint8_t counter = 0;
  std::uint8_t boot_latch = 0;
  std::uint8_t mode_ram = 0;
  std::uint8_t inbound_copy = 0;
  std::uint8_t final_ram = 0;
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
  std::uint8_t e5 = 0;
  std::uint8_t e6 = 0;
  std::uint8_t f5 = 0;
  std::array<std::uint8_t, 3> staged{};
};

struct LegacyHookPlan {
  std::array<std::uint16_t, 2> calls{};
  std::array<std::uint8_t, 2> call_accumulators{};
  std::uint8_t call_count = 0;
  std::uint16_t tail_jump = 0;
  std::uint16_t emit_call = 0;
  std::array<std::uint8_t, 10> emitted{};
  std::uint8_t emitted_count = 0;
  std::array<std::uint8_t, 4> e3_writes{};
  std::uint8_t e3_write_count = 0;
  bool wait_for_ee_clear = false;
};

LegacyHookPlan RunLegacyBootHook(const LegacySmcProfile& profile,
                                 std::uint8_t accumulator,
                                 LegacySmcState* state);
LegacyHookPlan RunLegacyStateGate(const LegacySmcProfile& profile,
                                  LegacySmcState* state);
LegacyHookPlan RunLegacyNotificationHook(const LegacySmcProfile& profile,
                                         LegacySmcState* state);
LegacyHookPlan RunLegacyCommandHook(const LegacySmcProfile& profile,
                                    std::uint8_t command,
                                    std::uint8_t query_result,
                                    std::uint8_t ee,
                                    LegacySmcState* state);
LegacyHookPlan RunLegacyStatusHook(const LegacySmcProfile& profile,
                                   bool probe_carry,
                                   LegacySmcState* state);
LegacyHookPlan RunLegacyResetHook(const LegacySmcProfile& profile,
                                  LegacySmcState* state);
LegacyHookPlan PlanLegacyTickWrapper(const LegacySmcProfile& profile);
LegacyHookPlan PlanLegacySenderWrapper(const LegacySmcProfile& profile);
void RunLegacyXbppReceive(const LegacySmcProfile& profile,
                          const std::array<std::uint8_t, 4>& inbound,
                          LegacySmcState* state);
LegacyHookPlan RunLegacyEeRedirect(const LegacySmcProfile& profile,
                                   std::uint8_t ee,
                                   LegacySmcState* state);
void RunLegacySender(const LegacySmcProfile& profile, std::uint8_t ee,
                     LegacySmcState* state);
void RunLegacyTick(const LegacySmcProfile& profile, LegacySmcState* state);

}  // namespace bandwagon::smc
