#include "runtime.hpp"

#include "entry_flow.hpp"
#include "probe_payload.hpp"

namespace bandwagon::stage3 {
namespace {

bool ProbeResident(Platform* platform) {
  const std::uint64_t allocation = platform->AllocatePhysicalMemory(
      1, 0x1000, 0x204, 0, 0xffffffffu, 0x80);
  if (allocation == 0) return false;
  platform->ZeroMemory(allocation, 0x1000);
  platform->InstallResidentProbe(allocation, kResidentProbePayload.data(),
                                 kResidentProbePayload.size());
  platform->SynchronizeCode();
  const bool available = ProbeResidentPeerDispatcher(
      platform->GetPhysicalAddress(allocation), *platform);
  platform->FreePhysicalMemory(1, allocation);
  return available;
}

RuntimeResult Fatal(Platform* platform, std::uint32_t code) {
  platform->DisplayFatalError(code);
  return {false, code, false, false};
}

}  // namespace

RuntimeResult RunStage3(Platform* platform) {
  if (!platform) return {};
  const bool resident = ProbeResident(platform);
  bool installed_stage4 = false;
  if (!resident) {
    platform->RunBadUpdateExploit(1);
    std::uint64_t stage4 = 0;
    std::uint32_t stage4_size = 0;
    if (!platform->ReadFileIntoPhysicalMemory(kStage4Path, &stage4,
                                              &stage4_size)) {
      return Fatal(platform, kStage4ReadFatalCode);
    }
    const std::uint64_t physical_alias =
        0x8000000000000000ULL | platform->GetPhysicalAddress(stage4);
    const HypervisorCall execute_stage4{0x0d,
                                        {0, physical_alias, 0, 0, 0, 0}};
    if (platform->Invoke(execute_stage4) != kStage4SuccessMarker) {
      return Fatal(platform, kStage4ExecuteFatalCode);
    }
    platform->FreePhysicalMemory(1, stage4);
    installed_stage4 = true;
  }
  std::array<std::uint8_t, 16> smc_message{};
  smc_message[0] = 0x99;
  platform->SendSmcMessage(smc_message);
  platform->LaunchTitle(kDefaultPayloadPath);
  return {true, 0, resident, installed_stage4};
}

}  // namespace bandwagon::stage3
