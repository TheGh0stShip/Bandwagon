#pragma once

#include "instruction_patch.hpp"
#include "sfcx_write_protection.hpp"
#include "xex_hooks.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace bandwagon::kernel_expansion {

inline constexpr std::string_view kKernelPatchStreamPath =
    "\\Device\\Harddisk0\\Partition1\\PeerPressure\\System\\kernel_patches.bin";
inline constexpr std::uint32_t kKernelPatchFatalErrorCode = 0x00012400u;
inline constexpr std::uint32_t kKernelPatchPendingStatus = 0x00000103u;
inline constexpr std::uint32_t kKernelPatchSentinel = 0xffffffffu;

using KernelPatchFileHandle = std::uintptr_t;

struct KernelPatchOpenRequest {
  std::string_view path;
  std::uint32_t desired_access;
  std::uint32_t object_attributes;
  std::uint32_t file_attributes;
  std::uint32_t share_access;
  std::uint32_t create_disposition;
  std::uint32_t create_options;
};

struct KernelPatchAllocationRequest {
  std::uint32_t requested_size;
  std::uint32_t allocation_type;
  std::uint32_t protection;
  std::uint32_t flags;
};

class KernelPatchFileServices {
 public:
  virtual ~KernelPatchFileServices() = default;

  virtual std::uint32_t Open(const KernelPatchOpenRequest& request,
                             KernelPatchFileHandle* handle) = 0;
  virtual std::uint32_t QueryFileSize(KernelPatchFileHandle handle,
                                      std::uint32_t information_class,
                                      std::uint32_t information_buffer_size,
                                      std::uint32_t* file_size) = 0;
  virtual std::uint32_t AllocateVirtualMemory(
      const KernelPatchAllocationRequest& request, std::uint8_t** base,
      std::uint32_t* region_size) = 0;
  virtual void DisplayFatalError(std::uint32_t code) = 0;
  virtual std::uint32_t Read(KernelPatchFileHandle handle,
                             std::uint8_t* buffer, std::uint32_t size,
                             std::int32_t* io_status) = 0;
  virtual std::uint32_t WaitForRead(KernelPatchFileHandle handle) = 0;
  virtual std::uint32_t Close(KernelPatchFileHandle handle) = 0;
  virtual std::uint32_t FreeVirtualMemory(std::uint8_t** base,
                                          std::uint32_t* region_size,
                                          std::uint32_t free_type,
                                          std::uint32_t flags) = 0;
};

class KernelPatchMemoryServices {
 public:
  virtual ~KernelPatchMemoryServices() = default;
  virtual void CopyToKernel(std::uint32_t destination,
                            const std::uint8_t* source,
                            std::size_t size) = 0;
  virtual void FlushCpuCache(std::uint32_t destination,
                             std::size_t size) = 0;
  virtual std::uintptr_t GetPhysicalAddress(
      std::uint32_t destination) = 0;
  virtual void FlushHypervisorDataCache(std::uintptr_t physical_address,
                                        std::size_t size) = 0;
};

class KernelPatchHookEnvironment {
 public:
  virtual ~KernelPatchHookEnvironment() = default;
  virtual std::uint8_t* MappedKernelImage() = 0;
  virtual std::size_t MappedKernelImageSize() const = 0;
  virtual XexHookServices* XexServices() = 0;
  virtual InstructionPatchServices* InstructionServices() = 0;
  virtual InstructionPatchLocation ResolveInstructionPatchLocation(
      std::uint32_t virtual_address, std::size_t word_count) = 0;
  virtual SfcxDriverDispatchTable* SfcxDispatchTable() = 0;
};

struct FixedRelativeHook {
  std::uint32_t target;
  std::uint32_t destination;
  std::uint32_t link_mode;
};

inline constexpr std::uint32_t kFixedAbsoluteHookTarget = 0x80061000u;
inline constexpr std::uint32_t kFixedAbsoluteHookDestination = 0x801f18e8u;
inline constexpr std::array<FixedRelativeHook, 4> kFixedRelativeHooks{{
    {0x8007d6e8u, 0x801f19c8u, 0},
    {0x8007d6b8u, 0x801f1a20u, 1},
    {0x8007b080u, 0x801f1a60u, 0},
    {0x80065748u, 0x801f1c60u, 1},
}};

// Readable host helper for the inline record loop in 0x801f0748. Target
// records are big-endian address/size/data tuples terminated by 0xffffffff.
// Bounds checks are host-safety additions; the target trusts its bundled file.
bool ApplyKernelPatchStreamRecords(const std::uint8_t* stream,
                                   std::size_t stream_size,
                                   KernelPatchMemoryServices* memory);

// Performs the fixed post-load hook sequence present at 0x801f091c..0x09ac.
void InstallFixedKernelHooks(KernelPatchHookEnvironment* hooks);

// Lift of 0x801f0748. File-load failures do not prevent fixed hook
// installation, and the target returns true on every normal path. The target
// consumes no arguments; services are injected only to make the lift portable
// and testable.
bool InstallKernelPatchesAndHooks(KernelPatchFileServices* files,
                                  KernelPatchMemoryServices* memory,
                                  KernelPatchHookEnvironment* hooks);

}  // namespace bandwagon::kernel_expansion
