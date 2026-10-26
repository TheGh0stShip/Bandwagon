#include "resident_runtime.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <map>
#include <tuple>
#include <utility>
#include <vector>

namespace {

class MockPlatform final : public bandwagon::stage4::ResidentPlatform {
 public:
  std::uint32_t BranchOriginalDispatcher(
      std::uint64_t r3, std::uint64_t r4, std::uint64_t r5,
      std::uint64_t r6, std::uint64_t r7) override {
    original = {r3, r4, r5, r6, r7};
    return 0x11;
  }
  std::uint32_t ReadWord(std::uint64_t address) override {
    return memory[address];
  }
  void WriteWord(std::uint64_t address, std::uint32_t value) override {
    memory[address] = value;
    writes.emplace_back(address, value);
  }
  void StoreDataCacheBlock(std::uint64_t address) override {
    dcbst.push_back(address);
  }
  void InvalidateInstructionCacheBlock(std::uint64_t address) override {
    icbi.push_back(address);
  }
  void SynchronizeInstructionStream() override { synchronized = true; }
  std::uint32_t Execute(std::uint64_t address) override {
    executed = address;
    return 0x22;
  }
  std::uint32_t BranchOperation5(std::uint64_t r3, std::uint64_t r4,
                                 std::uint64_t r5) override {
    op5 = {r3, r4, r5};
    return 0x33;
  }
  std::uint64_t CallB3d8(std::uint64_t r3, std::uint64_t r4,
                         std::uint64_t r5, std::uint64_t r6) override {
    first = {r3, r4, r5, r6};
    return first_result;
  }
  void CallB388(std::uint64_t r3, std::uint64_t r4) override {
    second = {r3, r4};
    made_second = true;
  }
  std::uint32_t BranchContinuationExit(std::uint64_t address) override {
    continuation_exit = address;
    return 0x44;
  }

  std::tuple<std::uint64_t, std::uint64_t, std::uint64_t, std::uint64_t,
             std::uint64_t>
      original{};
  std::map<std::uint64_t, std::uint32_t> memory;
  std::vector<std::pair<std::uint64_t, std::uint32_t>> writes;
  std::vector<std::uint64_t> dcbst;
  std::vector<std::uint64_t> icbi;
  bool synchronized = false;
  std::uint64_t executed = 0;
  std::tuple<std::uint64_t, std::uint64_t, std::uint64_t> op5{};
  std::tuple<std::uint64_t, std::uint64_t, std::uint64_t, std::uint64_t>
      first{};
  std::uint64_t first_result = 1;
  std::pair<std::uint64_t, std::uint64_t> second{};
  bool made_second = false;
  std::uint64_t continuation_exit = 0;
};

}  // namespace

int main() {
  using bandwagon::stage4::RunCopiedContinuation;
  using bandwagon::stage4::RunRbtrDispatcher;
  using bandwagon::stage4::CopiedDispatcherTarget;
  using bandwagon::stage4::SelectPostOutputTarget;
  assert(SelectPostOutputTarget(0x41414141, 0x1234) == 0x1234);
  assert(SelectPostOutputTarget(0, 0x1234) == 0x2540);
  assert(CopiedDispatcherTarget() == 0x600031fb8ULL);
  MockPlatform platform;
  assert(RunRbtrDispatcher(0, 4, 1, 2, 3, &platform) == 0x11);
  assert(RunRbtrDispatcher(0x72627472, 2, 0, 0, 0, &platform) == 1);
  assert(RunRbtrDispatcher(0x72627472, 8, 0, 0, 0, &platform) == 2);

  platform.memory[0x2000] = 0x11223344;
  platform.memory[0x2004] = 0x55667788;
  assert(RunRbtrDispatcher(0x72627472, 4, 0x1000, 0x2000, 2,
                           &platform) == 0x22);
  assert(platform.memory[0x1000] == 0x11223344);
  assert(platform.memory[0x1004] == 0x55667788);
  assert(platform.dcbst == std::vector<std::uint64_t>({0x1000, 0x1004}));
  assert(platform.icbi == platform.dcbst && platform.synchronized);
  assert(platform.executed == 0x1000);

  assert(RunRbtrDispatcher(0x72627472, 5, 0x50, 0x60, 0x70,
                           &platform) == 0x33);
  assert(platform.op5 ==
         (std::tuple<std::uint64_t, std::uint64_t, std::uint64_t>{
             0x60, 0x50, 0x70}));

  assert(RunCopiedContinuation(0x1000, 0x2000, 0x28, 1, 0x30,
                               &platform) == 0x44);
  assert(platform.first ==
         (std::tuple<std::uint64_t, std::uint64_t, std::uint64_t,
                     std::uint64_t>{0x1050, 0x1150, 0x30, 0x6000320b8ULL}));
  assert(platform.made_second && platform.second.first == 0x28);
  assert(platform.second.second == 0x6000321c8ULL);
  assert(platform.continuation_exit == 0x400029b3cULL);
  assert(RunRbtrDispatcher(0, 0, 0, 0, 0, nullptr) == 0);
  assert(RunCopiedContinuation(0, 0, 0, 0, 0, nullptr) == 0);

  std::cout << "Stage4 resident-runtime reconstruction tests passed\n";
}
