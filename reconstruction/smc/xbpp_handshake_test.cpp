#include "xbpp_handshake.hpp"

#include <array>
#include <cassert>
#include <iostream>

int main() {
  using bandwagon::smc::Motherboard;
  using bandwagon::smc::PlanXbppHandshake;
  using bandwagon::smc::PlanXbppTransfer;
  constexpr std::array<std::uint8_t, 4> kMagic{'X', 'B', 'P', 'P'};
  constexpr std::array<Motherboard, 6> kBoards{
      Motherboard::kCorona, Motherboard::kFalcon, Motherboard::kJasper,
      Motherboard::kTrinity, Motherboard::kXenon, Motherboard::kZephyr};

  for (const auto board : kBoards) {
    const auto plan = PlanXbppHandshake(board, kMagic, 0xa4);
    assert(plan.recognized);
    assert(plan.initial_eb == 1 && plan.initial_ea == 0x10 &&
           plan.initial_f5 == 0);
    assert(plan.state_value == 0xa5);
    assert((plan.staged_bytes == std::array<std::uint8_t, 3>{'P', 'B', 'X'}));
    assert(plan.transfer_ea == 0x18 && plan.transfer_f5 == 0);
  }

  assert(PlanXbppHandshake(Motherboard::kCorona, kMagic, 0)
             .state_ram_address == 0x3a);
  assert(PlanXbppHandshake(Motherboard::kTrinity, kMagic, 0)
             .state_ram_address == 0x3a);
  assert(PlanXbppHandshake(Motherboard::kFalcon, kMagic, 0)
             .state_ram_address == 0x3c);
  assert(PlanXbppHandshake(Motherboard::kXenon, kMagic, 0)
             .staging_ram_address == 0xb1);
  assert(PlanXbppHandshake(Motherboard::kJasper, kMagic, 0)
             .staging_ram_address == 0x31);

  const auto trinity = PlanXbppHandshake(Motherboard::kTrinity, kMagic, 0);
  assert(!trinity.rewrite_eb_for_transfer);
  const auto corona = PlanXbppHandshake(Motherboard::kCorona, kMagic, 0);
  assert(corona.rewrite_eb_for_transfer);
  assert(corona.corona_followup_transfer && corona.corona_followup_ea == 0x1c);

  constexpr std::array<std::uint8_t, 4> kWrong{'X', 'B', 'P', 'Q'};
  const auto rejected =
      PlanXbppHandshake(Motherboard::kZephyr, kWrong, 0xff);
  assert(!rejected.recognized);
  assert(rejected.state_value == 0xfe);
  assert(rejected.transfer_ea == 0);

  constexpr std::array<std::uint8_t, 3> kStaged{0x50, 0x42, 0x58};
  assert(!PlanXbppTransfer(Motherboard::kFalcon, 0, 0, 0, 0, kStaged)
              .transfer);
  assert(!PlanXbppTransfer(Motherboard::kFalcon, 2, 1, 0, 0, kStaged)
              .transfer);
  const auto transfer = PlanXbppTransfer(Motherboard::kFalcon, 2, 0, 0x15,
                                         0xff, kStaged);
  assert(transfer.transfer);
  assert(transfer.ea == 0);
  assert(transfer.eb == 0x50 && transfer.ec == 0x42 && transfer.ed == 0x58);
  assert(transfer.e3 == 0xd5);
  assert(transfer.e4 == 0xfe);
  assert(transfer.f5 == 6);
  const auto corona_busy = PlanXbppTransfer(Motherboard::kCorona, 2, 1, 0x15,
                                            0xff, kStaged);
  assert(corona_busy.transfer && corona_busy.wait_for_ee_clear);
  assert(corona_busy.set_p2_bit_6);

  std::cout << "SMC XBPP-handshake reconstruction tests passed\n";
}
