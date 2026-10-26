#pragma once

#include <array>
#include <cstdint>

namespace bandwagon::stage3 {

constexpr std::uint32_t kProbeReturnMarker = 0x41410001u;

struct HypervisorCall {
  std::uint32_t syscall_ordinal;
  std::array<std::uint64_t, 6> arguments;  // r3 through r8
};

struct ResidentProbePlan {
  std::uint64_t probe_alias;
  std::uint64_t output_alias;
  HypervisorCall hvx_post_output;
  HypervisorCall rbtr;
};

ResidentProbePlan BuildResidentProbePlan(std::uint32_t physical_address);
std::array<std::uint64_t, 12> BuildProbeSampleAddresses();

class HypervisorInvoker {
 public:
  virtual ~HypervisorInvoker() = default;
  virtual std::uint32_t Invoke(const HypervisorCall& call) = 0;
};

// Models the two invocation attempts after Stage3 has copied the 0x38-byte
// probe into physical memory. The rbtr call is skipped when HvxPostOutput
// returns the success marker.
bool ProbeResidentPeerDispatcher(std::uint32_t physical_address,
                                 HypervisorInvoker& invoker);

}  // namespace bandwagon::stage3
