#include "kernel_patch_loader.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace {

using namespace bandwagon::kernel_expansion;

void AppendBigEndian32(std::vector<std::uint8_t>* output,
                       std::uint32_t value) {
  output->push_back(static_cast<std::uint8_t>(value >> 24));
  output->push_back(static_cast<std::uint8_t>(value >> 16));
  output->push_back(static_cast<std::uint8_t>(value >> 8));
  output->push_back(static_cast<std::uint8_t>(value));
}

std::vector<std::uint8_t> TestPatchStream() {
  std::vector<std::uint8_t> stream;
  AppendBigEndian32(&stream, 0x80001000u);
  AppendBigEndian32(&stream, 4);
  stream.insert(stream.end(), {0x38, 0x60, 0x00, 0x01});
  AppendBigEndian32(&stream, 0x80002000u);
  AppendBigEndian32(&stream, 4);
  stream.insert(stream.end(), {0xaa, 0xbb, 0xcc, 0xdd});
  AppendBigEndian32(&stream, kKernelPatchSentinel);
  return stream;
}

struct AppliedPatch {
  std::uint32_t destination;
  std::vector<std::uint8_t> bytes;
};

class FakePatchMemory final : public KernelPatchMemoryServices {
 public:
  std::vector<AppliedPatch> patches;
  std::vector<std::string> events;
  std::vector<std::string>* workflow = nullptr;

  void Record(const char* event) {
    events.emplace_back(event);
    if (workflow) workflow->emplace_back(event);
  }

  void CopyToKernel(std::uint32_t destination, const std::uint8_t* source,
                    std::size_t size) override {
    patches.push_back(
        {destination, std::vector<std::uint8_t>(source, source + size)});
    Record("copy");
  }

  void FlushCpuCache(std::uint32_t, std::size_t) override {
    Record("cpu");
  }

  std::uintptr_t GetPhysicalAddress(std::uint32_t destination) override {
    Record("physical");
    return 0x100000000ull + destination;
  }

  void FlushHypervisorDataCache(std::uintptr_t, std::size_t) override {
    Record("hypervisor");
  }
};

class FakeInstructionServices final : public InstructionPatchServices {
 public:
  std::vector<std::string> events;
  int debug_breaks = 0;

  void FlushCpuCache(const void*, std::size_t size) override {
    events.emplace_back(size == 16 ? "stub-cpu" : "branch-cpu");
  }

  std::uintptr_t GetPhysicalAddress(const void* address) override {
    events.emplace_back("instruction-physical");
    return reinterpret_cast<std::uintptr_t>(address) + 0x1000;
  }

  void FlushHypervisorDataCache(std::uintptr_t, std::size_t size) override {
    events.emplace_back(size == 16 ? "stub-hypervisor"
                                   : "branch-hypervisor");
  }

  void DebugBreak() override { ++debug_breaks; }
};

class FakeXexServices final : public XexHookServices {
 public:
  int find_calls = 0;
  std::vector<std::string>* hook_order = nullptr;

  bool FindNtHeader(const std::uint8_t*, std::size_t,
                    std::size_t*) override {
    ++find_calls;
    if (hook_order) hook_order->emplace_back("xex");
    return false;
  }
  void FlushCpuCache(std::size_t, std::size_t) override { assert(false); }
  std::uint64_t PhysicalAddress(std::size_t) override {
    assert(false);
    return 0;
  }
  void FlushHypervisorDataCache(std::uint64_t,
                                std::size_t) override {
    assert(false);
  }
};

class FakeHookEnvironment final : public KernelPatchHookEnvironment {
 public:
  std::array<std::uint8_t, 1> kernel{{0}};
  FakeXexServices xex;
  FakeInstructionServices instructions;
  SfcxDriverDispatchTable sfcx;
  std::array<std::array<std::uint32_t, 4>, 5> storage{};
  std::vector<std::pair<std::uint32_t, std::size_t>> resolved;
  std::vector<std::string> hook_order;

  FakeHookEnvironment() {
    for (auto& words : storage) words.fill(0x60000000u);
    xex.hook_order = &hook_order;
  }

  std::uint8_t* MappedKernelImage() override { return kernel.data(); }
  std::size_t MappedKernelImageSize() const override {
    return kernel.size();
  }
  XexHookServices* XexServices() override { return &xex; }
  InstructionPatchServices* InstructionServices() override {
    return &instructions;
  }

  InstructionPatchLocation ResolveInstructionPatchLocation(
      std::uint32_t virtual_address, std::size_t word_count) override {
    const std::size_t index = resolved.size();
    assert(index < storage.size());
    resolved.emplace_back(virtual_address, word_count);
    hook_order.emplace_back(index == 0 ? "absolute" : "relative");
    return {virtual_address, storage[index].data()};
  }

