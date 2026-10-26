#include "sfcx_write_protection.hpp"

#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <string_view>

namespace {

using namespace bandwagon::kernel_expansion;

class FakeSfcx final : public SfcxWriteProtectionServices {
 public:
  SfcxStatus original_status = 0x40001234u;
  std::string_view dissected_component = "blocked.bin";
  bool mobile_name = false;
  int create_calls = 0;
  int device_control_calls = 0;
  int write_calls = 0;
  int set_information_calls = 0;
  int dissect_calls = 0;
  int mobile_calls = 0;
  SfcxDeviceObject* last_device = nullptr;
  SfcxIrp* last_irp = nullptr;
  std::string_view last_full_name;
  std::string_view last_mobile_name;

  void ResetCalls() {
    create_calls = 0;
    device_control_calls = 0;
    write_calls = 0;
    set_information_calls = 0;
    dissect_calls = 0;
    mobile_calls = 0;
    last_device = nullptr;
    last_irp = nullptr;
    last_full_name = {};
    last_mobile_name = {};
  }

  SfcxStatus SfcxFsdCreate(SfcxDeviceObject* device,
                           SfcxIrp* irp) override {
    ++create_calls;
    Remember(device, irp);
    return original_status;
  }

  SfcxStatus SfcxFsdDeviceControl(SfcxDeviceObject* device,
                                  SfcxIrp* irp) override {
    ++device_control_calls;
    Remember(device, irp);
    return original_status;
  }

  SfcxStatus SfcxFsdWrite(SfcxDeviceObject* device,
                          SfcxIrp* irp) override {
    ++write_calls;
    Remember(device, irp);
    return original_status;
  }

  SfcxStatus SfcxFsdSetInformation(SfcxDeviceObject* device,
                                   SfcxIrp* irp) override {
    ++set_information_calls;
    Remember(device, irp);
    return original_status;
  }

  std::string_view DissectFirstNameComponent(
      std::string_view name) override {
    ++dissect_calls;
    last_full_name = name;
    return dissected_component;
  }

  bool IsMobileFileName(std::string_view first_component) override {
    ++mobile_calls;
    last_mobile_name = first_component;
    return mobile_name;
  }

 private:
  void Remember(SfcxDeviceObject* device, SfcxIrp* irp) {
    last_device = device;
    last_irp = irp;
  }
};

}  // namespace

