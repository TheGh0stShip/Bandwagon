#include "legacy_runtime.hpp"

#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>

int main() {
  using namespace bandwagon::smc;
  constexpr std::array boards{Motherboard::kFalcon, Motherboard::kJasper,
                              Motherboard::kXenon, Motherboard::kZephyr};

  for (const Motherboard board : boards) {
    const LegacySmcProfile* profile = GetLegacySmcProfile(board);
    assert(profile && profile->board == board);

    LegacySmcState boot;
    boot.state = 0xff;
    auto plan = RunLegacyBootHook(*profile, 0, &boot);
    assert(boot.boot_latch == 1 && boot.state == 0xe7);
    assert(plan.tail_jump == profile->boot_tail);
    boot.state = 0xe7;
    RunLegacyBootHook(*profile, 4, &boot);
    assert(boot.state == 0xff);

    LegacySmcState gate;
    gate.state = 1;
    gate.counter = 9;
    assert(RunLegacyStateGate(*profile, &gate).tail_jump ==
           profile->state_gate_tail);
    assert(gate.state == 0x13 && gate.counter == 0);
    LegacySmcState suppressed;
    suppressed.state = 9;
    suppressed.mode_ram = 0x11;
    RunLegacyStateGate(*profile, &suppressed);
    assert(suppressed.state == 0x19);

    LegacySmcState notification;
    notification.state = 0x0b;
    notification.staged = {'P', 'B', 'X'};
    notification.e3 = 3;
    notification.e4 = 4;
    notification.e5 = 5;
    notification.e6 = 6;
    plan = RunLegacyNotificationHook(*profile, &notification);
    assert(plan.emit_call == profile->notification_call);
    assert(plan.emitted_count ==
           (profile->notification_leading_zero ? 10 : 9));
    const std::size_t prefix = profile->notification_leading_zero ? 1 : 0;
    assert(plan.emitted[prefix] == 1);
    assert(plan.emitted[prefix + 1] == 0x0b);
    assert(notification.bit_22_3);
    assert(RunLegacyNotificationHook(*profile, &notification).tail_jump ==
           profile->notification_finalize);

    LegacySmcState command;
    command.state = 0xff;
    command.counter = 8;
    plan = RunLegacyCommandHook(*profile, 0xa0, 1, 0, &command);
    assert(plan.calls[0] == profile->command_setup_call &&
           plan.calls[1] == profile->command_query_call &&
           plan.call_accumulators[0] == 1);
    assert(command.state == 0x39 && command.counter == 0);
    assert(RunLegacyCommandHook(*profile, profile->passthrough_command, 0, 0,
                                &command).tail_jump ==
           profile->passthrough_tail);
    command.state = 1;
    command.staged = {3, 2, 1};
    command.e3 = 1;
    command.e4 = 0xff;
    plan = RunLegacyCommandHook(*profile, 0xa0, 6, 1, &command);
    assert(plan.wait_for_ee_clear);
    assert(command.eb == 3 && command.ec == 2 && command.ed == 1 &&
           command.e3 == 0xc1 && command.e4 == 0xfe && command.f5 == 6);

    LegacySmcState status;
    status.state = 0xff;
    plan = RunLegacyStatusHook(*profile, true, &status);
    assert(status.state == 9 && plan.calls[0] == profile->status_call &&
           plan.tail_jump == profile->status_tail);
    assert(RunLegacyResetHook(*profile, &status).tail_jump ==
           profile->reset_tail);
    assert(status.state == 0);
    assert(PlanLegacyTickWrapper(*profile).tail_jump == profile->tick_entry);
    assert(PlanLegacySenderWrapper(*profile).tail_jump ==
           profile->sender_entry);

    LegacySmcState receive;
    receive.state = 0xaa;
    RunLegacyXbppReceive(*profile, {'X', 'B', 'P', 'P'}, &receive);
    assert(receive.inbound_copy == 'P' && receive.state == 0xab);
    assert((receive.staged ==
            std::array<std::uint8_t, 3>{'P', 'B', 'X'}));
    assert(receive.ea == 0x18 && receive.eb == 1);

    LegacySmcState redirect;
    redirect.state = 0xff;
    redirect.e3 = 0xaa;
    plan = RunLegacyEeRedirect(*profile, 1, &redirect);
    assert(redirect.state == 1 && redirect.e3 == 0 &&
           plan.e3_write_count == 4 &&
           (plan.e3_writes == std::array<std::uint8_t, 4>{1, 0, 1, 0}) &&
           plan.tail_jump == 0);
    assert(RunLegacyEeRedirect(*profile, 0, &redirect).tail_jump ==
           profile->ee_clear_tail);

    LegacySmcState sender;
    sender.state = 2;
    sender.staged = {3, 2, 1};
    sender.e3 = 1;
    sender.e4 = 0xff;
    RunLegacySender(*profile, 0, &sender);
    assert(sender.eb == 3 && sender.ec == 2 && sender.ed == 1 &&
           sender.e3 == 0xc1 && sender.e4 == 0xfe && sender.f5 == 6);
    LegacySmcState busy;
    busy.state = 2;
    busy.f5 = 9;
    RunLegacySender(*profile, 1, &busy);
    assert(busy.f5 == 9);

    LegacySmcState timeout;
    timeout.state = 0x24;
    timeout.counter = 0x1d;
    timeout.bit_22_3 = true;
    timeout.bit_22_5 = true;
    timeout.bit_22_6 = true;
    RunLegacyTick(*profile, &timeout);
    assert(timeout.state == 0 && timeout.bit_22_4 &&
           !timeout.bit_22_3 && !timeout.bit_22_5 && !timeout.bit_22_6 &&
           timeout.boot_latch == 1 && timeout.final_ram == 0);
  }

  const auto* xenon = GetLegacySmcProfile(Motherboard::kXenon);
  assert(xenon && xenon->counter_address == 0xb0 &&
         xenon->staging_address == 0xb1 && xenon->passthrough_command == 0x9c);
  const auto* jasper = GetLegacySmcProfile(Motherboard::kJasper);
  assert(jasper && !jasper->notification_leading_zero);
  assert(GetLegacySmcProfile(Motherboard::kCorona) == nullptr);
  assert(GetLegacySmcProfile(Motherboard::kTrinity) == nullptr);

  std::cout << "Legacy-board SMC runtime reconstruction tests passed\n";
}
