#include "instruction_patch.hpp"

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

namespace {

class FakePatchServices final
    : public bandwagon::kernel_expansion::InstructionPatchServices {
 public:
  const void* expected_address = nullptr;
  std::uintptr_t physical_address = 0x12345000;
  std::vector<std::string> events;

  void FlushCpuCache(const void* address, std::size_t size) override {
    assert(address == expected_address && size == 4);
    events.emplace_back("cpu-flush");
  }

  std::uintptr_t GetPhysicalAddress(const void* address) override {
    assert(address == expected_address);
    events.emplace_back("translate");
    return physical_address;
  }

  void FlushHypervisorDataCache(std::uintptr_t address,
                                std::size_t size) override {
    assert(address == physical_address && size == 4);
    events.emplace_back("hypervisor-flush");
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

  std::cout << "Kernel Expansion instruction-patch tests passed\n";
}