int main() {
  using namespace bandwagon::kernel_expansion;

  SfcxDriverDispatchTable dispatch;
  InstallSfcxNandWriteProtectionHooks(&dispatch);
  assert(dispatch.create == FilterSfcxCreate);
  assert(dispatch.device_control == FilterSfcxDeviceControl);
  assert(dispatch.write == FilterSfcxWrite);
  assert(dispatch.set_information == FilterSfcxSetInformation);
  InstallSfcxNandWriteProtectionHooks(nullptr);

  FakeSfcx services;
  for (const std::string_view name : kAllowedSfcxWriteNames) {
    services.mobile_name = false;
    assert(IsAllowedSfcxWritableName(&services, name));
  }
  assert(!IsAllowedSfcxWritableName(&services, "static.settings"));
  assert(!IsAllowedSfcxWritableName(&services, "extended.bin.extra"));
  services.mobile_name = true;
  assert(IsAllowedSfcxWritableName(&services, "mobile.dat"));
  assert(!IsAllowedSfcxWritableName(nullptr, "extended.bin"));

  NandWriteProtectionState protection;
  SfcxWriteProtectionContext context{&protection, &services};
  SfcxDeviceObject device;
  SfcxIoStackLocation stack;
  SfcxIrp irp{&stack};

  // A zero policy flag forwards before touching validation-only IRP fields.
  protection.enabled = 0;
  services.ResetCalls();
  assert(FilterSfcxCreate(&context, &device, nullptr) ==
         services.original_status);
  assert(FilterSfcxDeviceControl(&context, &device, nullptr) ==
         services.original_status);
  assert(FilterSfcxWrite(&context, &device, nullptr) ==
         services.original_status);
  assert(FilterSfcxSetInformation(&context, &device, nullptr) ==
         services.original_status);
  assert(services.create_calls == 1 && services.device_control_calls == 1);
  assert(services.write_calls == 1 && services.set_information_calls == 1);

  protection.enabled = 1;
  services.ResetCalls();
  stack = {};
  assert(FilterSfcxCreate(&context, &device, &irp) ==
         services.original_status);
  assert(services.create_calls == 1 && services.dissect_calls == 0);
  assert(services.last_device == &device && services.last_irp == &irp);

  stack.create_name = "folder\\file.bin";
  stack.desired_access = 0;
  services.mobile_name = false;
  services.dissected_component = "blocked.bin";
  for (const std::uint8_t disposition :
       std::array<std::uint8_t, 4>{0, 2, 4, 5}) {
    services.ResetCalls();
    stack.create_disposition = disposition;
    assert(FilterSfcxCreate(&context, &device, &irp) ==
           kSfcxStatusAccessDenied);
    assert(services.create_calls == 0 && services.dissect_calls == 1);
    assert(services.last_full_name == "folder\\file.bin");
  }

  for (const std::uint8_t disposition :
       std::array<std::uint8_t, 2>{1, 3}) {
    services.ResetCalls();
    stack.create_disposition = disposition;
    stack.desired_access = 0;
    assert(FilterSfcxCreate(&context, &device, &irp) ==
           services.original_status);
    assert(services.create_calls == 1 && services.dissect_calls == 0);
  }

  stack.create_disposition = 1;
  for (const std::uint32_t access :
       std::array<std::uint32_t, 3>{2u, 4u, 0x10000u}) {
    services.ResetCalls();
    stack.desired_access = access;
    assert(FilterSfcxCreate(&context, &device, &irp) ==
           kSfcxStatusAccessDenied);
    assert(services.create_calls == 0 && services.dissect_calls == 1);
  }
  services.ResetCalls();
  stack.desired_access = 1;
  assert(FilterSfcxCreate(&context, &device, &irp) ==
         services.original_status);
  assert(services.create_calls == 1 && services.dissect_calls == 0);

  stack.create_disposition = 0;
  stack.desired_access = 0;
  for (const std::string_view allowed : kAllowedSfcxWriteNames) {
    services.ResetCalls();
    services.dissected_component = allowed;
    services.mobile_name = false;
    assert(FilterSfcxCreate(&context, &device, &irp) ==
           services.original_status);
    assert(services.create_calls == 1 && services.mobile_calls == 1);
  }
  services.ResetCalls();
  services.dissected_component = "mobile.dat";
  services.mobile_name = true;
  assert(FilterSfcxCreate(&context, &device, &irp) ==
         services.original_status);
  assert(services.create_calls == 1 && services.mobile_calls == 1);

  // Any raw nonzero policy byte enables validation.
  protection.enabled = 0x7f;
  services.ResetCalls();
  services.mobile_name = false;
  services.dissected_component = "blocked.bin";
  assert(FilterSfcxCreate(&context, &device, &irp) ==
         kSfcxStatusAccessDenied);
  assert(services.create_calls == 0);

  stack = {};
  services.ResetCalls();
  stack.flags = kSfcxRequestBypassFlag;
  assert(FilterSfcxDeviceControl(&context, &device, &irp) ==
         services.original_status);
  assert(services.device_control_calls == 1);

  services.ResetCalls();
  stack.flags = 0;
  stack.control_code = 0x4000;
  assert(FilterSfcxDeviceControl(&context, &device, &irp) ==
         services.original_status);
  assert(services.device_control_calls == 1);

  for (std::uint32_t function = 0x11; function <= 0x13; ++function) {
    services.ResetCalls();
    stack.control_code = kSfcxIoctlWriteAccess | (function << 2);
    assert(FilterSfcxDeviceControl(&context, &device, &irp) ==
           services.original_status);
    assert(services.device_control_calls == 1);
  }
  for (const std::uint32_t function : {0x10u, 0x14u}) {
    services.ResetCalls();
    stack.control_code = kSfcxIoctlWriteAccess | (function << 2);
    assert(FilterSfcxDeviceControl(&context, &device, &irp) ==
           kSfcxStatusAccessDenied);
    assert(services.device_control_calls == 0);
  }

  SfcxFileObject file;
  stack = {};
  stack.file_object = &file;
  services.ResetCalls();
  assert(FilterSfcxWrite(&context, &device, &irp) ==
         kSfcxStatusAccessDenied);
  assert(services.write_calls == 0);
  file.file_system_context = &file;
  assert(FilterSfcxWrite(&context, &device, &irp) ==
         services.original_status);
  assert(services.write_calls == 1);

  stack = {};
  for (std::uint32_t information_class = 9; information_class <= 14;
       ++information_class) {
    services.ResetCalls();
    stack.information_class = information_class;
    const bool denied = information_class == 10 || information_class == 13;
    assert(FilterSfcxSetInformation(&context, &device, &irp) ==
           (denied ? kSfcxStatusAccessDenied : services.original_status));
    assert(services.set_information_calls == (denied ? 0 : 1));
  }

  std::cout << "Kernel Expansion SFCX write-protection tests passed\n";
}
