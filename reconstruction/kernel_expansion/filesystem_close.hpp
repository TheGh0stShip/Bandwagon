#pragma once

#include <cstdint>

namespace bandwagon::kernel_expansion {

struct OpenFileListEntry {
  OpenFileListEntry* forward = nullptr;
  OpenFileListEntry* backward = nullptr;
};

struct FileSystemFileObject {
  std::uintptr_t file_context = 0;  // target FILE_OBJECT offset +0x08
  OpenFileListEntry open_file_link; // target FILE_OBJECT offset +0x50
};

struct FileSystemDeviceExtension {};

struct FileSystemCloseRequest {
  std::int32_t status = 0;  // target IRP offset +0x10
};

class FileSystemCloseServices {
 public:
  virtual ~FileSystemCloseServices() = default;
  virtual void EnterCriticalRegion() = 0;
  virtual void EnterDeviceLock(FileSystemDeviceExtension* extension) = 0;
  virtual void ReleaseFileContext(std::uintptr_t context) = 0;
  virtual void LeaveDeviceLock(FileSystemDeviceExtension* extension) = 0;
  virtual void LeaveCriticalRegion() = 0;
  virtual void CompleteRequest(FileSystemCloseRequest* request,
                               std::uint32_t priority_boost) = 0;
};

// IRP close handler at 0x801f5b88. The target obtains file_object through
// IRP::CurrentStackLocation (+0x50), whose FileObject field is at +0x18.
std::int32_t CloseFileSystemFile(FileSystemDeviceExtension* extension,
                                 FileSystemFileObject* file_object,
                                 FileSystemCloseRequest* request,
                                 FileSystemCloseServices* services);

}  // namespace bandwagon::kernel_expansion
