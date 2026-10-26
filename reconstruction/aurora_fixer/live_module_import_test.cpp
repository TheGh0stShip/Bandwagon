#include "live_dll_main.hpp"

#include "absolute_branch.hpp"
#include "dll_main.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Store16(std::vector<std::uint8_t>* bytes, std::size_t at,
             std::uint16_t value) {
  (*bytes)[at] = static_cast<std::uint8_t>(value >> 8);
  (*bytes)[at + 1] = static_cast<std::uint8_t>(value);
}

void Store32(std::vector<std::uint8_t>* bytes, std::size_t at,
             std::uint32_t value) {
  (*bytes)[at] = static_cast<std::uint8_t>(value >> 24);
  (*bytes)[at + 1] = static_cast<std::uint8_t>(value >> 16);
  (*bytes)[at + 2] = static_cast<std::uint8_t>(value >> 8);
  (*bytes)[at + 3] = static_cast<std::uint8_t>(value);
}

std::vector<std::uint8_t> Descriptor() {
  const char names[] = "xam.xex\0xboxkrnl.exe\0";
  constexpr std::size_t names_size = sizeof(names) - 1;
  std::vector<std::uint8_t> bytes(12 + names_size + 48, 0);
  Store32(&bytes, 0, static_cast<std::uint32_t>(bytes.size()));
  Store32(&bytes, 4, names_size);
  Store32(&bytes, 8, 1);
  std::copy(names, names + names_size, bytes.begin() + 12);
  const std::size_t record = 12 + names_size;
  Store32(&bytes, record, 48);
  Store16(&bytes, record + 36, 1);
  Store16(&bytes, record + 38, 2);
  Store32(&bytes, record + 40, 0x82001000u);
  Store32(&bytes, record + 44, 0x82001010u);
  return bytes;
}

class MockPlatform final : public bandwagon::aurora_fixer::LiveDllPlatform {
 public:
  MockPlatform() : descriptor(Descriptor()), memory(0x30, 0) {
    Store32(&memory, 0, resolved);
    const auto branch = bandwagon::aurora_fixer::EncodeAbsoluteBranch(
        resolved, false);
    std::copy(branch.begin(), branch.end(), memory.begin() + 0x10);
  }

  bool GetImportDescriptor(std::uint32_t target,
                           std::vector<std::uint8_t>* output) override {
    if (!descriptor_available || target != 0x82000000u) return false;
    *output = descriptor;
    return true;
  }
  bool ResolveExport(std::string_view module, std::uint32_t ordinal,
                     std::uint32_t* address) override {
    if (!export_available || ordinal != 1)
      return false;
    (void)module;
    *address = resolved;
    return true;
  }
  void Log(std::string_view message) override { logs.emplace_back(message); }
  std::uint32_t GetCurrentModule() override { return 0x82000000u; }
  void ReadMemory(std::uint32_t address, std::uint8_t* output,
                  std::size_t size) override {
    const std::size_t offset = address - 0x82001000u;
    std::copy_n(memory.data() + offset, size, output);
  }
  void WriteMemory(std::uint32_t address, const std::uint8_t* data,
                   std::size_t size) override {
    const std::size_t offset = address - 0x82001000u;
    std::copy_n(data, size, memory.data() + offset);
    writes.push_back({address, static_cast<std::uint32_t>(size)});
  }
  std::uint32_t MmGetPhysicalAddress(std::uint32_t address) override {
    physical.push_back(address);
    return address & 0x1fffffffu;
  }
  void KeFlushCacheRange(std::uint32_t address,
                         std::uint32_t size) override {
    kernel_flushes.push_back({address, size});
  }
  void HvxFlushDcacheRange(std::uint32_t address,
                           std::uint32_t size) override {
    hypervisor_flushes.push_back({address, size});
  }

  struct Range { std::uint32_t address; std::uint32_t size; };
  bool descriptor_available = true;
  bool export_available = true;
  const std::uint32_t resolved = 0x80001234u;
  std::vector<std::uint8_t> descriptor;
  std::vector<std::uint8_t> memory;
  std::vector<std::string> logs;
  std::vector<Range> writes;
  std::vector<std::uint32_t> physical;
  std::vector<Range> kernel_flushes;
  std::vector<Range> hypervisor_flushes;
};

}  // namespace

int main() {
  using namespace bandwagon::aurora_fixer;
  MockPlatform platform;
  const auto result = PatchModuleImportByOrdinal(
      0x82000000u, "xboxkrnl.exe", 1, 0x880102f8u, &platform);
  assert(result.status == LiveImportPatchStatus::kSuccess);
  assert(result.direct_pointers == 1 && result.branch_stubs == 1);
  assert(platform.writes.size() == 2);
  assert(platform.writes[0].size == 4 && platform.writes[1].size == 16);
  assert(platform.physical.size() == 2);
  assert(platform.kernel_flushes[0].size == 4);
  assert(platform.kernel_flushes[1].size == 16);
  assert(platform.hypervisor_flushes[0].size == 4);
  assert(platform.hypervisor_flushes[1].size == 16);
  const auto replacement = EncodeAbsoluteBranch(0x880102f8u, false);
  assert(std::equal(replacement.begin(), replacement.end(),
                    platform.memory.begin() + 0x10));

  MockPlatform missing_descriptor;
  missing_descriptor.descriptor_available = false;
  assert(PatchModuleImportByOrdinal(0x82000000u, "xboxkrnl.exe", 1, 0,
                                    &missing_descriptor).status ==
         LiveImportPatchStatus::kDescriptorUnavailable);
  MockPlatform missing_export;
  missing_export.export_available = false;
  assert(PatchModuleImportByOrdinal(0x82000000u, "xboxkrnl.exe", 1, 0,
                                    &missing_export).status ==
         LiveImportPatchStatus::kExportUnavailable);
  MockPlatform missing_library;
  assert(PatchModuleImportByOrdinal(0x82000000u, "xbdm.xex", 1, 0,
                                    &missing_library).status ==
         LiveImportPatchStatus::kLibraryNotImported);

  MockPlatform entry;
  assert(AuroraFixerDllMainLive(kDllProcessAttach, 0x880102f8u, &entry));
  assert(entry.logs.front() == "Hello from DLL_PROCESS_ATTACH\n");
  assert(entry.writes.size() == 2);
  ReturnImmediately();

  std::cout << "AuroraFixer live module-import tests passed\n";
}
