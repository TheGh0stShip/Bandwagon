#include "trinity_runtime.hpp"

namespace bandwagon::smc {

TrinityHookPlan RunTrinityBootHook(std::uint8_t accumulator,
                                   TrinityState* state) {
  TrinityHookPlan plan;
  plan.tail_jump = 0x0c19;
  if (!state) return plan;
  state->ram_3b = 1;
  state->state_3a &= 0xe7u;
  if ((accumulator & 4u) != 0) state->state_3a |= 0x18u;
  return plan;
}

TrinityHookPlan RunTrinityCommandHook(std::uint8_t command,
                                      std::uint8_t query_result,
                                      TrinityState* state) {
  TrinityHookPlan plan;
  if (command == 0x9f) {
    plan.tail_jump = 0x0ddb;
    return plan;
  }
  if (command != 0xa0 || !state) return plan;
  plan.calls = {0x25a1, 0x25a6};
  plan.call_count = 2;
  plan.call_accumulators[0] = 1;
  switch (query_result) {
    case 1:
      state->state_3a = static_cast<std::uint8_t>(
          (state->state_3a & 0x19u) | 0x20u);
      state->counter_30 = 0;
      break;
    case 2:
      state->state_3a &= 0x19u;
      break;
    case 3:
      state->state_3a |= 4u;
      state->counter_30 = 0;
      break;
    case 4:
      state->state_3a &= 0x19u;
      break;
    default:
      break;
  }
  return plan;
}

TrinityHookPlan RunTrinityStatusHook(bool probe_carry, TrinityState* state) {
  TrinityHookPlan plan;
  plan.calls[0] = 0x2768;
  plan.call_count = 1;
  plan.tail_jump = 0x2067;
  if (state) {
    state->state_3a &= 1u;
    if (probe_carry) state->state_3a |= 8u;
  }
  return plan;
}

TrinityHookPlan PlanTrinityTickWrapper() {
  TrinityHookPlan plan;
  plan.calls[0] = 0x0f60;
  plan.call_count = 1;
  plan.tail_jump = 0x2fb6;
  return plan;
}

void RunTrinityXbppReceive(const std::array<std::uint8_t, 4>& inbound,
                           TrinityState* state) {
  if (!state) return;
  state->ram_54 = inbound[3];
  const XbppHandshakePlan plan = PlanXbppHandshake(
      Motherboard::kTrinity, inbound, state->state_3a);
  state->state_3a = plan.state_value;
  state->eb = plan.initial_eb;
  state->ea = plan.initial_ea;
  state->f5 = plan.initial_f5;
  if (!plan.recognized) return;
  state->staged = plan.staged_bytes;
  state->ea = plan.transfer_ea;
  state->f5 = plan.transfer_f5;
}

TrinityHookPlan RunTrinityEeRedirect(std::uint8_t ee, TrinityState* state) {
  TrinityHookPlan plan;
  if (state) state->state_3a &= 1u;
  plan.tail_jump = (ee & 1u) != 0 ? 0x00b0 : 0x1280;
  return plan;
}

TrinityHookPlan RunTrinitySender(std::uint8_t ee, TrinityState* state) {
  TrinityHookPlan hook;
  hook.calls[0] = 0x1b2c;
  hook.call_count = 1;
  if (!state) return hook;
  const XbppTransferPlan plan = PlanXbppTransfer(
      Motherboard::kTrinity, state->state_3a, ee, state->e3, state->e4,
      state->staged);
  if (!plan.transfer) return hook;
  state->ea = plan.ea;
  state->eb = plan.eb;
  state->ec = plan.ec;
  state->ed = plan.ed;
  state->e3 = plan.e3;
  state->e4 = plan.e4;
  state->f5 = plan.f5;
  return hook;
}

void RunTrinityTick(TrinityState* state) {
  if (!state) return;
  bool finalize = false;
  if ((state->state_3a & 2u) != 0) {
    ++state->counter_30;
    if (state->counter_30 == 0) {
      state->state_3a = static_cast<std::uint8_t>(state->state_3a + 0x40u);
      finalize = (state->state_3a & 0x80u) != 0;
    }
  } else if ((state->state_3a & 0x24u) != 0) {
    ++state->counter_30;
    finalize = state->counter_30 == 0x1e;
  }
  if (!finalize) return;
  state->state_3a &= 0x09u;
  state->bit_22_4 = true;
  state->bit_22_5 = false;
  state->bit_22_6 = false;
  state->ram_3b = 1;
  state->ram_60 = 0;
  state->bit_22_3 = false;
}

TrinityHookPlan RunTrinityNotificationHook(TrinityState* state) {
  TrinityHookPlan plan;
  if (!state) return plan;
  if (state->bit_22_3 && (state->state_3a & 2u) != 0) {
    plan.tail_jump = 0x2fdc;
    return plan;
  }
  plan.calls = {0x25c8, 0x25c8};
  plan.call_accumulators = {
      0, static_cast<std::uint8_t>((state->state_3a & 8u) != 0)};
  plan.call_count = 2;
  state->bit_22_3 = true;
  return plan;
}

TrinityHookPlan RunTrinityStateGate(TrinityState* state) {
  TrinityHookPlan plan;
  plan.tail_jump = 0x0c7a;
  if (!state || (state->state_3a & 0x10u) != 0) return plan;
  state->state_3a |= 0x10u;
  const bool suppress_start =
      state->ram_5f == 0x11 && (state->state_3a & 8u) != 0;
  if (!suppress_start && (state->state_3a & 1u) != 0) {
    state->state_3a |= 2u;
    state->counter_30 = 0;
  }
  return plan;
}

TrinityHookPlan RunTrinityResetHook(TrinityState* state) {
  TrinityHookPlan plan;
  plan.tail_jump = 0x2794;
  if (state) state->state_3a = 0;
  return plan;
}

}  // namespace bandwagon::smc
