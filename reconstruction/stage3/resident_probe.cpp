#include "resident_probe.hpp"

namespace bandwagon::stage3 {

ResidentProbePlan BuildResidentProbePlan(std::uint32_t physical_address) {
  const std::uint64_t alias = 0x8000000000000000ULL | physical_address;
  const std::uint64_t output = alias + 0x100;
  return {
      alias,
      output,
      {0x0d, {0x41414141u, alias, 0, 0, 0, output}},
      {0x00, {0x72627472u, 4, alias, alias, 1, output}},
  };
}

std::array<std::uint64_t, 12> BuildProbeSampleAddresses() {
  std::array<std::uint64_t, 12> addresses{};
  std::uint64_t address = 0x8000020000020000ULL;
  for (auto& item : addresses) {
    item = address;
    address += 0x200;
  }
  return addresses;
}

bool ProbeResidentPeerDispatcher(std::uint32_t physical_address,
                                 HypervisorInvoker& invoker) {
  const ResidentProbePlan plan = BuildResidentProbePlan(physical_address);
  if (invoker.Invoke(plan.hvx_post_output) == kProbeReturnMarker) return true;
  return invoker.Invoke(plan.rbtr) == kProbeReturnMarker;
}

}  // namespace bandwagon::stage3
