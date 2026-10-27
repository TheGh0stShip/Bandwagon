#pragma once

#include <cstdint>

namespace bandwagon::kernel_expansion {

inline constexpr std::uint8_t kMajorRead = 2;
inline constexpr std::uint8_t kMajorWrite = 3;
inline constexpr std::uint8_t kStackPendingReturned = 1;
inline constexpr std::uint8_t kFileSynchronousIo = 1;

struct CompletionFileObject {
  std::uint8_t flags = 0;                // target offset +0x02
  std::uint64_t current_byte_offset = 0; // target offset +0x14
};

struct CompletionStackLocation {
  std::uint8_t major_function = 0; // target offset +0x00
  std::uint8_t control = 0;        // target offset +0x03
  std::uint64_t byte_offset = 0;   // target offset +0x0c
  CompletionFileObject* file_object = nullptr; // target offset +0x18
};

struct CompletionRequest {
  std::int32_t status = 0;          // target offset +0x10
  std::uint32_t information = 0;    // target offset +0x14
  bool pending_returned = false;    // target offset +0x1a
  CompletionStackLocation* stack = nullptr; // target offset +0x50
};

struct CompletionDeviceExtension {
  std::uintptr_t first_lock = 0;  // target offset +0x10
  std::uintptr_t second_lock = 0; // target offset +0x48
};

class CompletionLockServices {
 public:
  virtual ~CompletionLockServices() = default;
  virtual void ReleaseReadWriteLock(std::uintptr_t lock) = 0;
};

// Completion callback at 0x801f9810.
std::int32_t CompleteFileIoAndReleaseLocks(
    CompletionDeviceExtension* extension, CompletionRequest* request,
    CompletionLockServices* services);

}  // namespace bandwagon::kernel_expansion
