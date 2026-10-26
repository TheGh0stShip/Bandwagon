#include "entry_flow.hpp"

namespace bandwagon::stage3 {
namespace {

void AddFinalization(Stage3EntryPlan* plan) {
  plan->send_smc_message = true;
  plan->smc_command = 0x99;
  plan->launch_default = true;
}

}  // namespace

Stage3EntryPlan BuildStage3EntryPlan(bool resident_dispatcher_available,
                                     bool stage4_read_succeeded,
                                     std::uint32_t stage4_result) {
  Stage3EntryPlan plan;
  if (resident_dispatcher_available) {
    AddFinalization(&plan);
    return plan;
  }

  plan.run_badupdate_exploit = true;
  plan.load_stage4 = true;
  if (!stage4_read_succeeded) {
    plan.fatal_code = kStage4ReadFatalCode;
    return plan;
  }

  plan.execute_stage4 = true;
  if (stage4_result != kStage4SuccessMarker) {
    plan.fatal_code = kStage4ExecuteFatalCode;
    return plan;
  }

  plan.free_stage4 = true;
  AddFinalization(&plan);
  return plan;
}

}  // namespace bandwagon::stage3
