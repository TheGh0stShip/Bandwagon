#include "runtime.hpp"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {

class FakeCompression final : public bandwagon::bootxell::XellCompression {
 public:
  bool Decompress(const std::uint8_t*, std::size_t, std::size_t,
                  std::vector<std::uint8_t>*) override { return false; }
  bool Compress(const std::vector<std::uint8_t>&, std::uint8_t*,
                std::size_t*) override { return false; }
};

class FakeParser final : public bandwagon::bootxell::IniStringParser {
 public:
  bool result = true;
  bool ParseString(const char*, bandwagon::bootxell::IniHandler handler,
                   void* user) override {
    handler(user, "BootOption", nullptr, nullptr);
    handler(user, "BootOption", "Name", "Recovery");
    handler(user, "BootOption", "FileName", "recovery.bin");
    handler(user, nullptr, nullptr, nullptr);
    return result;
  }
};

class FakeServices final : public bandwagon::bootxell::BootXellServices {
 public:
  std::unordered_map<std::string, std::vector<std::uint8_t>> files;
  std::unordered_map<bandwagon::bootxell::FileHandle, std::string> open;
  std::vector<std::uint8_t*> heap;
  std::vector<std::uint8_t*> physical;
  std::vector<std::string> events;
  std::string requested = "Recovery";
  bandwagon::bootxell::RbtrLaunchRequest submitted{};
  bool submitted_request = false;
  bandwagon::bootxell::FileHandle next = 1;

  ~FakeServices() override {
    for (auto* value : heap) delete[] value;
    for (auto* value : physical) delete[] value;
  }
  bandwagon::bootxell::FileHandle OpenRead(std::string_view path) override {
    events.emplace_back("open:" + std::string(path));
    if (!files.count(std::string(path)))
      return bandwagon::bootxell::kInvalidFileHandle;
    const auto handle = next++;
    open[handle] = std::string(path);
    return handle;
  }
  bool QueryFileSize(bandwagon::bootxell::FileHandle handle,
                     std::uint32_t* size) override {
    events.emplace_back("size");
    *size = static_cast<std::uint32_t>(files.at(open.at(handle)).size());
    return true;
  }
  bool ReadFile(bandwagon::bootxell::FileHandle handle,
                std::uint8_t* destination, std::uint32_t size,
                std::uint32_t* bytes_read) override {
    events.emplace_back("read");
    const auto& data = files.at(open.at(handle));
    std::copy_n(data.begin(), size, destination);
    *bytes_read = size;
    return true;
  }
  void CloseFile(bandwagon::bootxell::FileHandle handle) override {
    events.emplace_back("close");
    open.erase(handle);
  }
  std::uint8_t* AllocateHeap(std::size_t size) override {
    events.emplace_back("alloc-heap");
    auto* result = new std::uint8_t[size];
    heap.push_back(result);
    return result;
  }
  void FreeHeap(std::uint8_t* allocation) override {
    events.emplace_back("free-heap");
    const auto found = std::find(heap.begin(), heap.end(), allocation);
    assert(found != heap.end());
    delete[] allocation;
    heap.erase(found);
  }
  std::uint8_t* AllocatePhysical(std::uint32_t size, std::uint32_t alignment,
                                 std::uint32_t flags) override {
    events.emplace_back("alloc-physical");
    assert((size & 127u) == 0 && alignment == 128 && flags == 0x204);
    auto* result = new std::uint8_t[size];
    physical.push_back(result);
    return result;
  }
  void FreePhysical(std::uint8_t* allocation) override {
    events.emplace_back("free-physical");
    const auto found = std::find(physical.begin(), physical.end(), allocation);
    assert(found != physical.end());
    delete[] allocation;
    physical.erase(found);
  }
  std::uint32_t PhysicalAddress(const std::uint8_t*) override {
    events.emplace_back("physical-address");
    return 0x1c100000;
  }
  void SubmitRbtr(
      const bandwagon::bootxell::RbtrLaunchRequest& request) override {
    events.emplace_back("rbtr");
    submitted = request;
    submitted_request = true;
  }
  bool ReadRequestedOptionName(char* destination,
                               std::size_t capacity) override {
    events.emplace_back("requested-option");
    if (requested.size() >= capacity) return false;
    std::memcpy(destination, requested.c_str(), requested.size() + 1);
    return true;
  }
};

}  // namespace

int main() {
  using namespace bandwagon::bootxell;
  FakeServices services;
  services.files[std::string(kBootIniPath)] = {'i', 'n', 'i'};
  services.files["PPROOT:\\OtherOS\\recovery.bin"] =
      std::vector<std::uint8_t>(129, 0x42);
  FakeParser parser;
  FakeCompression compression;

  assert(BootXellMain(&services, &parser, &compression));
  assert(services.submitted_request);
  assert(services.submitted.magic == 0x72627472 &&
         services.submitted.command == 4 &&
         services.submitted.word_count == 64 &&
         services.submitted.physical_alias == 0x800000001c100000ULL);
  assert(services.heap.empty() && services.physical.empty() &&
         services.open.empty());

  FakeServices marked;
  marked.files["PPROOT:\\OtherOS\\marked.bin"] =
      std::vector<std::uint8_t>(32, 'x');
  BootOption option;
  option.file_name = "marked.bin";
  assert(!LaunchBootOption(&marked, &compression, option));
  assert(!marked.submitted_request && marked.physical.empty() &&
         marked.open.empty());
  option.bypass_xell_scanning = true;
  assert(LaunchBootOption(&marked, &compression, option));

  FakeServices read_services;
  read_services.files["config"] = {'a', 'b', 'c'};
  std::uint8_t* buffer = nullptr;
  std::uint32_t size = 0;
  assert(ReadFileToBuffer(&read_services, "config", &buffer, &size));
  assert(size == 4 && std::strcmp(reinterpret_cast<char*>(buffer), "abc") == 0);
  read_services.FreeHeap(buffer);

  std::cout << "BootXell runtime reconstruction tests passed\n";
}
