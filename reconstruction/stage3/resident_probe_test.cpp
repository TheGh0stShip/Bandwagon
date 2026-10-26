#include "resident_probe.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

namespace {

class FakeInvoker final : public bandwagon::stage3::HypervisorInvoker {
 public:
  std::uint32_t Invoke(const bandwagon::stage3::HypervisorCall& call) override {
    calls.push_back(call);
    return results[calls.size() - 1];
  }
  std::vector<std::uint32_t> results;
  std::vector<bandwagon::stage3::HypervisorCall> calls;
};

}  // namespace

int main() {
  using bandwagon::stage3::BuildProbeSampleAddresses;
  using bandwagon::stage3::BuildResidentProbePlan;
  using bandwagon::stage3::ProbeResidentPeerDispatcher;

  const auto plan = BuildResidentProbePlan(0x1c100000u);
  assert(plan.probe_alias == 0x800000001c100000ULL);
  assert(plan.output_alias == 0x800000001c100100ULL);
  assert(plan.hvx_post_output.syscall_ordinal == 0x0d);
  assert((plan.hvx_post_output.arguments ==
          std::array<std::uint64_t, 6>{0x41414141u, plan.probe_alias, 0, 0, 0,
                                       plan.output_alias}));
  assert(plan.rbtr.syscall_ordinal == 0);
  assert((plan.rbtr.arguments ==
          std::array<std::uint64_t, 6>{0x72627472u, 4, plan.probe_alias,
                                       plan.probe_alias, 1,
                                       plan.output_alias}));

  const auto samples = BuildProbeSampleAddresses();
  assert(samples.front() == 0x8000020000020000ULL);
  assert(samples.back() == 0x8000020000021600ULL);
  for (std::size_t i = 1; i < samples.size(); ++i) {
    assert(samples[i] - samples[i - 1] == 0x200);
  }

  FakeInvoker first;
  first.results = {0x41410001u};
  assert(ProbeResidentPeerDispatcher(0x1000u, first));
  assert(first.calls.size() == 1);
  FakeInvoker fallback;
  fallback.results = {0, 0x41410001u};
  assert(ProbeResidentPeerDispatcher(0x1000u, fallback));
  assert(fallback.calls.size() == 2);
  FakeInvoker absent;
  absent.results = {0, 0};
  assert(!ProbeResidentPeerDispatcher(0x1000u, absent));
  assert(absent.calls.size() == 2);

  std::cout << "Stage3 resident-probe reconstruction tests passed\n";
}
