#include "update_check_launch.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

class FakeServices final
    : public bandwagon::kernel_expansion::UpdateCheckServices {
 public:
  int load_result = 0;
  int resolve_result = 0;
  bool check_result = true;
  std::uint32_t returned_version = 0x010001dd;
  bandwagon::kernel_expansion::ModuleHandle module = 0x1234;
  bandwagon::kernel_expansion::Procedure procedure = 0x5678;
  std::vector<std::string> events;
  std::uint32_t lock_sequence = 0;

  void DetachNic(std::uintptr_t attachment) override {
    assert(attachment == 0xabcd);
    events.emplace_back("detach");
  }
  bandwagon::kernel_expansion::SpinLockCookie AcquireStateLock() override {
    events.emplace_back("lock");
    return ++lock_sequence;
  }
  void ReleaseStateLock(
      bandwagon::kernel_expansion::SpinLockCookie cookie) override {
    assert(cookie == lock_sequence);
    events.emplace_back("unlock");
  }
  void DereferenceObject(std::uintptr_t object) override {
    assert(object == 0xbeef);
    events.emplace_back("dereference");
  }
  int LoadImage(std::string_view path, std::uint32_t flags,
                std::uint32_t minimum_version,
                bandwagon::kernel_expansion::ModuleHandle* output) override {
    assert(path == bandwagon::kernel_expansion::kUpdateCheckPath);
    assert(flags == 8 && minimum_version == 0 && output);
    events.emplace_back("load");
    *output = module;
    return load_result;
  }
  int ResolveProcedure(
      bandwagon::kernel_expansion::ModuleHandle input,
      std::uint32_t ordinal,
      bandwagon::kernel_expansion::Procedure* output) override {
    assert(input == module && ordinal == 1 && output);
    events.emplace_back("resolve");
    *output = procedure;
    return resolve_result;
  }
  bool InvokeVersionCheck(
      bandwagon::kernel_expansion::Procedure input,
      std::uint32_t* version) override {
    assert(input == procedure && version);
    events.emplace_back("check");
    *version = returned_version;
    return check_result;
  }
  void PrepareModuleForUnload(
      bandwagon::kernel_expansion::ModuleHandle input) override {
    assert(input == module);
    events.emplace_back("prepare-unload");
  }
  void UnloadImage(
      bandwagon::kernel_expansion::ModuleHandle input) override {
    assert(input == module);
    events.emplace_back("unload");
  }
};

bandwagon::kernel_expansion::UpdateCheckState InitialState() {
  return {0xabcd, 0xbeef, 9, false, 0x01000100, 1};
}

}  // namespace

int main() {
  using namespace bandwagon::kernel_expansion;

  auto state = InitialState();
  FakeServices success;
  assert(LaunchUpdateCheck(&state, &success) == 0);
  assert(state.nic_attachment == 0);
  assert(state.referenced_object == 0);
  assert(state.referenced_object_auxiliary == 0);
  assert(state.update_available);
  assert(state.latest_version == 0x010001dd);
  assert(state.launch_phase == 3);
  assert((success.events == std::vector<std::string>{
      "detach", "lock", "dereference", "unlock", "load", "resolve",
      "check", "lock", "unlock", "prepare-unload", "unload"}));

  state = InitialState();
  FakeServices current;
  current.returned_version = state.latest_version;
  assert(LaunchUpdateCheck(&state, &current) == 0);
  assert(!state.update_available && state.launch_phase == 1);
  assert(state.latest_version == 0x01000100);

  state = InitialState();
  FakeServices rejected;
  rejected.check_result = false;
  assert(LaunchUpdateCheck(&state, &rejected) == 0);
  assert((rejected.events == std::vector<std::string>{
      "detach", "lock", "dereference", "unlock", "load", "resolve",
      "check", "prepare-unload", "unload"}));

  state = InitialState();
  FakeServices load_failure;
  load_failure.load_result = -1;
  assert(LaunchUpdateCheck(&state, &load_failure) == 0);
  assert((load_failure.events == std::vector<std::string>{
      "detach", "lock", "dereference", "unlock", "load",
      "prepare-unload", "unload"}));

  state = InitialState();
  FakeServices empty_failure;
  empty_failure.load_result = -1;
  empty_failure.module = 0;
  assert(LaunchUpdateCheck(&state, &empty_failure) == 0);
  assert((empty_failure.events == std::vector<std::string>{
      "detach", "lock", "dereference", "unlock", "load"}));

  assert(LaunchUpdateCheck(nullptr, &success) == 0);
  assert(LaunchUpdateCheck(&state, nullptr) == 0);

  std::cout << "Kernel Expansion UpdateCheck launch tests passed\n";
}