  SfcxDriverDispatchTable* SfcxDispatchTable() override {
    hook_order.emplace_back("sfcx");
    return &sfcx;
  }
};

class FakePatchFiles final : public KernelPatchFileServices {
 public:
  std::vector<std::uint8_t> file = TestPatchStream();
  std::vector<std::uint8_t> allocation;
  std::vector<std::string> events;
  std::vector<std::string>* workflow = nullptr;
  std::uint32_t open_status = 0;
  std::uint32_t query_status = 0;
  std::uint32_t allocation_status = 0;
  std::uint32_t read_status = 0;
  std::uint32_t wait_status = 0;
  std::int32_t io_status = 0;
  std::uint32_t rounded_region_size = 0;
  KernelPatchFileHandle opened_handle = 0x55;
  bool populate_failed_open_handle = false;
  bool provide_allocation = true;
  bool allocation_live = false;
  int close_calls = 0;
  int free_calls = 0;
  int fatal_calls = 0;
  int wait_calls = 0;
  std::uint32_t read_size = 0;

  void Record(const char* event) {
    events.emplace_back(event);
    if (workflow) workflow->emplace_back(event);
  }

  std::uint32_t Open(const KernelPatchOpenRequest& request,
                     KernelPatchFileHandle* handle) override {
    Record("open");
    assert(request.path == kKernelPatchStreamPath);
    assert(request.desired_access == 0x80100080u);
    assert(request.object_attributes == 0x40u);
    assert(request.file_attributes == 0x80u);
    assert(request.share_access == 1u);
    assert(request.create_disposition == 1u);
    assert(request.create_options == 0x60u);
    if (open_status == 0 || populate_failed_open_handle) {
      *handle = opened_handle;
    }
    return open_status;
  }

  std::uint32_t QueryFileSize(KernelPatchFileHandle handle,
                              std::uint32_t information_class,
                              std::uint32_t information_buffer_size,
                              std::uint32_t* file_size) override {
    Record("query");
    assert(handle == opened_handle);
    assert(information_class == 0x22u);
    assert(information_buffer_size == 0x38u);
    if (query_status == 0) {
      *file_size = static_cast<std::uint32_t>(file.size());
    }
    return query_status;
  }

  std::uint32_t AllocateVirtualMemory(
      const KernelPatchAllocationRequest& request, std::uint8_t** base,
      std::uint32_t* region_size) override {
    Record("allocate");
    assert(request.requested_size == file.size());
    assert(request.allocation_type == 0x1000u);
    assert(request.protection == 4u && request.flags == 2u);
    assert(*region_size == request.requested_size);
    if (rounded_region_size != 0) *region_size = rounded_region_size;
    if (provide_allocation) {
      allocation.assign(*region_size, 0);
      *base = allocation.data();
      allocation_live = true;
    }
    return allocation_status;
  }

  void DisplayFatalError(std::uint32_t code) override {
    Record("fatal");
    assert(code == kKernelPatchFatalErrorCode);
    ++fatal_calls;
  }

  std::uint32_t Read(KernelPatchFileHandle handle, std::uint8_t* buffer,
                     std::uint32_t size,
                     std::int32_t* output_io_status) override {
    Record("read");
    assert(handle == opened_handle);
    assert(buffer == allocation.data());
    assert(size == allocation.size());
    read_size = size;
    std::copy(file.begin(), file.end(), buffer);
    *output_io_status = io_status;
    return read_status;
  }

  std::uint32_t WaitForRead(KernelPatchFileHandle handle) override {
    Record("wait");
    assert(handle == opened_handle);
    ++wait_calls;
    return wait_status;
  }

  std::uint32_t Close(KernelPatchFileHandle handle) override {
    Record("close");
    assert(handle == opened_handle);
    ++close_calls;
    return 0xc0000001u;
  }

  std::uint32_t FreeVirtualMemory(std::uint8_t** base,
                                  std::uint32_t* region_size,
                                  std::uint32_t free_type,
                                  std::uint32_t flags) override {
    Record("free");
    assert(*base == allocation.data());
    assert(*region_size == allocation.size());
    assert(free_type == 0x8000u && flags == 2u);
    allocation_live = false;
    ++free_calls;
    *base = nullptr;
    *region_size = 0;
    return 0xc0000001u;
  }
};

