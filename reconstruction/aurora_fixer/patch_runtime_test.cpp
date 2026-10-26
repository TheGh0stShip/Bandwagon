#include "patch_runtime.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace {

class MockPlatform final
    : public bandwagon::aurora_fixer::PatchRuntimePlatform {
 public:
  void ReadMemory(std::uint32_t address, std::uint8_t* output,
                  std::size_t size) override {
    calls.push_back("read");
    assert(address == base && size == 16);
    std::copy(memory.begin(), memory.end(), output);
  }
  void WriteMemory(std::uint32_t address, const std::uint8_t* data,
                   std::size_t size) override {
    calls.push_back("write");
    assert(address == base && size == 16);
    std::copy(data, data + size, memory.begin());
  }
  std::uint32_t MmGetPhysicalAddress(std::uint32_t address) override {
    calls.push_back("physical");
    assert(address == base);
    return 0x1234;
  }
  void KeFlushCacheRange(std::uint32_t address,
                         std::uint32_t size) override {
    calls.push_back("kernel_flush");
    assert(address == base && size == 16);
  }
  void HvxFlushDcacheRange(std::uint32_t address,
                           std::uint32_t size) override {
    calls.push_back("hv_flush");
    assert(address == base && size == 16);
  }

  std::uint32_t base = 0x82000000;
  std::array<std::uint8_t, 16> memory{};
  std::vector<std::string> calls;
};

}  // namespace

int main() {
  using bandwagon::aurora_fixer::PatchAbsoluteBranch;
  MockPlatform platform;
  for (std::size_t i = 0; i < platform.memory.size(); ++i)
    platform.memory[i] = static_cast<std::uint8_t>(i);
  std::array<std::uint8_t, 16> saved{};
  PatchAbsoluteBranch(platform.base, 0x89abcdef, true, &saved, &platform);
  for (std::size_t i = 0; i < saved.size(); ++i)
    assert(saved[i] == static_cast<std::uint8_t>(i));
  assert(platform.memory[0] == 0x3d && platform.memory[1] == 0x60);
  assert(platform.memory[12] == 0x4e && platform.memory[15] == 0x21);
  assert((platform.calls == std::vector<std::string>{
                                "read", "write", "physical", "kernel_flush",
                                "hv_flush"}));

  MockPlatform no_save;
  PatchAbsoluteBranch(no_save.base, 0x1000, false, nullptr, &no_save);
  assert(no_save.calls.front() == "write");
  assert(no_save.memory[15] == 0x20);
  PatchAbsoluteBranch(0, 0, false, nullptr, nullptr);
  std::cout << "AuroraFixer patch-runtime reconstruction tests passed\n";
}
