#include "xbpp_handshake.hpp"

namespace bandwagon::smc {

XbppHandshakePlan PlanXbppHandshake(
    Motherboard board, const std::array<std::uint8_t, 4>& inbound,
    std::uint8_t state_before) {
  XbppHandshakePlan plan;
  plan.state_ram_address =
      (board == Motherboard::kCorona || board == Motherboard::kTrinity) ? 0x3a
                                                                        : 0x3c;
  plan.state_value = state_before & 0xfeu;
  plan.staging_ram_address = board == Motherboard::kXenon ? 0xb1 : 0x31;
  plan.recognized = inbound == std::array<std::uint8_t, 4>{'X', 'B', 'P', 'P'};
  if (!plan.recognized) return plan;

  plan.staged_bytes = {inbound[2], inbound[1], inbound[0]};
  plan.state_value |= 1;
  plan.rewrite_eb_for_transfer = board != Motherboard::kTrinity;
  plan.transfer_ea = 0x18;
  plan.transfer_f5 = 0;
  if (board == Motherboard::kCorona) {
    plan.corona_followup_transfer = true;
    plan.corona_followup_ea = 0x1c;
  }
  return plan;
}

XbppTransferPlan PlanXbppTransfer(
    Motherboard board, std::uint8_t state, std::uint8_t ee, std::uint8_t e3,
    std::uint8_t e4, const std::array<std::uint8_t, 3>& staged_bytes) {
  XbppTransferPlan plan;
  if ((state & 2u) == 0) return plan;
  if (board == Motherboard::kCorona) {
    plan.wait_for_ee_clear = (ee & 1u) != 0;
    plan.set_p2_bit_6 = true;
  } else if ((ee & 1u) != 0) {
    return plan;
  }
  plan.transfer = true;
  plan.ea = 0;
  plan.eb = staged_bytes[0];
  plan.ec = staged_bytes[1];
  plan.ed = staged_bytes[2];
  plan.e3 = e3 | 0xc0u;
  plan.e4 = static_cast<std::uint8_t>((e4 & 0xfcu) | 2u);
  plan.f5 = 6;
  return plan;
}

}  // namespace bandwagon::smc