void AssertHooksInstalled(const FakeHookEnvironment& hooks) {
  assert(hooks.xex.find_calls == 1);
  assert((hooks.hook_order == std::vector<std::string>{
                                  "xex", "absolute", "relative", "relative",
                                  "relative", "relative", "sfcx"}));
  assert((hooks.resolved ==
          std::vector<std::pair<std::uint32_t, std::size_t>>{
              {kFixedAbsoluteHookTarget, 4},
              {kFixedRelativeHooks[0].target, 1},
              {kFixedRelativeHooks[1].target, 1},
              {kFixedRelativeHooks[2].target, 1},
              {kFixedRelativeHooks[3].target, 1},
          }));
  assert((hooks.storage[0] == std::array<std::uint32_t, 4>{
                                  0x3d60801fu, 0x396b18e8u,
                                  0x7d6903a6u, 0x4e800420u}));
  assert(hooks.storage[1][0] == 0x481742e0u);
  assert(hooks.storage[2][0] == 0x48174369u);
  assert(hooks.storage[3][0] == 0x481769e0u);
  assert(hooks.storage[4][0] == 0x4818c519u);
  assert(hooks.instructions.debug_breaks == 0);
  assert(hooks.sfcx.create == FilterSfcxCreate);
  assert(hooks.sfcx.device_control == FilterSfcxDeviceControl);
  assert(hooks.sfcx.write == FilterSfcxWrite);
  assert(hooks.sfcx.set_information == FilterSfcxSetInformation);
}

}  // namespace

