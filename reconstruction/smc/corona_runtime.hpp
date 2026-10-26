#pragma once

#include "xbpp_handshake.hpp"

#include <array>
#include <cstdint>

namespace bandwagon::smc {

struct CoronaSmcProfile {
  std::uint8_t counter_address;
  std::uint8_t staging_address;
  std::uint8_t state_address;
  std::uint8_t boot_latch_address;
  std::uint8_t mode_ram_address;
  std::uint8_t final_ram_address;
  std::uint8_t r4_copy_address;
  std::uint8_t r5_copy_address;
  std::uint16_t boot_tail;
  std::uint16_t state_gate_tail;
  std::uint16_t notification_emit_call;
  std::uint16_t notification_carry_call;
  std::uint16_t notification_finalize;
  std::uint16_t passthrough_tail;
  std::uint16_t command_setup_call;
  std::uint16_t command_query_call;
  std::uint16_t read_setup_call;
  std::uint16_t status_call;
  std::uint16_t status_tail;
  std::uint16_t reset_call;
  std::uint16_t reset_tail;
  std::uint16_t tick_call;
  std::uint16_t tick_entry;
  std::uint16_t sender_entry;
};

const CoronaSmcProfile& GetCoronaSmcProfile();

struct CoronaSmcState {
  std::uint8_t state = 0;
  std::uint8_t counter = 0;
  std::uint8_t boot_latch = 0;
  std::uint8_t mode_ram = 0;
  std::uint8_t final_ram = 0;
  std::uint8_t r4_copy = 0;
  std::uint8_t r5_copy = 0;
  bool bit_22_4 = false;
  bool bit_22_5 = false;
  bool bit_22_6 = false;
  bool bit_22_7 = false;
  bool bit_27_3 = false;
  bool p2_bit_6 = false;
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

struct CoronaHookPlan {
  std::array<std::uint16_t, 3> calls{};
  std::array<std::uint8_t, 3> call_accumulators{};
  std::uint8_t call_count = 0;
  std::uint16_t tail_jump = 0;
  std::uint16_t emit_call = 0;
  std::array<std::uint8_t, 11> emitted{};
  std::uint8_t emitted_count = 0;
  std::array<std::uint8_t, 3> f5_writes{};
  std::uint8_t f5_write_count = 0;
  std::uint8_t accumulator_output = 0;
  std::uint8_t external_base = 0;
  bool has_accumulator_output = false;
  bool wait_for_ee_clear = false;
  bool acknowledge_ee = false;
  bool transfer_performed = false;
  bool finalized = false;
};

CoronaHookPlan RunCoronaBootHook(std::uint8_t accumulator,
                                CoronaSmcState* state);
CoronaHookPlan RunCoronaStateGate(CoronaSmcState* state);
CoronaHookPlan RunCoronaNotificationHook(bool carry_probe,
                                        CoronaSmcState* state);
CoronaHookPlan RunCoronaCommandHook(std::uint8_t command,
                                   std::uint8_t query_result,
                                   std::uint8_t completion_result,
                                   std::uint8_t ee,
                                   CoronaSmcState* state);
CoronaHookPlan RunCoronaEightByteReadHook(
    std::uint8_t selector, std::uint8_t external_base,
    const std::array<std::uint8_t, 8>& external_bytes,
    CoronaSmcState* state);
CoronaHookPlan RunCoronaStatusHook(bool probe_carry, CoronaSmcState* state);
CoronaHookPlan RunCoronaResetHook(CoronaSmcState* state);
CoronaHookPlan PlanCoronaTickWrapper();
void RunCoronaXbppReceive(const std::array<std::uint8_t, 4>& inbound,
                          std::uint8_t r4, std::uint8_t r5,
                          CoronaSmcState* state);
CoronaHookPlan RunCoronaSenderTick(std::uint8_t ee, CoronaSmcState* state);
CoronaHookPlan RunCoronaFinalization(std::uint8_t ee,
                                    CoronaSmcState* state);

}  // namespace bandwagon::smc
