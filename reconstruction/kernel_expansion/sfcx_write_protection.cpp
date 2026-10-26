#include "sfcx_write_protection.hpp"

namespace bandwagon::kernel_expansion {
namespace {

bool ProtectionEnabled(const SfcxWriteProtectionContext* context) {
  return context && context->protection && context->protection->enabled != 0;
}

bool IsWriteDisposition(std::uint8_t disposition) {
  return disposition == 0 || disposition == 2 || disposition == 4 ||
         disposition == 5;
}

}  // namespace

void InstallSfcxNandWriteProtectionHooks(SfcxDriverDispatchTable* table) {
  if (!table) return;
  table->create = FilterSfcxCreate;
  table->device_control = FilterSfcxDeviceControl;
  table->write = FilterSfcxWrite;
  table->set_information = FilterSfcxSetInformation;
}

bool IsAllowedSfcxWritableName(SfcxWriteProtectionServices* services,
                               std::string_view first_component) {
  if (!services) return false;
  if (services->IsMobileFileName(first_component)) return true;
  for (const std::string_view allowed : kAllowedSfcxWriteNames) {
    if (first_component == allowed) return true;
  }
  return false;
}

SfcxStatus FilterSfcxCreate(SfcxWriteProtectionContext* context,
                            SfcxDeviceObject* device, SfcxIrp* irp) {
  if (!context || !context->services) return kSfcxStatusAccessDenied;
  if (!ProtectionEnabled(context)) {
    return context->services->SfcxFsdCreate(device, irp);
  }
  if (!irp || !irp->current_stack_location) return kSfcxStatusAccessDenied;

  const SfcxIoStackLocation& stack = *irp->current_stack_location;
  if (stack.create_name.empty()) {
    return context->services->SfcxFsdCreate(device, irp);
  }

  if (!IsWriteDisposition(stack.create_disposition) &&
      (stack.desired_access & kSfcxWriteAccessMask) == 0) {
    return context->services->SfcxFsdCreate(device, irp);
  }

  const std::string_view first_component =
      context->services->DissectFirstNameComponent(stack.create_name);
  if (IsAllowedSfcxWritableName(context->services, first_component)) {
    return context->services->SfcxFsdCreate(device, irp);
  }
  return kSfcxStatusAccessDenied;
}

SfcxStatus FilterSfcxDeviceControl(SfcxWriteProtectionContext* context,
                                   SfcxDeviceObject* device, SfcxIrp* irp) {
  if (!context || !context->services) return kSfcxStatusAccessDenied;
  if (!ProtectionEnabled(context)) {
    return context->services->SfcxFsdDeviceControl(device, irp);
  }
  if (!irp || !irp->current_stack_location) return kSfcxStatusAccessDenied;

  const SfcxIoStackLocation& stack = *irp->current_stack_location;
  if ((stack.flags & kSfcxRequestBypassFlag) != 0 ||
      (stack.control_code & kSfcxIoctlWriteAccess) == 0) {
    return context->services->SfcxFsdDeviceControl(device, irp);
  }

  const std::uint32_t function = (stack.control_code >> 2) & 0xfffu;
  if (function >= 0x11 && function <= 0x13) {
    return context->services->SfcxFsdDeviceControl(device, irp);
  }
  return kSfcxStatusAccessDenied;
}

SfcxStatus FilterSfcxWrite(SfcxWriteProtectionContext* context,
                           SfcxDeviceObject* device, SfcxIrp* irp) {
  if (!context || !context->services) return kSfcxStatusAccessDenied;
  if (!ProtectionEnabled(context)) {
    return context->services->SfcxFsdWrite(device, irp);
  }
  if (!irp || !irp->current_stack_location ||
      !irp->current_stack_location->file_object) {
    return kSfcxStatusAccessDenied;
  }

  if (irp->current_stack_location->file_object->file_system_context) {
    return context->services->SfcxFsdWrite(device, irp);
  }
  return kSfcxStatusAccessDenied;
}

SfcxStatus FilterSfcxSetInformation(SfcxWriteProtectionContext* context,
                                    SfcxDeviceObject* device, SfcxIrp* irp) {
  if (!context || !context->services) return kSfcxStatusAccessDenied;
  if (!ProtectionEnabled(context)) {
    return context->services->SfcxFsdSetInformation(device, irp);
  }
  if (!irp || !irp->current_stack_location) return kSfcxStatusAccessDenied;

  const std::uint32_t information_class =
      irp->current_stack_location->information_class;
  if (information_class == 10 || information_class == 13) {
    return kSfcxStatusAccessDenied;
  }
  return context->services->SfcxFsdSetInformation(device, irp);
}

}  // namespace bandwagon::kernel_expansion
