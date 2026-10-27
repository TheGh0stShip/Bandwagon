#include "filesystem_completion.hpp"

#include <array>
#include <cassert>
#include <iostream>

using namespace bandwagon::kernel_expansion;

namespace {

class FakeLocks final : public CompletionLockServices {
 public:
  void ReleaseReadWriteLock(std::uintptr_t lock) override {
    released[count++] = lock;
  }
  std::array<std::uintptr_t, 2> released{};
  std::size_t count = 0;
};

}  // namespace

int main() {
  CompletionDeviceExtension extension{0x1111, 0x2222};
  CompletionFileObject file{kFileSynchronousIo, 7};
  CompletionStackLocation stack{kMajorRead, 0x20, 0x100000000ull,
                                &file};
  CompletionRequest request{0, 0x345, true, &stack};
  FakeLocks locks;
  assert(CompleteFileIoAndReleaseLocks(&extension, &request, &locks) == 0);
  assert(stack.control == 0x21);
  assert(file.current_byte_offset == 0x100000345ull);
  assert(locks.count == 2);
  assert(locks.released[0] == 0x1111);
  assert(locks.released[1] == 0x2222);

  for (const std::uint8_t major : {kMajorWrite, std::uint8_t{9}}) {
    CompletionFileObject branch_file{kFileSynchronousIo, 0xaaaa};
    CompletionStackLocation branch_stack{major, 0, 0x1000, &branch_file};
    CompletionRequest branch_request{major == kMajorWrite ? -1 : 0, 0x20,
                                     false, &branch_stack};
    FakeLocks branch_locks;
    CompleteFileIoAndReleaseLocks(&extension, &branch_request, &branch_locks);
    assert(branch_file.current_byte_offset == 0xaaaa);
    assert(branch_stack.control == 0);
  }

  CompletionFileObject async_file{0, 0xbbbb};
  CompletionStackLocation async_stack{kMajorWrite, 0, 0x2000, &async_file};
  CompletionRequest async_request{0, 0x30, false, &async_stack};
  FakeLocks async_locks;
  CompleteFileIoAndReleaseLocks(&extension, &async_request, &async_locks);
  assert(async_file.current_byte_offset == 0xbbbb);

  std::cout << "Kernel Expansion filesystem completion tests passed\n";
}
