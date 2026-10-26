#include "trinity_runtime.hpp"

#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>

int main() {
  using namespace bandwagon::smc;

  TrinityState boot;
  boot.state_3a = 0xff;
  auto hook = RunTrinityBootHook(4, &boot);
  assert(boot.ram_3b == 1 && boot.state_3a == 0xff);
  assert(hook.tail_jump == 0x0c19);
  boot.state_3a = 0xff;
  RunTrinityBootHook(0, &boot);
  assert(boot.state_3a == 0xe7);

  TrinityState command;
  command.state_3a = 0xff;
  command.counter_30 = 7;
  hook = RunTrinityCommandHook(0xa0, 1, &command);
  assert(hook.call_count == 2 && hook.calls[0] == 0x25a1 &&
         hook.calls[1] == 0x25a6);
  assert(command.state_3a == 0x39 && command.counter_30 == 0);
  command.state_3a = 0xf0;
  RunTrinityCommandHook(0xa0, 3, &command);
  assert(command.state_3a == 0xf4);
  assert(RunTrinityCommandHook(0x9f, 0, &command).tail_jump == 0x0ddb);

  TrinityState status;
  status.state_3a = 0xff;
  hook = RunTrinityStatusHook(true, &status);
  assert(status.state_3a == 9 && hook.calls[0] == 0x2768 &&
         hook.tail_jump == 0x2067);
  assert(PlanTrinityTickWrapper().tail_jump == 0x2fb6);

  TrinityState notify;
  notify.state_3a = 8;
  hook = RunTrinityNotificationHook(&notify);
  assert(hook.call_count == 2 && hook.calls[0] == 0x25c8 &&
         hook.call_accumulators[0] == 0 && hook.call_accumulators[1] == 1);
  assert(notify.bit_22_3);
  notify.state_3a |= 2;
  assert(RunTrinityNotificationHook(&notify).tail_jump == 0x2fdc);

  TrinityState gate;
  gate.state_3a = 1;
  gate.counter_30 = 9;
  assert(RunTrinityStateGate(&gate).tail_jump == 0x0c7a);
  assert(gate.state_3a == 0x13 && gate.counter_30 == 0);
  TrinityState suppressed;
  suppressed.state_3a = 9;
  suppressed.ram_5f = 0x11;
  RunTrinityStateGate(&suppressed);
  assert(suppressed.state_3a == 0x19);
  assert(RunTrinityResetHook(&suppressed).tail_jump == 0x2794);
  assert(suppressed.state_3a == 0);

  TrinityState receive;
  receive.state_3a = 0xaa;
  RunTrinityXbppReceive({'X', 'B', 'P', 'P'}, &receive);
  assert(receive.ram_54 == 'P' && receive.state_3a == 0xab);
  assert((receive.staged == std::array<std::uint8_t, 3>{'P', 'B', 'X'}));
  assert(receive.eb == 1 && receive.ea == 0x18 && receive.f5 == 0);
  RunTrinityXbppReceive({'N', 'O', 'P', 'E'}, &receive);
  assert(receive.state_3a == 0xaa && receive.ea == 0x10);

  TrinityState redirect;
  redirect.state_3a = 0xff;
  assert(RunTrinityEeRedirect(1, &redirect).tail_jump == 0x00b0);
  assert(redirect.state_3a == 1);
  assert(RunTrinityEeRedirect(0, &redirect).tail_jump == 0x1280);

  TrinityState sender;
  sender.state_3a = 2;
  sender.staged = {3, 2, 1};
  sender.e3 = 1;
  sender.e4 = 0xff;
  hook = RunTrinitySender(0, &sender);
  assert(hook.calls[0] == 0x1b2c);
  assert(sender.ea == 0 && sender.eb == 3 && sender.ec == 2 &&
         sender.ed == 1 && sender.e3 == 0xc1 && sender.e4 == 0xfe &&
         sender.f5 == 6);
  TrinityState busy;
  busy.state_3a = 2;
  busy.f5 = 9;
  RunTrinitySender(1, &busy);
  assert(busy.f5 == 9);

  TrinityState short_timeout;
  short_timeout.state_3a = 0x24;
  short_timeout.counter_30 = 0x1d;
  short_timeout.bit_22_3 = true;
  short_timeout.bit_22_5 = true;
  short_timeout.bit_22_6 = true;
  RunTrinityTick(&short_timeout);
  assert(short_timeout.state_3a == 0 && short_timeout.counter_30 == 0x1e);
  assert(short_timeout.bit_22_4 && !short_timeout.bit_22_3 &&
         !short_timeout.bit_22_5 && !short_timeout.bit_22_6);
  assert(short_timeout.ram_3b == 1 && short_timeout.ram_60 == 0);

  TrinityState wrapped;
  wrapped.state_3a = 0x02;
  wrapped.counter_30 = 0xff;
  RunTrinityTick(&wrapped);
  assert(wrapped.state_3a == 0x42 && wrapped.counter_30 == 0);
  wrapped.state_3a = 0x42;
  wrapped.counter_30 = 0xff;
  RunTrinityTick(&wrapped);
  assert(wrapped.state_3a == 0 && wrapped.bit_22_4);

  RunTrinityBootHook(0, nullptr);
  RunTrinityTick(nullptr);
  std::cout << "Trinity SMC runtime reconstruction tests passed\n";
}
