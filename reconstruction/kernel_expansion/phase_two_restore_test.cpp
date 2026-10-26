#include "phase_two_restore.hpp"

#include <cassert>
#include <cstring>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

class FakeFiles final : public bandwagon::kernel_expansion::FileServices {
 public:
  std::string content = "first.bin\nsecond.bin\n";
  int open_status = 0;
  bool freed = false;

  int OpenReadOnly(std::string_view path, bool object_manager_path,
                   bandwagon::kernel_expansion::FileHandle* handle) override {
    assert(path == bandwagon::kernel_expansion::kPhaseTwoMarkerPath);
    assert(!object_manager_path && handle);
    *handle = 0x44;
    return open_status;
  }
  int QueryFileSize(bandwagon::kernel_expansion::FileHandle handle,
                    std::uint32_t* size) override {
    assert(handle == 0x44 && size);
    *size = static_cast<std::uint32_t>(content.size());
    return 0;
  }
  std::uint8_t* Allocate(std::uint32_t size) override {
    return new std::uint8_t[size];
  }
  int Read(bandwagon::kernel_expansion::FileHandle handle,
           std::uint8_t* buffer, std::uint32_t size,
           std::int32_t* io_status) override {
    assert(handle == 0x44 && buffer && size == content.size() && io_status);
    std::memcpy(buffer, content.data(), size);
    *io_status = 0;
    return 0;
  }
  int WaitForRead(bandwagon::kernel_expansion::FileHandle) override {
    assert(false);
    return -1;
  }
  void Free(std::uint8_t* buffer) override {
    delete[] buffer;
    freed = true;
  }
  void Close(bandwagon::kernel_expansion::FileHandle handle) override {
    assert(handle == 0x44);
  }
};

class FakePhaseTwo final
    : public bandwagon::kernel_expansion::PhaseTwoFileServices {
 public:
  std::string delete_failure;
  std::string move_failure;
  int marker_delete_status = 0;
  std::vector<std::string> events;

  int DeleteFile(std::string_view path) override {
    events.emplace_back("delete:" + std::string(path));
    if (path == bandwagon::kernel_expansion::kPhaseTwoMarkerPath) {
      return marker_delete_status;
    }
    return path == delete_failure ? -1 : 0;
  }
  bool MoveFile(std::string_view source,
                std::string_view destination) override {
    events.emplace_back("move:" + std::string(source) + "->" +
                        std::string(destination));
    return destination != move_failure;
  }
};

}  // namespace

int main() {
  using namespace bandwagon::kernel_expansion;

  FakeFiles files;
  FakePhaseTwo phase;
  assert(CompletePhaseTwoFileRestore(&files, &phase));
  assert(files.freed);
  assert((phase.events == std::vector<std::string>{
      "delete:first.bin", "move:first.bin_->first.bin", "delete:second.bin",
      "move:second.bin_->second.bin", "delete:PPROOT:\\update.phase2"}));

  FakeFiles absent;
  absent.open_status = -1;
  FakePhaseTwo untouched;
  assert(CompletePhaseTwoFileRestore(&absent, &untouched));
  assert(untouched.events.empty() && !absent.freed);

  FakeFiles delete_files;
  FakePhaseTwo delete_failure;
  delete_failure.delete_failure = "first.bin";
  assert(!CompletePhaseTwoFileRestore(&delete_files, &delete_failure));
  assert(delete_files.freed && delete_failure.events.size() == 1);

  FakeFiles move_files;
  FakePhaseTwo move_failure;
  move_failure.move_failure = "first.bin";
  assert(!CompletePhaseTwoFileRestore(&move_files, &move_failure));
  assert(move_files.freed && move_failure.events.size() == 2);

  FakeFiles unterminated;
  unterminated.content = "not-processed-without-newline";
  FakePhaseTwo marker_only;
  assert(CompletePhaseTwoFileRestore(&unterminated, &marker_only));
  assert((marker_only.events ==
          std::vector<std::string>{"delete:PPROOT:\\update.phase2"}));

  FakeFiles marker_files;
  FakePhaseTwo marker_failure;
  marker_failure.marker_delete_status = -1;
  assert(!CompletePhaseTwoFileRestore(&marker_files, &marker_failure));
  assert(marker_files.freed);

  assert(!CompletePhaseTwoFileRestore(nullptr, &phase));
  assert(!CompletePhaseTwoFileRestore(&files, nullptr));

  std::cout << "Kernel Expansion phase-two restore tests passed\n";
}
