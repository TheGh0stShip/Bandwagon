#include "filesystem_close.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

using namespace bandwagon::kernel_expansion;

namespace {

class FakeServices final : public FileSystemCloseServices {
 public:
  void EnterCriticalRegion() override { events.emplace_back("enter-region"); }
  void EnterDeviceLock(FileSystemDeviceExtension* extension) override {
    assert(extension == expected_extension);
    events.emplace_back("enter-lock");
  }
  void ReleaseFileContext(std::uintptr_t context) override {
    assert(context == expected_context);
    events.emplace_back("release-context");
  }
  void LeaveDeviceLock(FileSystemDeviceExtension* extension) override {
    assert(extension == expected_extension);
    events.emplace_back("leave-lock");
  }
  void LeaveCriticalRegion() override { events.emplace_back("leave-region"); }
  void CompleteRequest(FileSystemCloseRequest* request,
                       std::uint32_t priority_boost) override {
    assert(request == expected_request);
    assert(request->status == 0);
    assert(priority_boost == 0);
    events.emplace_back("complete");
  }

  FileSystemDeviceExtension* expected_extension = nullptr;
  FileSystemCloseRequest* expected_request = nullptr;
  std::uintptr_t expected_context = 0;
  std::vector<std::string> events;
};

}  // namespace

int main() {
  FileSystemDeviceExtension extension;
  FileSystemCloseRequest request{-1};
  OpenFileListEntry head;
  FileSystemFileObject file;
  head.forward = &file.open_file_link;
  head.backward = &file.open_file_link;
  file.file_context = 0x12345678u;
  file.open_file_link.forward = &head;
  file.open_file_link.backward = &head;

  FakeServices services;
  services.expected_extension = &extension;
  services.expected_request = &request;
  services.expected_context = file.file_context;
  assert(CloseFileSystemFile(&extension, &file, &request, &services) == 0);
  assert(request.status == 0);
  assert(head.forward == &head);
  assert(head.backward == &head);
  assert((services.events == std::vector<std::string>{
                                 "enter-region", "enter-lock",
                                 "release-context", "leave-lock",
                                 "leave-region", "complete"}));

  FileSystemCloseRequest no_context_request{-1};
  OpenFileListEntry no_context_head;
  FileSystemFileObject no_context_file;
  no_context_head.forward = &no_context_file.open_file_link;
  no_context_head.backward = &no_context_file.open_file_link;
  no_context_file.open_file_link.forward = &no_context_head;
  no_context_file.open_file_link.backward = &no_context_head;
  FakeServices no_context_services;
  no_context_services.expected_extension = &extension;
  no_context_services.expected_request = &no_context_request;
  assert(CloseFileSystemFile(&extension, &no_context_file,
                             &no_context_request,
                             &no_context_services) == 0);
  assert((no_context_services.events == std::vector<std::string>{
                                            "enter-region", "enter-lock",
                                            "leave-lock", "leave-region",
                                            "complete"}));

  std::cout << "Kernel Expansion filesystem close tests passed\n";
}
