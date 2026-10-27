#include "filesystem_close.hpp"

namespace bandwagon::kernel_expansion {

std::int32_t CloseFileSystemFile(FileSystemDeviceExtension* extension,
                                 FileSystemFileObject* file_object,
                                 FileSystemCloseRequest* request,
                                 FileSystemCloseServices* services) {
  services->EnterCriticalRegion();
  services->EnterDeviceLock(extension);

  if (file_object->file_context != 0) {
    services->ReleaseFileContext(file_object->file_context);
  }

  OpenFileListEntry* const forward = file_object->open_file_link.forward;
  OpenFileListEntry* const backward = file_object->open_file_link.backward;
  backward->forward = forward;
  forward->backward = backward;

  services->LeaveDeviceLock(extension);
  services->LeaveCriticalRegion();

  request->status = 0;
  services->CompleteRequest(request, 0);
  return 0;
}

}  // namespace bandwagon::kernel_expansion
