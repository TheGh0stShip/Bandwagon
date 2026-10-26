#include "corona_runtime.hpp"

namespace bandwagon::smc {
namespace {

constexpr CoronaSmcProfile kCoronaProfile{
    0x30, 0x31, 0x3a, 0x3c, 0x60, 0x61, 0x4b, 0x55,
    0x0bf4, 0x0c55, 0x270d, 0x2646, 0x339d, 0x0dbb, 0x26e6, 0x26eb,
    0x2708, 0x2915, 0x2105, 0x293c, 0x0800, 0x0f4c, 0x083e, 0x334c};

void ApplyCoronaFinalization(std::uint8_t ee, CoronaSmcState* state,
                             CoronaHookPlan* plan) {
  if (!state || !plan) return;
  plan->wait_for_ee_clear = plan->wait_for_ee_clear || (ee & 1u) != 0;
  plan->acknowledge_ee = true;
  plan->finalized = true;
  plan->f5_writes = {0x55, 0xaa, 0x08};
  plan->f5_write_count = 3;
  state->ea = 0;
  state->eb = 0x40;
  state->ec = 0x09;
  state->ed = 0;
  state->e3 |= 0xc8u;
  state->e4 = static_cast<std::uint8_t>((state->e4 & 0xfcu) | 3u);
  state->f5 = 8;
  state->state &= 0x09u;
  state->bit_22_5 = true;
  state->bit_22_6 = false;
  state->bit_22_7 = false;
  state->boot_latch = 1;
  state->final_ram = 0;
  state->bit_22_4 = false;
}

}  // namespace

const CoronaSmcProfile& GetCoronaSmcProfile() { return kCoronaProfile; }

CoronaHookPlan RunCoronaBootHook(std::uint8_t accumulator,
                                CoronaSmcState* state) {
  CoronaHookPlan plan;
  plan.tail_jump = kCoronaProfile.boot_tail;
  plan.accumulator_output = accumulator ^ 4u;
  plan.has_accumulator_output = true;
  if (!state) return plan;
  state->boot_latch = 1;
  if ((plan.accumulator_output & 4u) != 0) state->state &= 0xe7u;
  return plan;
}

CoronaHookPlan RunCoronaStateGate(CoronaSmcState* state) {
  CoronaHookPlan plan;
  plan.tail_jump = kCoronaProfile.state_gate_tail;
  if (!state || (state->state & 0x10u) != 0) return plan;
  state->state |= 0x10u;
  const bool suppress_start =
      state->mode_ram == 0x11 && (state->state & 8u) != 0;
  if (!suppress_start && (state->state & 1u) != 0) {
    state->state |= 2u;
    state->counter = 0;
  }
  return plan;
}

CoronaHookPlan RunCoronaNotificationHook(bool carry_probe,
                                        CoronaSmcState* state) {
  CoronaHookPlan plan;
  plan.emit_call = kCoronaProfile.notification_emit_call;
  if (!state) return plan;
  if (state->bit_22_4 && (state->state & 2u) != 0) {
    plan.tail_jump = kCoronaProfile.notification_finalize;
    return plan;
  }
  plan.emitted[plan.emitted_count++] = 0;
  plan.emitted[plan.emitted_count++] =
      static_cast<std::uint8_t>((state->state & 8u) != 0);
  plan.emitted[plan.emitted_count++] = state->state;
  for (const auto value : state->staged)
    plan.emitted[plan.emitted_count++] = value;
  plan.emitted[plan.emitted_count++] = state->e3;
  plan.emitted[plan.emitted_count++] = state->e4;
  plan.emitted[plan.emitted_count++] = state->e5;
  plan.emitted[plan.emitted_count++] = state->e6;
  plan.calls[0] = kCoronaProfile.notification_carry_call;
  plan.call_count = 1;
  plan.emitted[plan.emitted_count++] = static_cast<std::uint8_t>(carry_probe);
  state->bit_22_4 = true;
  return plan;
}

CoronaHookPlan RunCoronaCommandHook(std::uint8_t command,
                                   std::uint8_t query_result,
                                   std::uint8_t completion_result,
                                   std::uint8_t ee,
                                   CoronaSmcState* state) {
  CoronaHookPlan plan;
  if (command == 0x9f) {
    plan.tail_jump = kCoronaProfile.passthrough_tail;
    return plan;
  }
  if (command != 0xa0 || !state) return plan;
  plan.calls = {kCoronaProfile.command_setup_call,
                kCoronaProfile.command_query_call, 0};
  plan.call_accumulators[0] = 1;
  plan.call_count = 2;
  switch (query_result) {
    case 2:
      state->state &= 0xfdu;
      break;
    case 4:
      state->state |= 4u;
      state->counter = 0;
      break;
    case 5:
      state->state &= 0xfbu;
      break;
    case 6:
      if ((state->state & 1u) != 0) {
        state->p2_bit_6 = true;
        plan.wait_for_ee_clear = (ee & 1u) != 0;
        plan.acknowledge_ee = true;
        plan.transfer_performed = true;
        state->ea = 0xff;
        state->eb = 0xff;
        state->ec = 0xff;
        state->ed = 0xff;
        state->e3 = static_cast<std::uint8_t>((state->e3 & 0xfbu) | 0xc0u);
        state->e4 = static_cast<std::uint8_t>((state->e4 & 0xfcu) | 2u);
        plan.calls[2] = kCoronaProfile.command_query_call;
        plan.call_count = 3;
        state->f5 = completion_result;
        state->p2_bit_6 = false;
      }
      break;
    default:
      break;
  }
  return plan;
}

CoronaHookPlan RunCoronaEightByteReadHook(
    std::uint8_t selector, std::uint8_t external_base,
    const std::array<std::uint8_t, 8>& external_bytes,
    CoronaSmcState* state) {
  CoronaHookPlan plan;
  if (selector != 0x30) {
    if (state) state->bit_27_3 = false;
    return plan;
  }
  plan.calls = {kCoronaProfile.command_setup_call,
                kCoronaProfile.command_query_call,
                kCoronaProfile.read_setup_call};
  plan.call_accumulators[0] = 1;
  plan.call_accumulators[2] = 1;
  plan.call_count = 3;
  plan.external_base = external_base;
  plan.emit_call = kCoronaProfile.notification_emit_call;
  for (const auto value : external_bytes)
    plan.emitted[plan.emitted_count++] = value;
  return plan;
}

CoronaHookPlan RunCoronaStatusHook(bool probe_carry, CoronaSmcState* state) {
  CoronaHookPlan plan;
  plan.calls[0] = kCoronaProfile.status_call;
  plan.call_count = 1;
  plan.tail_jump = kCoronaProfile.status_tail;
  if (state) {
    state->state &= 0xe7u;
    if (probe_carry) state->state |= 8u;
  }
  return plan;
}

CoronaHookPlan RunCoronaResetHook(CoronaSmcState* state) {
  CoronaHookPlan plan;
  plan.calls[0] = kCoronaProfile.reset_call;
  plan.call_count = 1;
  plan.tail_jump = kCoronaProfile.reset_tail;
  if (state) state->state &= 0xfdu;
  return plan;
}

CoronaHookPlan PlanCoronaTickWrapper() {
  CoronaHookPlan plan;
  plan.calls = {kCoronaProfile.tick_call, kCoronaProfile.sender_entry, 0};
  plan.call_count = 2;
  plan.tail_jump = kCoronaProfile.tick_entry;
  return plan;
}

void RunCoronaXbppReceive(const std::array<std::uint8_t, 4>& inbound,
                          std::uint8_t r4, std::uint8_t r5,
                          CoronaSmcState* state) {
  if (!state) return;
  state->r4_copy = r4;
  state->r5_copy = r5;
  const XbppHandshakePlan plan =
      PlanXbppHandshake(Motherboard::kCorona, inbound, state->state);
  state->state = plan.state_value;
  state->eb = plan.initial_eb;
  state->ea = plan.initial_ea;
  state->f5 = plan.initial_f5;
  if (!plan.recognized) return;
  state->staged = plan.staged_bytes;
  state->eb = 1;
  state->ea = plan.corona_followup_ea;
  state->f5 = plan.transfer_f5;
}

CoronaHookPlan RunCoronaSenderTick(std::uint8_t ee, CoronaSmcState* state) {
  CoronaHookPlan plan;
  if (!state) return plan;
  if ((state->state & 2u) != 0) {
    state->p2_bit_6 = true;
    plan.wait_for_ee_clear = (ee & 1u) != 0;
    plan.acknowledge_ee = true;
    plan.transfer_performed = true;
    state->ea = 0;
    state->eb = state->staged[0];
    state->ec = state->staged[1];
    state->ed = state->staged[2];
    state->e3 |= 0xc0u;
    state->e4 = static_cast<std::uint8_t>((state->e4 & 0xfcu) | 2u);
    state->f5 = 6;
    ++state->counter;
    if (state->counter == 0) {
      state->state = static_cast<std::uint8_t>(state->state + 0x40u);
      if ((state->state & 0x80u) != 0) {
        state->p2_bit_6 = false;
        ApplyCoronaFinalization(ee, state, &plan);
        return plan;
      }
    }
    state->p2_bit_6 = false;
    return plan;
  }
  if ((state->state & 4u) != 0) {
    ++state->counter;
    if (state->counter == 0x14)
      ApplyCoronaFinalization(ee, state, &plan);
  }
  return plan;
}

CoronaHookPlan RunCoronaFinalization(std::uint8_t ee,
                                    CoronaSmcState* state) {
  CoronaHookPlan plan;
  ApplyCoronaFinalization(ee, state, &plan);
  return plan;
}

}  // namespace bandwagon::smc
