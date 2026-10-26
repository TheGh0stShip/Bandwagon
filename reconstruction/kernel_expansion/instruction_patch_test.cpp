#include "instruction_patch.hpp"

#include <array>
#include <cassert>
#include <iostream>
#include <string>
#include <vector>

namespace {

class FakePatchServices final
    : public bandwagon::kernel_expansion::InstructionPatchServices {
 public:
  const void* expected_address = nullptr;
  std::size_t expected_size = 4;
  std::uintptr_t physical_address = 0x12345000;
  int debug_breaks = 0;
  std::vector<std::string> events;

  void FlushCpuCache(const void* address, std::size_t size) override {
    assert(address == expected_address && size == expected_size);
    events.emplace_back("cpu-flush");
  }

  std::uintptr_t GetPhysicalAddress(const void* address) override {
    assert(address == expected_address);
    events.emplace_back("translate");
    return physical_address;
  }

  void FlushHypervisorDataCache(std::uintptr_t address,
                                std::size_t size) override {
    assert(address == physical_address && size == expected_size);
    events.emplace_back("hypervisor-flush");
  }

  void DebugBreak() override {
    ++debug_breaks;
    events.emplace_back("debug-break");
  }

  void Reset(const void* address, std::size_t size) {
    expected_address = address;
    expected_size = size;
    events.clear();
  }
};

}  // namespace

int main() {
  using namespace bandwagon::kernel_expansion;

  std::uint32_t instruction = 0x48000001;
  FakePatchServices services;
  services.expected_address = &instruction;
  ReplaceInstructionWithNop(&instruction, &services);
  assert(instruction == kPpcNopInstruction);
  assert((services.events == std::vector<std::string>{
      "cpu-flush", "translate", "hypervisor-flush"}));

  ReplaceInstructionWithNop(nullptr, &services);
  ReplaceInstructionWithNop(&instruction, nullptr);
  assert(services.events.size() == 3);
  assert(ReturnImmediatelyHook(0x1234567889abcdefull) ==
         0x1234567889abcdefull);

  std::uint32_t branch_word = 0x60000000u;
  std::uint32_t saved_word = 0;
  services.Reset(&branch_word, 4);
  assert(InstallRelativeBranchHook({0x80000000u, &branch_word}, 0x80000100u,
                                   1, &saved_word, &services));
  assert(saved_word == 0x60000000u);
  assert(branch_word == 0x48000101u);
  assert((services.events == std::vector<std::string>{
      "cpu-flush", "translate", "hypervisor-flush"}));

  services.Reset(&branch_word, 4);
  branch_word = 0;
  assert(InstallRelativeBranchHook({0x80001000u, &branch_word}, 0x80000000u,
                                   2, nullptr, &services));
  assert(branch_word == 0x4bfff000u);

  services.Reset(&branch_word, 4);
  branch_word = 0;
  assert(InstallRelativeBranchHook({0x80000000u, &branch_word}, 0x80000103u,
                                   0x100, nullptr, &services));
  assert(branch_word == 0x48000100u);

  services.Reset(&branch_word, 4);
  assert(InstallRelativeBranchHook({0x80000000u, &branch_word}, 0x81fffffcu,
                                   0x101, nullptr, &services));
  assert(branch_word == 0x49fffffdu);
  services.Reset(&branch_word, 4);
  assert(InstallRelativeBranchHook({0x82000000u, &branch_word}, 0x80000000u,
                                   0, nullptr, &services));
  assert(branch_word == 0x4a000000u);

  branch_word = 0x12345678u;
  saved_word = 0xaaaaaaaau;
  services.Reset(&branch_word, 4);
  assert(!InstallRelativeBranchHook({0x80000000u, &branch_word}, 0x82000000u,
                                    1, &saved_word, &services));
  assert(branch_word == 0x12345678u && saved_word == 0xaaaaaaaau);
  assert((services.events == std::vector<std::string>{"debug-break"}));
  assert(services.debug_breaks == 1);

  services.Reset(&branch_word, 4);
  assert(!InstallRelativeBranchHook({0x82000004u, &branch_word}, 0x80000000u,
                                    0, nullptr, &services));
  assert(services.debug_breaks == 2);
  assert(!InstallRelativeBranchHook({0x80000000u, nullptr}, 0x80000000u, 0,
                                    nullptr, &services));

  std::array<std::uint32_t, 4> stub{
      0x11111111u, 0x22222222u, 0x33333333u, 0x44444444u};
  std::array<std::uint32_t, 4> saved_stub{};
  services.Reset(stub.data(), 16);
  InstallAbsoluteBranchHook({0x80061000u, stub.data()}, 0x801f18e8u, 0,
                            saved_stub.data(), &services);
  assert((saved_stub == std::array<std::uint32_t, 4>{
                            0x11111111u, 0x22222222u, 0x33333333u,
                            0x44444444u}));
  assert((stub == std::array<std::uint32_t, 4>{
                      0x3d60801fu, 0x396b18e8u, 0x7d6903a6u,
                      0x4e800420u}));

  services.Reset(stub.data(), 16);
  InstallAbsoluteBranchHook({0x8007d6e8u, stub.data()}, 0x801f19c8u, 0x101,
                            nullptr, &services);
  assert((stub == std::array<std::uint32_t, 4>{
                      0x3d60801fu, 0x396b19c8u, 0x7d6903a6u,
                      0x4e800421u}));

  services.Reset(stub.data(), 16);
  InstallAbsoluteBranchHook({0x80000000u, stub.data()}, 0x8007d6e8u, 2,
                            nullptr, &services);
  assert((stub == std::array<std::uint32_t, 4>{
                      0x3d608008u, 0x396bd6e8u, 0x7d6903a6u,
                      0x4e800420u}));
  assert((services.events == std::vector<std::string>{
      "cpu-flush", "translate", "hypervisor-flush"}));

  services.Reset(stub.data(), 16);
  InstallAbsoluteBranchHook({0x80000000u, stub.data()}, 0xffff8000u, 0,
                            nullptr, &services);
  assert((stub == std::array<std::uint32_t, 4>{
                      0x3d610000u, 0x396b8000u, 0x7d6903a6u,
                      0x4e800420u}));

  InstallAbsoluteBranchHook({0, nullptr}, 0, 0, nullptr, &services);

  std::cout << "Kernel Expansion instruction-patch tests passed\n";
}
