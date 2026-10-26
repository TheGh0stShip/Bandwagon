#include "runtime.hpp"

#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace {

class FakeModuleServices final
    : public bandwagon::kernel_expansion::ModuleServices {
 public:
  int load_result = 0;
  int resolve_result = 0;
  bandwagon::kernel_expansion::ModuleHandle returned_module = 0x1234;
  bandwagon::kernel_expansion::Procedure returned_procedure = 0x5678;
  std::vector<std::string> events;

  int LoadImage(std::string_view path, std::uint32_t flags,
                std::uint32_t minimum_version,
                bandwagon::kernel_expansion::ModuleHandle* module) override {
    assert(path == bandwagon::kernel_expansion::kBootXellPath);
    assert(flags == 8 && minimum_version == 0 && module);
    events.emplace_back("load");
    *module = returned_module;
    return load_result;
  }
  int ResolveProcedure(
      bandwagon::kernel_expansion::ModuleHandle module,
      std::uint32_t ordinal,
      bandwagon::kernel_expansion::Procedure* procedure) override {
    assert(module == returned_module && ordinal == 1 && procedure);
    events.emplace_back("resolve");
    *procedure = returned_procedure;
    return resolve_result;
  }
  void InvokeProcedure(
      bandwagon::kernel_expansion::Procedure procedure) override {
    assert(procedure == returned_procedure);
    events.emplace_back("invoke");
  }
  void UnloadImage(
      bandwagon::kernel_expansion::ModuleHandle module) override {
    assert(module == returned_module);
    events.emplace_back("unload");
  }
};

class FakeFileServices final
    : public bandwagon::kernel_expansion::FileServices {
 public:
  int open_result = 0;
  int query_result = 0;
  int read_result = 0;
  int wait_result = 0;
  std::int32_t io_status = 0;
  std::uint32_t file_size = 3;
  bool allocation_failure = false;
  bandwagon::kernel_expansion::FileHandle returned_handle = 0x2222;
  std::vector<std::string> events;
  std::unique_ptr<std::uint8_t[]> owned;

  int OpenReadOnly(std::string_view path, bool object_manager_path,
                   bandwagon::kernel_expansion::FileHandle* handle) override {
    assert(path == "\\Device\\Harddisk0\\test.bin");
    assert(object_manager_path && handle);
    events.emplace_back("open");
    *handle = returned_handle;
    return open_result;
  }
  int QueryFileSize(bandwagon::kernel_expansion::FileHandle handle,
                    std::uint32_t* size) override {
    assert(handle == returned_handle && size);
    events.emplace_back("query");
    *size = file_size;
    return query_result;
  }
  std::uint8_t* Allocate(std::uint32_t size) override {
    assert(size == file_size + 1);
    events.emplace_back("allocate");
    if (allocation_failure) return nullptr;
    owned = std::make_unique<std::uint8_t[]>(size);
    return owned.get();
  }
  int Read(bandwagon::kernel_expansion::FileHandle handle,
           std::uint8_t* buffer, std::uint32_t size,
           std::int32_t* completion_status) override {
    assert(handle == returned_handle && buffer == owned.get());
    assert(size == file_size && completion_status);
    events.emplace_back("read");
    if (size >= 3) {
      buffer[0] = 'a';
      buffer[1] = 'b';
      buffer[2] = 'c';
    }
    *completion_status = io_status;
    return read_result;
  }
  int WaitForRead(
      bandwagon::kernel_expansion::FileHandle handle) override {
    assert(handle == returned_handle);
    events.emplace_back("wait");
    return wait_result;
  }
  void Free(std::uint8_t* buffer) override {
    assert(buffer == owned.get());
    events.emplace_back("free");
    owned.reset();
  }
  void Close(bandwagon::kernel_expansion::FileHandle handle) override {
    assert(handle == returned_handle);
    events.emplace_back("close");
  }
};

std::array<std::uint8_t, 0x5c> FixedPointGptHeader() {
  std::array<std::uint8_t, 0x5c> header{};
  constexpr std::array<std::uint8_t, 12> prefix{{
      'E', 'F', 'I', ' ', 'P', 'A', 'R', 'T', 0, 0, 1, 0,
  }};
  std::copy(prefix.begin(), prefix.end(), header.begin());
  header[0x0c] = 0x5c;
  // This little-endian value is the CRC fixed point for this test header. It
  // exercises the binary's unusual self-including checksum comparison.
  header[0x10] = 0x4a;
  header[0x11] = 0x5c;
  header[0x12] = 0x85;
  header[0x13] = 0x8a;
  header[0x50] = 0x80;
  header[0x54] = 0x80;
  return header;
}

}  // namespace

