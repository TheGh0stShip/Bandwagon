#pragma once

#include "nand_write_protection.hpp"

#include <cstdint>
#include <string_view>

namespace bandwagon::kernel_expansion {

using SfcxStatus = std::uint32_t;
inline constexpr SfcxStatus kSfcxStatusAccessDenied = 0xc0000022u;
inline constexpr std::uint32_t kSfcxWriteAccessMask = 0x00010006u;
inline constexpr std::uint32_t kSfcxIoctlWriteAccess = 0x00008000u;
inline constexpr std::uint8_t kSfcxRequestBypassFlag = 0x80u;

inline constexpr std::string_view kAllowedSfcxWriteNames[] = {
    "extended.bin",
    "Static.settings",
    "Statistics.settings",
};

struct SfcxDeviceObject {};

struct SfcxFileObject {
  // FILE_OBJECT::FsContext at target offset +0x08.
  void* file_system_context = nullptr;
};

// Typed host view of the fields read from the current I/O stack location.
// Comments retain the corresponding target offsets without imposing the
// retail kernel's private binary layout on portable test builds.
struct SfcxIoStackLocation {
  std::uint8_t flags = 0;                 // +0x02
  std::uint32_t desired_access = 0;       // +0x04
  std::uint8_t create_disposition = 0;    // +0x08
  std::uint32_t information_class = 0;    // +0x08
  std::uint32_t control_code = 0;         // +0x10
  std::string_view create_name;           // descriptor pointer at +0x10
  SfcxFileObject* file_object = nullptr;  // +0x18
};

struct SfcxIrp {
  // IRP::CurrentStackLocation at target offset +0x50.
  SfcxIoStackLocation* current_stack_location = nullptr;
};

class SfcxWriteProtectionServices {
 public:
  virtual ~SfcxWriteProtectionServices() = default;

  virtual SfcxStatus SfcxFsdCreate(SfcxDeviceObject* device,
                                   SfcxIrp* irp) = 0;
  virtual SfcxStatus SfcxFsdDeviceControl(SfcxDeviceObject* device,
                                          SfcxIrp* irp) = 0;
  virtual SfcxStatus SfcxFsdWrite(SfcxDeviceObject* device,
                                  SfcxIrp* irp) = 0;
  virtual SfcxStatus SfcxFsdSetInformation(SfcxDeviceObject* device,
                                           SfcxIrp* irp) = 0;

  // Injectable forms of the retail ObDissectName and
  // SfcxIsMobileFileName helpers used by the create filter.
  virtual std::string_view DissectFirstNameComponent(
      std::string_view name) = 0;
  virtual bool IsMobileFileName(std::string_view first_component) = 0;
};

struct SfcxWriteProtectionContext {
  const NandWriteProtectionState* protection = nullptr;
  SfcxWriteProtectionServices* services = nullptr;
};

using SfcxProtectedDispatch = SfcxStatus (*)(
    SfcxWriteProtectionContext* context, SfcxDeviceObject* device,
    SfcxIrp* irp);

// Logical host view; the comments identify retail driver-object slots rather
// than requiring those private 32-bit offsets in this portable structure.
struct SfcxDriverDispatchTable {
  SfcxProtectedDispatch create = nullptr;           // driver object +0x0c
  SfcxProtectedDispatch write = nullptr;            // driver object +0x18
  SfcxProtectedDispatch set_information = nullptr;  // driver object +0x20
  SfcxProtectedDispatch device_control = nullptr;   // driver object +0x30
};

// Lift of 0x801f1cb0, which replaces four SfcxDriverObject dispatch slots.
void InstallSfcxNandWriteProtectionHooks(SfcxDriverDispatchTable* table);

// Lift of the filename policy helper at 0x801f4f28.
bool IsAllowedSfcxWritableName(SfcxWriteProtectionServices* services,
                               std::string_view first_component);

// Lifts of the four SFCX dispatch filters at 0x801f3758..0x801f38f8.
// The target assumes its fixed globals and kernel request pointers are valid.
// These portable adapters additionally deny missing injected services or
// validation-time request views; those null guards are host-safety behavior,
// not claimed target behavior for invalid kernel pointers. A missing policy
// state is treated as disabled, matching the host accessor abstraction.
SfcxStatus FilterSfcxCreate(SfcxWriteProtectionContext* context,
                            SfcxDeviceObject* device, SfcxIrp* irp);
SfcxStatus FilterSfcxDeviceControl(SfcxWriteProtectionContext* context,
                                   SfcxDeviceObject* device, SfcxIrp* irp);
SfcxStatus FilterSfcxWrite(SfcxWriteProtectionContext* context,
                           SfcxDeviceObject* device, SfcxIrp* irp);
SfcxStatus FilterSfcxSetInformation(SfcxWriteProtectionContext* context,
                                    SfcxDeviceObject* device, SfcxIrp* irp);

}  // namespace bandwagon::kernel_expansion
