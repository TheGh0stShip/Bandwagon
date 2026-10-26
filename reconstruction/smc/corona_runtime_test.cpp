#include "corona_runtime.hpp"

#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>

int main() {
  using namespace bandwagon::smc;
  const CoronaSmcProfile& profile = GetCoronaSmcProfile();
  assert(profile.counter_address == 0x30 && profile.state_address == 0x3a &&
         profile.sender_entry == 0x334c);

  CoronaSmcState boot;
  boot.state = 0xff;
  auto plan = RunCoronaBootHook(0, &boot);
  assert(plan.has_accumulator_output && plan.accumulator_output == 4 &&
         plan.tail_jump == profile.boot_tail);
  assert(boot.boot_latch == 1 && boot.state == 0xe7);
  boot.state = 0xff;
  assert(RunCoronaBootHook(4, &boot).accumulator_output == 0);
  assert(boot.state == 0xff);

  CoronaSmcState gate;
  gate.state = 1;
  gate.counter = 9;
  assert(RunCoronaStateGate(&gate).tail_jump == profile.state_gate_tail);
  assert(gate.state == 0x13 && gate.counter == 0);
  CoronaSmcState suppressed;
  suppressed.state = 9;
  suppressed.mode_ram = 0x11;
  RunCoronaStateGate(&suppressed);
  assert(suppressed.state == 0x19);

  CoronaSmcState notification;
  notification.state = 0x0b;
  notification.staged = {'P', 'B', 'X'};
  notification.e3 = 3;
  notification.e4 = 4;
  notification.e5 = 5;
  notification.e6 = 6;
  plan = RunCoronaNotificationHook(true, &notification);
  assert(plan.emit_call == profile.notification_emit_call &&
         plan.calls[0] == profile.notification_carry_call &&
         plan.emitted_count == 11 && plan.emitted[0] == 0 &&
         plan.emitted[1] == 1 && plan.emitted[10] == 1);
  assert(notification.bit_22_4);
  assert(RunCoronaNotificationHook(false, &notification).tail_jump ==
         profile.notification_finalize);

  CoronaSmcState command;
  command.state = 0xff;
  command.counter = 8;
  plan = RunCoronaCommandHook(0xa0, 4, 0, 0, &command);
  assert(command.state == 0xff && command.counter == 0 &&
         plan.calls[0] == profile.command_setup_call &&
         plan.calls[1] == profile.command_query_call);
  RunCoronaCommandHook(0xa0, 2, 0, 0, &command);
  assert((command.state & 2u) == 0);
  RunCoronaCommandHook(0xa0, 5, 0, 0, &command);
  assert((command.state & 4u) == 0);
  assert(RunCoronaCommandHook(0x9f, 0, 0, 0, &command).tail_jump ==
         profile.passthrough_tail);
  command.state = 1;
  command.e3 = 0x17;
  command.e4 = 0xff;
  plan = RunCoronaCommandHook(0xa0, 6, 0x34, 1, &command);
  assert(plan.wait_for_ee_clear && plan.acknowledge_ee &&
         plan.transfer_performed && plan.call_count == 3);
  assert(command.ea == 0xff && command.eb == 0xff && command.ec == 0xff &&
         command.ed == 0xff && command.e3 == 0xd3 && command.e4 == 0xfe &&
         command.f5 == 0x34 && !command.p2_bit_6);

  CoronaSmcState read;
  read.bit_27_3 = true;
  constexpr std::array<std::uint8_t, 8> kExternal{0, 1, 2, 3, 4, 5, 6, 7};
  plan = RunCoronaEightByteReadHook(0x30, 0x80, kExternal, &read);
  assert(plan.call_count == 3 && plan.external_base == 0x80 &&
         plan.emit_call == profile.notification_emit_call &&
         plan.emitted_count == 8 && plan.emitted[7] == 7);
  RunCoronaEightByteReadHook(0x31, 0, kExternal, &read);
  assert(!read.bit_27_3);

  CoronaSmcState status;
  status.state = 0xff;
  plan = RunCoronaStatusHook(true, &status);
  assert(status.state == 0xef && plan.calls[0] == profile.status_call &&
         plan.tail_jump == profile.status_tail);
  status.state = 0xff;
  plan = RunCoronaResetHook(&status);
  assert(status.state == 0xfd && plan.calls[0] == profile.reset_call &&
         plan.tail_jump == profile.reset_tail);
  plan = PlanCoronaTickWrapper();
  assert(plan.calls[0] == profile.tick_call &&
         plan.calls[1] == profile.sender_entry &&
         plan.tail_jump == profile.tick_entry);

  CoronaSmcState receive;
  RunCoronaXbppReceive({'X', 'B', 'P', 'P'}, 0x44, 0x55, &receive);
  assert(receive.r4_copy == 0x44 && receive.r5_copy == 0x55 &&
         receive.state == 1 && receive.eb == 1 && receive.ea == 0x1c &&
         (receive.staged == std::array<std::uint8_t, 3>{'P', 'B', 'X'}));

  CoronaSmcState sender;
  sender.state = 2;
  sender.counter = 1;
  sender.staged = {3, 2, 1};
  sender.e3 = 1;
  sender.e4 = 0xff;
  plan = RunCoronaSenderTick(1, &sender);
  assert(plan.wait_for_ee_clear && plan.acknowledge_ee &&
         plan.transfer_performed && !plan.finalized);
  assert(sender.eb == 3 && sender.ec == 2 && sender.ed == 1 &&
         sender.e3 == 0xc1 && sender.e4 == 0xfe && sender.f5 == 6 &&
         sender.counter == 2 && !sender.p2_bit_6);

  CoronaSmcState timeout;
  timeout.state = 4;
  timeout.counter = 0x13;
  timeout.e3 = 1;
  timeout.e4 = 0xff;
  timeout.bit_22_4 = true;
  timeout.bit_22_6 = true;
  timeout.bit_22_7 = true;
  plan = RunCoronaSenderTick(1, &timeout);
  assert(plan.finalized && plan.wait_for_ee_clear && plan.acknowledge_ee &&
         plan.f5_write_count == 3 &&
         (plan.f5_writes == std::array<std::uint8_t, 3>{0x55, 0xaa, 0x08}));
  assert(timeout.ea == 0 && timeout.eb == 0x40 && timeout.ec == 9 &&
         timeout.ed == 0 && timeout.e3 == 0xc9 && timeout.e4 == 0xff &&
         timeout.f5 == 8 && timeout.state == 0 && timeout.bit_22_5 &&
         !timeout.bit_22_4 && !timeout.bit_22_6 && !timeout.bit_22_7 &&
         timeout.boot_latch == 1 && timeout.final_ram == 0);

  CoronaSmcState wrapped;
  wrapped.state = 0x42;
  wrapped.counter = 0xff;
  wrapped.staged = {3, 2, 1};
  plan = RunCoronaSenderTick(0, &wrapped);
  assert(plan.finalized && wrapped.state == 0 && !wrapped.p2_bit_6);

  std::cout << "Corona SMC runtime reconstruction tests passed\n";
}