int main() {
  using namespace bandwagon::kernel_expansion;

  FakeModuleServices success;
  LaunchBootXell(&success);
  assert((success.events ==
          std::vector<std::string>{"load", "resolve", "invoke", "unload"}));

  FakeModuleServices resolve_failure;
  resolve_failure.resolve_result = -1;
  LaunchBootXell(&resolve_failure);
  assert((resolve_failure.events ==
          std::vector<std::string>{"load", "resolve", "unload"}));

  FakeModuleServices load_failure;
  load_failure.load_result = -1;
  LaunchBootXell(&load_failure);
  assert((load_failure.events == std::vector<std::string>{"load", "unload"}));

  FakeModuleServices empty_failure;
  empty_failure.load_result = -1;
  empty_failure.returned_module = 0;
  LaunchBootXell(&empty_failure);
  assert((empty_failure.events == std::vector<std::string>{"load"}));
  LaunchBootXell(nullptr);

  constexpr std::string_view kTestPath = "\\Device\\Harddisk0\\test.bin";
  FakeFileServices file_success;
  std::uint8_t* data = nullptr;
  std::uint32_t size = 0;
  assert(ReadFileToAllocatedBuffer(&file_success, kTestPath, &data, &size));
  assert(data == file_success.owned.get() && size == 4);
  assert(data[0] == 'a' && data[1] == 'b' && data[2] == 'c' && data[3] == 0);
  assert((file_success.events == std::vector<std::string>{
      "open", "query", "allocate", "read", "close"}));

  FakeFileServices pending;
  pending.read_result = kStatusPending;
  data = nullptr;
  size = 0;
  assert(ReadFileToAllocatedBuffer(&pending, kTestPath, &data, &size));
  assert((pending.events == std::vector<std::string>{
      "open", "query", "allocate", "read", "wait", "close"}));

  FakeFileServices read_failure;
  read_failure.read_result = -1;
  data = reinterpret_cast<std::uint8_t*>(0x1234);
  size = 99;
  assert(!ReadFileToAllocatedBuffer(
      &read_failure, kTestPath, &data, &size));
  assert(data == reinterpret_cast<std::uint8_t*>(0x1234) && size == 99);
  assert((read_failure.events == std::vector<std::string>{
      "open", "query", "allocate", "read", "free", "close"}));

  FakeFileServices bad_completion;
  bad_completion.io_status = -1;
  assert(!ReadFileToAllocatedBuffer(
      &bad_completion, kTestPath, &data, &size));
  assert((bad_completion.events == std::vector<std::string>{
      "open", "query", "allocate", "read", "free", "close"}));

  FakeFileServices query_failure;
  query_failure.query_result = -1;
  assert(!ReadFileToAllocatedBuffer(
      &query_failure, kTestPath, &data, &size));
  assert((query_failure.events ==
          std::vector<std::string>{"open", "query", "close"}));

  assert(!ReadFileToAllocatedBuffer(nullptr, kTestPath, &data, &size));
  assert(!ReadFileToAllocatedBuffer(&file_success, {}, &data, &size));

  auto header = FixedPointGptHeader();
  assert(ComputeRecoveredGptChecksum(header.data(), header.size()) ==
         0x8a855c4a);
  assert(IsRecognizedGptHeader(header.data(), header.size()));

  header[0] = 'X';
  assert(!IsRecognizedGptHeader(header.data(), header.size()));
  header = FixedPointGptHeader();
  header[0x50] = 0x81;
  assert(!IsRecognizedGptHeader(header.data(), header.size()));
  header = FixedPointGptHeader();
  header[0x54] = 0x40;
  assert(!IsRecognizedGptHeader(header.data(), header.size()));
  header = FixedPointGptHeader();
  header[0x0c] = 0x5b;
  assert(!IsRecognizedGptHeader(header.data(), header.size()));
  assert(!IsRecognizedGptHeader(nullptr, 0));

  assert(IsAsciiAlphaNumeric('0'));
  assert(IsAsciiAlphaNumeric('9'));
  assert(IsAsciiAlphaNumeric('A'));
  assert(IsAsciiAlphaNumeric('Z'));
  assert(IsAsciiAlphaNumeric('a'));
  assert(IsAsciiAlphaNumeric('z'));
  assert(!IsAsciiAlphaNumeric('/'));
  assert(!IsAsciiAlphaNumeric(':'));
  assert(!IsAsciiAlphaNumeric('@'));
  assert(!IsAsciiAlphaNumeric('['));
  assert(!IsAsciiAlphaNumeric('`'));
  assert(!IsAsciiAlphaNumeric('{'));
  assert(!IsAsciiAlphaNumeric(0x80));

  std::cout << "Kernel Expansion runtime reconstruction tests passed\n";
}