int main() {
  using namespace bandwagon::kernel_expansion;

  const std::vector<std::uint8_t> stream = TestPatchStream();
  FakePatchMemory direct_memory;
  assert(ApplyKernelPatchStreamRecords(stream.data(), stream.size(),
                                       &direct_memory));
  assert(direct_memory.patches.size() == 2);
  assert(direct_memory.patches[0].destination == 0x80001000u);
  assert((direct_memory.patches[0].bytes ==
          std::vector<std::uint8_t>{0x38, 0x60, 0x00, 0x01}));
  assert(direct_memory.patches[1].destination == 0x80002000u);
  assert((direct_memory.patches[1].bytes ==
          std::vector<std::uint8_t>{0xaa, 0xbb, 0xcc, 0xdd}));
  assert((direct_memory.events == std::vector<std::string>{
      "copy", "cpu", "physical", "hypervisor", "copy", "cpu",
      "physical", "hypervisor"}));

  assert(!ApplyKernelPatchStreamRecords(nullptr, 0, &direct_memory));
  assert(!ApplyKernelPatchStreamRecords(stream.data(), 3, &direct_memory));
  std::vector<std::uint8_t> oversized;
  AppendBigEndian32(&oversized, 0x80001000u);
  AppendBigEndian32(&oversized, 5);
  oversized.push_back(0xaa);
  assert(!ApplyKernelPatchStreamRecords(oversized.data(), oversized.size(),
                                        &direct_memory));
  std::vector<std::uint8_t> no_sentinel(stream.begin(), stream.end() - 4);
  FakePatchMemory missing_sentinel;
  assert(!ApplyKernelPatchStreamRecords(
      no_sentinel.data(), no_sentinel.size(), &missing_sentinel));
  assert(missing_sentinel.patches.size() == 2);
  std::vector<std::uint8_t> sentinel_only;
  AppendBigEndian32(&sentinel_only, kKernelPatchSentinel);
  FakePatchMemory sentinel_memory;
  assert(ApplyKernelPatchStreamRecords(sentinel_only.data(),
                                       sentinel_only.size(),
                                       &sentinel_memory));
  assert(sentinel_memory.patches.empty());

  FakeHookEnvironment direct_hooks;
  InstallFixedKernelHooks(&direct_hooks);
  AssertHooksInstalled(direct_hooks);
  InstallFixedKernelHooks(nullptr);

  FakePatchFiles success_files;
  success_files.rounded_region_size = 0x1000;
  FakePatchMemory success_memory;
  std::vector<std::string> success_workflow;
  success_files.workflow = &success_workflow;
  success_memory.workflow = &success_workflow;
  FakeHookEnvironment success_hooks;
  assert(InstallKernelPatchesAndHooks(&success_files, &success_memory,
                                      &success_hooks));
  assert(success_files.read_size == 0x1000);
  assert(success_files.close_calls == 1 && success_files.free_calls == 1);
  assert(!success_files.allocation_live && success_memory.patches.size() == 2);
  assert((success_files.events == std::vector<std::string>{
      "open", "query", "allocate", "read", "close", "free"}));
  assert((success_workflow == std::vector<std::string>{
      "open", "query", "allocate", "read", "close",
      "copy", "cpu", "physical", "hypervisor",
      "copy", "cpu", "physical", "hypervisor", "free"}));
  AssertHooksInstalled(success_hooks);

  FakePatchFiles pending_files;
  pending_files.read_status = kKernelPatchPendingStatus;
  FakePatchMemory pending_memory;
  FakeHookEnvironment pending_hooks;
  assert(InstallKernelPatchesAndHooks(&pending_files, &pending_memory,
                                      &pending_hooks));
  assert(pending_files.wait_calls == 1 && pending_files.free_calls == 1);
  assert(pending_memory.patches.size() == 2);
  AssertHooksInstalled(pending_hooks);

  FakePatchFiles open_failure;
  open_failure.open_status = 0xc0000001u;
  open_failure.populate_failed_open_handle = true;
  FakePatchMemory open_failure_memory;
  FakeHookEnvironment open_failure_hooks;
  assert(InstallKernelPatchesAndHooks(&open_failure, &open_failure_memory,
                                      &open_failure_hooks));
  assert(open_failure.close_calls == 0);
  assert(open_failure_memory.patches.empty());
  AssertHooksInstalled(open_failure_hooks);

  FakePatchFiles query_failure;
  query_failure.query_status = 0x40000001u;
  FakePatchMemory query_failure_memory;
  FakeHookEnvironment query_failure_hooks;
  assert(InstallKernelPatchesAndHooks(&query_failure, &query_failure_memory,
                                      &query_failure_hooks));
  assert(query_failure.close_calls == 1);
  assert(query_failure.events ==
         std::vector<std::string>({"open", "query", "close"}));
  AssertHooksInstalled(query_failure_hooks);

  FakePatchFiles allocation_failure;
  allocation_failure.allocation_status = 0xc0000001u;
  allocation_failure.provide_allocation = false;
  FakePatchMemory allocation_failure_memory;
  FakeHookEnvironment allocation_failure_hooks;
  assert(InstallKernelPatchesAndHooks(&allocation_failure,
                                      &allocation_failure_memory,
                                      &allocation_failure_hooks));
  assert(allocation_failure.fatal_calls == 1);
  assert(allocation_failure.close_calls == 1 &&
         allocation_failure.free_calls == 0);
  AssertHooksInstalled(allocation_failure_hooks);

  FakePatchFiles null_allocation;
  null_allocation.provide_allocation = false;
  FakePatchMemory null_allocation_memory;
  FakeHookEnvironment null_allocation_hooks;
  assert(InstallKernelPatchesAndHooks(&null_allocation,
                                      &null_allocation_memory,
                                      &null_allocation_hooks));
  assert(null_allocation.fatal_calls == 1);
  assert(null_allocation.close_calls == 1 && null_allocation.free_calls == 0);
  AssertHooksInstalled(null_allocation_hooks);

  FakePatchFiles warning_allocation;
  warning_allocation.allocation_status = 0xc0000001u;
  FakePatchMemory warning_memory;
  FakeHookEnvironment warning_hooks;
  assert(InstallKernelPatchesAndHooks(&warning_allocation, &warning_memory,
                                      &warning_hooks));
  assert(warning_allocation.fatal_calls == 1);
  assert(warning_allocation.free_calls == 1);
  assert(warning_memory.patches.size() == 2);
  AssertHooksInstalled(warning_hooks);

  FakePatchFiles positive_completion;
  positive_completion.io_status = 1;
  FakePatchMemory positive_completion_memory;
  FakeHookEnvironment positive_completion_hooks;
  assert(InstallKernelPatchesAndHooks(&positive_completion,
                                      &positive_completion_memory,
                                      &positive_completion_hooks));
  assert(positive_completion.free_calls == 1);
  assert(positive_completion_memory.patches.size() == 2);
  AssertHooksInstalled(positive_completion_hooks);

  for (int failure_kind = 0; failure_kind < 4; ++failure_kind) {
    FakePatchFiles read_failure;
    if (failure_kind == 0) read_failure.read_status = 0xc0000001u;
    if (failure_kind == 1) read_failure.read_status = 0x40000001u;
    if (failure_kind == 2) {
      read_failure.read_status = kKernelPatchPendingStatus;
      read_failure.wait_status = 0xc0000001u;
    }
    if (failure_kind == 3) read_failure.io_status = -1;
    FakePatchMemory failure_memory;
    FakeHookEnvironment failure_hooks;
    assert(InstallKernelPatchesAndHooks(&read_failure, &failure_memory,
                                        &failure_hooks));
    assert(read_failure.close_calls == 1 && read_failure.free_calls == 0);
    assert(read_failure.allocation_live);
    assert(failure_memory.patches.empty());
    AssertHooksInstalled(failure_hooks);
  }

  assert(!InstallKernelPatchesAndHooks(nullptr, &success_memory,
                                       &success_hooks));
  assert(!InstallKernelPatchesAndHooks(&success_files, nullptr,
                                       &success_hooks));
  assert(!InstallKernelPatchesAndHooks(&success_files, &success_memory,
                                       nullptr));

  std::cout << "Kernel Expansion kernel-patch loader tests passed\n";
}
