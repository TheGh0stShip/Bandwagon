#include "kernel_patch_loader.hpp"

namespace bandwagon::kernel_expansion {
namespace {

std::uint32_t LoadBigEndian32(const std::uint8_t* value) {
  return (static_cast<std::uint32_t>(value[0]) << 24) |
         (static_cast<std::uint32_t>(value[1]) << 16) |
         (static_cast<std::uint32_t>(value[2]) << 8) |
         static_cast<std::uint32_t>(value[3]);
}

}  // namespace

bool ApplyKernelPatchStreamRecords(const std::uint8_t* stream,
                                   std::size_t stream_size,
                                   KernelPatchMemoryServices* memory) {
  if (!stream || !memory) return false;

  std::size_t cursor = 0;
  for (;;) {
    if (cursor > stream_size || stream_size - cursor < 4) return false;
    const std::uint32_t destination = LoadBigEndian32(stream + cursor);
    if (destination == kKernelPatchSentinel) return true;
    if (stream_size - cursor < 8) return false;

    const std::uint32_t size = LoadBigEndian32(stream + cursor + 4);
    if (size > stream_size - cursor - 8) return false;
    const std::uint8_t* const patch = stream + cursor + 8;
    memory->CopyToKernel(destination, patch, size);
    memory->FlushCpuCache(destination, size);
    const std::uintptr_t physical = memory->GetPhysicalAddress(destination);
    memory->FlushHypervisorDataCache(physical, size);
    cursor += 8u + static_cast<std::size_t>(size);
  }
}

void InstallFixedKernelHooks(KernelPatchHookEnvironment* hooks) {
  if (!hooks) return;

  (void)InstallKernelXexDataHandlers(
      hooks->MappedKernelImage(), hooks->MappedKernelImageSize(),
      hooks->XexServices());

  InstructionPatchServices* const instruction_services =
      hooks->InstructionServices();
  InstallAbsoluteBranchHook(
      hooks->ResolveInstructionPatchLocation(kFixedAbsoluteHookTarget, 4),
      kFixedAbsoluteHookDestination, 0, nullptr, instruction_services);
  for (const FixedRelativeHook& hook : kFixedRelativeHooks) {
    (void)InstallRelativeBranchHook(
        hooks->ResolveInstructionPatchLocation(hook.target, 1),
        hook.destination, hook.link_mode, nullptr, instruction_services);
  }

  InstallSfcxNandWriteProtectionHooks(hooks->SfcxDispatchTable());
}

bool InstallKernelPatchesAndHooks(KernelPatchFileServices* files,
                                  KernelPatchMemoryServices* memory,
                                  KernelPatchHookEnvironment* hooks) {
  if (!files || !memory || !hooks) return false;

  KernelPatchFileHandle handle = 0;
  std::uint8_t* allocation = nullptr;
  std::uint32_t region_size = 0;

  const KernelPatchOpenRequest open_request{
      kKernelPatchStreamPath,
      0x80100080u,
      0x40u,
      0x80u,
      1u,
      1u,
      0x60u,
  };

  std::uint32_t status = files->Open(open_request, &handle);
  if (status == 0) {
    std::uint32_t file_size = 0;
    status = files->QueryFileSize(handle, 0x22u, 0x38u, &file_size);
    if (status == 0) {
      region_size = file_size;
      const KernelPatchAllocationRequest allocation_request{
          file_size,
          0x1000u,
          4u,
          2u,
      };
      const std::uint32_t allocation_status = files->AllocateVirtualMemory(
          allocation_request, &allocation, &region_size);
      if (allocation_status != 0 || !allocation) {
        files->DisplayFatalError(kKernelPatchFatalErrorCode);
      }

      // VdDisplayFatalError is effectively non-returning on target. Avoid a
      // null read when a host test double returns from it.
      if (allocation) {
        std::int32_t io_status = 0;
        status = files->Read(handle, allocation, region_size, &io_status);
        if (status == kKernelPatchPendingStatus) {
          status = files->WaitForRead(handle);
        }
        if (status == 0 && io_status >= 0) {
          (void)files->Close(handle);
          (void)ApplyKernelPatchStreamRecords(allocation, region_size, memory);
          (void)files->FreeVirtualMemory(&allocation, &region_size, 0x8000u,
                                         2u);
          InstallFixedKernelHooks(hooks);
          return true;
        }
      }
    }

    // This intentionally does not free a read allocation: the target closes
    // the handle and leaks that mapping on its read/wait/IO-status failures.
    (void)files->Close(handle);
  }

  // An open failure jumps directly here, even if the API populated a handle.
  InstallFixedKernelHooks(hooks);
  return true;
}

}  // namespace bandwagon::kernel_expansion
