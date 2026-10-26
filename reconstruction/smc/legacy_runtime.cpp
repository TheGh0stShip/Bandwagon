#include "legacy_runtime.hpp"

namespace bandwagon::smc {
namespace {

constexpr LegacySmcProfile kProfiles[] = {
    {Motherboard::kFalcon, 0x30, 0x31, 0x62, 0x52, 0x63, 0x9f, true,
     0x0b8e, 0x0bef, 0x242f, 0x2eac, 0x0d36, 0x2408, 0x240d,
     0x25a6, 0x1f1d, 0x25d2, 0x0ea4, 0x1a2e, 0x2e86, 0x2e59, 0x1151},
    {Motherboard::kJasper, 0x30, 0x31, 0x63, 0x53, 0x64, 0x9f, false,
     0x0b93, 0x0bf4, 0x246c, 0x2f18, 0x0d3b, 0x2445, 0x244a,
     0x25f8, 0x1f38, 0x2624, 0x0ea9, 0x1a3a, 0x2ef2, 0x2ec5, 0x1156},
    {Motherboard::kXenon, 0xb0, 0xb1, 0x61, 0x52, 0x62, 0x9c, true,
     0x0aaf, 0x0b10, 0x238e, 0x2f53, 0x0c38, 0x236e, 0x2373,
     0x24f8, 0x1e2d, 0x2520, 0x0d94, 0x1936, 0x2f2d, 0x2f00, 0x1022},
    {Motherboard::kZephyr, 0x30, 0x31, 0x62, 0x52, 0x63, 0x9c, true,
     0x0b5c, 0x0bbd, 0x239e, 0x2e08, 0x0ce5, 0x237e, 0x2383,
     0x250e, 0x1e93, 0x253a, 0x0e58, 0x19b7, 0x2de2, 0x2db5, 0x1105},
};

void ApplyTransfer(LegacySmcState* state) {
  state->ea = 0;
  state->eb = state->staged[0];
  state->ec = state->staged[1];
  state->ed = state->staged[2];
  state->e3 |= 0xc0u;
  state->e4 = static_cast<std::uint8_t>((state->e4 & 0xfcu) | 2u);
  state->f5 = 6;
}

}  // namespace

const LegacySmcProfile* GetLegacySmcProfile(Motherboard board) {
  for (const auto& profile : kProfiles)
    if (profile.board == board) return &profile;
  return nullptr;
}

LegacyHookPlan RunLegacyBootHook(const LegacySmcProfile& profile,
                                 std::uint8_t accumulator,
                                 LegacySmcState* state) {
  LegacyHookPlan plan;
  plan.tail_jump = profile.boot_tail;
  if (!state) return plan;
  state->boot_latch = 1;
  state->state &= 0xe7u;
  if ((accumulator & 4u) != 0) state->state |= 0x18u;
  return plan;
}

LegacyHookPlan RunLegacyStateGate(const LegacySmcProfile& profile,
                                  LegacySmcState* state) {
  LegacyHookPlan plan;
  plan.tail_jump = profile.state_gate_tail;
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

LegacyHookPlan RunLegacyNotificationHook(const LegacySmcProfile& profile,
                                         LegacySmcState* state) {
  LegacyHookPlan plan;
  plan.emit_call = profile.notification_call;
  if (!state) return plan;
  if (state->bit_22_3 && (state->state & 2u) != 0) {
    plan.tail_jump = profile.notification_finalize;
    return plan;
  }
  if (profile.notification_leading_zero)
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
  state->bit_22_3 = true;
  return plan;
}

LegacyHookPlan RunLegacyCommandHook(const LegacySmcProfile& profile,
                                    std::uint8_t command,
                                    std::uint8_t query_result,
                                    std::uint8_t ee,
                                    LegacySmcState* state) {
  LegacyHookPlan plan;
  if (command == profile.passthrough_command) {
    plan.tail_jump = profile.passthrough_tail;
    return plan;
  }
  if (command != 0xa0 || !state) return plan;
  plan.calls = {profile.command_setup_call, profile.command_query_call};
  plan.call_accumulators[0] = 1;
  plan.call_count = 2;
  switch (query_result) {
    case 1:
      state->state = static_cast<std::uint8_t>((state->state & 0x19u) | 0x20u);
      state->counter = 0;
      break;
    case 2:
    case 4:
      state->state &= 0x19u;
      break;
    case 3:
      state->state |= 4u;
      state->counter = 0;
      break;
    case 6:
      if ((state->state & 1u) != 0) {
        plan.wait_for_ee_clear = (ee & 1u) != 0;
        ApplyTransfer(state);
      }
      break;
    default:
      break;
  }
  return plan;
}

LegacyHookPlan RunLegacyStatusHook(const LegacySmcProfile& profile,
                                   bool probe_carry,
                                   LegacySmcState* state) {
  LegacyHookPlan plan;
  plan.calls[0] = profile.status_call;
  plan.call_count = 1;
  plan.tail_jump = profile.status_tail;
  if (state) {
    state->state &= 1u;
    if (probe_carry) state->state |= 8u;
  }
  return plan;
}

LegacyHookPlan RunLegacyResetHook(const LegacySmcProfile& profile,
                                  LegacySmcState* state) {
  LegacyHookPlan plan;
  plan.tail_jump = profile.reset_tail;
  if (state) state->state = 0;
  return plan;
}

LegacyHookPlan PlanLegacyTickWrapper(const LegacySmcProfile& profile) {
  LegacyHookPlan plan;
  plan.calls[0] = profile.tick_call;
  plan.call_count = 1;
  plan.tail_jump = profile.tick_entry;
  return plan;
}

LegacyHookPlan PlanLegacySenderWrapper(const LegacySmcProfile& profile) {
  LegacyHookPlan plan;
  plan.calls[0] = profile.sender_call;
  plan.call_count = 1;
  plan.tail_jump = profile.sender_entry;
  return plan;
}

void RunLegacyXbppReceive(const LegacySmcProfile& profile,
                          const std::array<std::uint8_t, 4>& inbound,
                          LegacySmcState* state) {
  if (!state) return;
  state->inbound_copy = inbound[2];
  const XbppHandshakePlan plan =
      PlanXbppHandshake(profile.board, inbound, state->state);
  state->state = plan.state_value;
  state->eb = plan.initial_eb;
  state->ea = plan.initial_ea;
  state->f5 = plan.initial_f5;
  if (!plan.recognized) return;
  state->staged = plan.staged_bytes;
  state->eb = 1;
  state->ea = plan.transfer_ea;
  state->f5 = plan.transfer_f5;
}

LegacyHookPlan RunLegacyEeRedirect(const LegacySmcProfile& profile,
                                   std::uint8_t ee,
                                   LegacySmcState* state) {
  LegacyHookPlan plan;
  if (state) state->state &= 1u;
  if ((ee & 1u) != 0) {
    plan.e3_writes = {1, 0, 1, 0};
    plan.e3_write_count = 4;
    if (state) state->e3 = 0;
  } else {
    plan.tail_jump = profile.ee_clear_tail;
  }
  return plan;
}

void RunLegacySender(const LegacySmcProfile& profile, std::uint8_t ee,
                     LegacySmcState* state) {
  if (!state) return;
  const XbppTransferPlan plan = PlanXbppTransfer(
      profile.board, state->state, ee, state->e3, state->e4, state->staged);
  if (!plan.transfer) return;
  ApplyTransfer(state);
}

void RunLegacyTick(const LegacySmcProfile& profile, LegacySmcState* state) {
  (void)profile;
  if (!state) return;
  bool finalize = false;
  if ((state->state & 2u) != 0) {
    ++state->counter;
    if (state->counter == 0) {
      state->state = static_cast<std::uint8_t>(state->state + 0x40u);
      finalize = (state->state & 0x80u) != 0;
    }
  } else if ((state->state & 0x24u) != 0) {
    ++state->counter;
    finalize = state->counter == 0x1e;
  }
  if (!finalize) return;
  state->state &= 0x09u;
  state->bit_22_4 = true;
  state->bit_22_5 = false;
  state->bit_22_6 = false;
  state->boot_latch = 1;
  state->final_ram = 0;
  state->bit_22_3 = false;
}

}  // namespace bandwagon::smc
