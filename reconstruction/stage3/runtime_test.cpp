#include "runtime.hpp"

#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace {

class MockPlatform final : public bandwagon::stage3::Platform {
 public:
  std::uint64_t AllocatePhysicalMemory(std::uint32_t flags, std::uint32_t size,
                                       std::uint32_t protection,
                                       std::uint32_t minimum,
                                       std::uint32_t maximum,
                                       std::uint32_t alignment) override {
    allocation_arguments = {flags, size, protection, minimum, maximum,
                            alignment};
    return probe_allocation;
  }
  void ZeroMemory(std::uint64_t address, std::size_t size) override {
    zeroed = {address, size};
  }
  void InstallResidentProbe(std::uint64_t address, const std::uint8_t* data,
                            std::size_t size) override {
    probe_installed_at = address;
    installed_probe.assign(data, data + size);
  }
  std::uint32_t GetPhysicalAddress(std::uint64_t address) override {
    return address == probe_allocation ? probe_physical : stage4_physical;
  }
  void SynchronizeCode() override { synchronized = true; }
  void FreePhysicalMemory(std::uint32_t flags, std::uint64_t address) override {
    frees.emplace_back(flags, address);
  }
  std::uint32_t Invoke(const bandwagon::stage3::HypervisorCall& call) override {
    calls.push_back(call);
    if (call.arguments[0] == 0x41414141u ||
        call.arguments[0] == 0x72627472u) {
      return probe_result;
    }
    return stage4_result;
  }
  void RunBadUpdateExploit(std::uint32_t argument) override {
    exploit_argument = argument;
  }
  bool ReadFileIntoPhysicalMemory(std::string_view path,
                                  std::uint64_t* allocation,
                                  std::uint32_t* size) override {
    read_path = std::string(path);
    *allocation = stage4_allocation;
    *size = stage4_size;
    return read_succeeds;
  }
  void DisplayFatalError(std::uint32_t code) override { fatal = code; }
  void SendSmcMessage(
      const std::array<std::uint8_t, 16>& message) override {
    smc = message;
    sent_smc = true;
  }
  void LaunchTitle(std::string_view path) override {
    launched = std::string(path);
  }

  std::uint64_t probe_allocation = 0x100000;
  std::uint32_t probe_physical = 0x200000;
  std::uint32_t probe_result = 0x41410001;
  bool read_succeeds = true;
  std::uint64_t stage4_allocation = 0x300000;
  std::uint32_t stage4_physical = 0x400000;
  std::uint32_t stage4_size = 0x10780;
  std::uint32_t stage4_result = 0x41414141;
  std::array<std::uint32_t, 6> allocation_arguments{};
  std::pair<std::uint64_t, std::size_t> zeroed{};
  std::uint64_t probe_installed_at = 0;
  std::vector<std::uint8_t> installed_probe;
  bool synchronized = false;
  std::vector<std::pair<std::uint32_t, std::uint64_t>> frees;
  std::vector<bandwagon::stage3::HypervisorCall> calls;
  std::uint32_t exploit_argument = 0;
  std::string read_path;
  std::uint32_t fatal = 0;
  bool sent_smc = false;
  std::array<std::uint8_t, 16> smc{};
  std::string launched;
};

}  // namespace

int main() {
  using bandwagon::stage3::RunStage3;
  MockPlatform resident;
  const auto fast = RunStage3(&resident);
  assert(fast.completed && fast.used_resident_dispatcher);
  assert(!fast.installed_stage4 && resident.exploit_argument == 0);
  const std::array<std::uint32_t, 6> allocation_args{
      1, 0x1000, 0x204, 0, 0xffffffffu, 0x80};
  assert(resident.allocation_arguments == allocation_args);
  assert(resident.zeroed.first == resident.probe_allocation &&
         resident.zeroed.second == 0x1000);
  assert(resident.probe_installed_at == resident.probe_allocation);
  assert(resident.installed_probe.size() == 0x38);
  assert(resident.installed_probe.front() == 0x3c);
  assert(resident.installed_probe.back() == 0x20);
  assert(resident.synchronized && resident.frees.size() == 1);
  assert(resident.sent_smc && resident.smc[0] == 0x99);
  assert(resident.launched == "PAYLOAD:\\default.xex");

  MockPlatform install;
  install.probe_result = 0;
  const auto installed = RunStage3(&install);
  assert(installed.completed && installed.installed_stage4);
  assert(install.calls.size() == 3);
  assert(install.calls[2].syscall_ordinal == 0x0d);
  assert(install.calls[2].arguments[0] == 0);
  assert(install.calls[2].arguments[1] == 0x8000000000400000ULL);
  assert(install.frees.size() == 2);
  assert(install.read_path == "PAYLOAD:\\BadUpdateExploit-4thStage.bin");

  MockPlatform read_failure;
  read_failure.probe_result = 0;
  read_failure.read_succeeds = false;
  const auto failed_read = RunStage3(&read_failure);
  assert(!failed_read.completed && failed_read.fatal_code == 0x12414);
  assert(read_failure.fatal == 0x12414 && !read_failure.sent_smc);

  MockPlatform execute_failure;
  execute_failure.probe_result = 0;
  execute_failure.stage4_result = 0;
  const auto failed_execute = RunStage3(&execute_failure);
  assert(!failed_execute.completed && failed_execute.fatal_code == 0x12415);
  assert(execute_failure.frees.size() == 1);

  MockPlatform allocation_failure;
  allocation_failure.probe_allocation = 0;
  const auto recovered = RunStage3(&allocation_failure);
  assert(recovered.completed && recovered.installed_stage4);
  assert(!RunStage3(nullptr).completed);
  std::cout << "Stage3 runtime reconstruction tests passed\n";
}
