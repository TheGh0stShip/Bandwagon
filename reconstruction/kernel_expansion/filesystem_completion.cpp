#include "filesystem_completion.hpp"

namespace bandwagon::kernel_expansion {

std::int32_t CompleteFileIoAndReleaseLocks(
    CompletionDeviceExtension* extension, CompletionRequest* request,
    CompletionLockServices* services) {
  CompletionStackLocation* const stack = request->stack;
  if (request->pending_returned) {
    stack->control |= kStackPendingReturned;
  }

  if ((stack->major_function == kMajorRead ||
       stack->major_function == kMajorWrite) &&
      request->status == 0 &&
      (stack->file_object->flags & kFileSynchronousIo) != 0) {
    stack->file_object->current_byte_offset =
        stack->byte_offset + request->information;
  }

  services->ReleaseReadWriteLock(extension->first_lock);
  services->ReleaseReadWriteLock(extension->second_lock);
  return 0;
}

}  // namespace bandwagon::kernel_expansion
