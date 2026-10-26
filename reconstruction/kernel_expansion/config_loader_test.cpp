#include "config_loader.hpp"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

using bandwagon::kernel_expansion::ConfigAllocationRequest;
using bandwagon::kernel_expansion::FileHandle;
using bandwagon::kernel_expansion::PeerPressureConfig;

class FakeBootstrap final
    : public bandwagon::kernel_expansion::ConfigBootstrapServices {
 public:
  PeerPressureConfig config{};
  int allocation_status = 0;
  bool provide_config = true;
  int initialize_calls = 0;
  int allocate_calls = 0;
  int fatal_calls = 0;
  std::uint32_t fatal_code = 0;

  void InitializeStateLock() override { ++initialize_calls; }

  int AllocateConfig(const ConfigAllocationRequest& request,
                     PeerPressureConfig** output) override {
    assert(output);
    assert(request.size == 0x170);
    assert(request.allocation_type == 0x1000);
    assert(request.protection == 4);
    assert(request.flags == 2);
    ++allocate_calls;
    *output = provide_config ? &config : nullptr;
    return allocation_status;
  }

  void DisplayFatalError(std::uint32_t code) override {
    ++fatal_calls;
    fatal_code = code;
  }
};

class FakeConfigServices final
    : public bandwagon::kernel_expansion::ConfigServices {
 public:
  int calls = 0;

  void RunLockedConfigPath(std::uint32_t mode) override {
    assert(mode == 0);
    ++calls;
  }
};

class FakeFiles final : public bandwagon::kernel_expansion::FileServices {
 public:
  std::string content = "abc";
  int open_status = 0;
  int open_calls = 0;
  int close_calls = 0;
  int free_calls = 0;
  std::vector<std::uint8_t*> live_allocations;

  ~FakeFiles() override {
    for (std::uint8_t* allocation : live_allocations) delete[] allocation;
  }

  int OpenReadOnly(std::string_view path, bool object_manager_path,
                   FileHandle* handle) override {
    using namespace bandwagon::kernel_expansion;
    assert(path == kPeerPressureConfigPath);
    assert(object_manager_path && handle);
    ++open_calls;
    if (open_status == 0) *handle = static_cast<FileHandle>(0x40 + open_calls);
    return open_status;
  }

  int QueryFileSize(FileHandle handle, std::uint32_t* size) override {
    assert(handle != 0 && size);
    *size = static_cast<std::uint32_t>(content.size());
    return 0;
  }

  std::uint8_t* Allocate(std::uint32_t size) override {
    auto* allocation = new std::uint8_t[size];
    live_allocations.push_back(allocation);
    return allocation;
  }

  int Read(FileHandle handle, std::uint8_t* buffer, std::uint32_t size,
           std::int32_t* io_status) override {
    assert(handle != 0 && buffer && size == content.size() && io_status);
    std::memcpy(buffer, content.data(), size);
    *io_status = 0;
    return 0;
  }

  int WaitForRead(FileHandle) override {
    assert(false);
    return -1;
  }

  void Free(std::uint8_t* buffer) override {
    const auto found =
        std::find(live_allocations.begin(), live_allocations.end(), buffer);
    assert(found != live_allocations.end());
    delete[] buffer;
    live_allocations.erase(found);
    ++free_calls;
  }

  void Close(FileHandle handle) override {
    assert(handle != 0);
    ++close_calls;
  }
};

class FakeParser final
    : public bandwagon::kernel_expansion::StructuredConfigParser {
 public:
  bool result = true;
  int calls = 0;
  PeerPressureConfig* expected_destination = nullptr;
  const void* expected_schema = nullptr;

  bool Parse(const std::uint8_t* text, void* destination,
             const void* schema_entries,
             std::uint32_t entry_count) override {
    using namespace bandwagon::kernel_expansion;
    assert(text && std::memcmp(text, "abc\0", 4) == 0);
    assert(destination == expected_destination);
    assert(schema_entries == expected_schema);
    assert(entry_count == kConfigSchemaEntryCount);
    ++calls;
    static_cast<PeerPressureConfig*>(destination)->storage[7] = 0x5a;
    return result;
  }
};

void PrepareParser(FakeParser* parser, FakeBootstrap* bootstrap,
                   const void* schema) {
  parser->expected_destination = &bootstrap->config;
  parser->expected_schema = schema;
}

}  // namespace

int main() {
  using namespace bandwagon::kernel_expansion;

  const std::uint32_t schema_marker = 0x12345678;

  ConfigurationState state{};
  FakeBootstrap bootstrap;
  FakeFiles files;
  FakeParser parser;
  FakeConfigServices config_services;
  PrepareParser(&parser, &bootstrap, &schema_marker);
  assert(LoadPeerPressureConfig(&state, &bootstrap, &files, &parser,
                                &config_services, &schema_marker));
  assert(state.singleton == &bootstrap.config);
  assert(bootstrap.initialize_calls == 1 && bootstrap.allocate_calls == 1);
  assert(bootstrap.fatal_calls == 0);
  assert(files.open_calls == 2 && files.close_calls == 2);
  assert(files.free_calls == 2 && files.live_allocations.empty());
  assert(parser.calls == 1 && bootstrap.config.storage[7] == 0x5a);
  assert(config_services.calls == 0);

  ConfigurationState missing_state{};
  FakeBootstrap missing_bootstrap;
  FakeFiles missing_files;
  missing_files.open_status = -1;
  FakeParser missing_parser;
  FakeConfigServices missing_services;
  PrepareParser(&missing_parser, &missing_bootstrap, &schema_marker);
  assert(!LoadPeerPressureConfig(
      &missing_state, &missing_bootstrap, &missing_files, &missing_parser,
      &missing_services, &schema_marker));
  assert(missing_files.open_calls == 1 && missing_files.close_calls == 0);
  assert(missing_files.live_allocations.empty() && missing_parser.calls == 0);
  assert(missing_services.calls == 1);
  assert(missing_bootstrap.config.DefaultEnabledFlag());

  ConfigurationState parse_failure_state{};
  FakeBootstrap parse_failure_bootstrap;
  parse_failure_bootstrap.config.storage.fill(0xcc);
  FakeFiles parse_failure_files;
  FakeParser parse_failure_parser;
  parse_failure_parser.result = false;
  FakeConfigServices parse_failure_services;
  PrepareParser(&parse_failure_parser, &parse_failure_bootstrap,
                &schema_marker);
  assert(!LoadPeerPressureConfig(
      &parse_failure_state, &parse_failure_bootstrap, &parse_failure_files,
      &parse_failure_parser, &parse_failure_services, &schema_marker));
  assert(parse_failure_files.open_calls == 2);
  assert(parse_failure_files.free_calls == 2 &&
         parse_failure_files.live_allocations.empty());
  assert(parse_failure_parser.calls == 1);
  assert(parse_failure_services.calls == 0);
  assert(parse_failure_bootstrap.config.storage[7] == 0);
  assert(parse_failure_bootstrap.config.DefaultEnabledFlag());

  ConfigurationState allocation_failure_state{};
  FakeBootstrap allocation_failure_bootstrap;
  allocation_failure_bootstrap.allocation_status = -1;
  allocation_failure_bootstrap.provide_config = false;
  FakeFiles allocation_failure_files;
  FakeParser allocation_failure_parser;
  FakeConfigServices allocation_failure_services;
  assert(!LoadPeerPressureConfig(
      &allocation_failure_state, &allocation_failure_bootstrap,
      &allocation_failure_files, &allocation_failure_parser,
      &allocation_failure_services, &schema_marker));
  assert(allocation_failure_bootstrap.fatal_calls == 1);
  assert(allocation_failure_bootstrap.fatal_code == kConfigFatalErrorCode);
  assert(allocation_failure_files.open_calls == 0);

  ConfigurationState warning_state{};
  FakeBootstrap warning_bootstrap;
  warning_bootstrap.allocation_status = -1;
  FakeFiles warning_files;
  FakeParser warning_parser;
  FakeConfigServices warning_services;
  PrepareParser(&warning_parser, &warning_bootstrap, &schema_marker);
  assert(LoadPeerPressureConfig(
      &warning_state, &warning_bootstrap, &warning_files, &warning_parser,
      &warning_services, &schema_marker));
  assert(warning_bootstrap.fatal_calls == 1);
  assert(warning_bootstrap.fatal_code == kConfigFatalErrorCode);
  assert(warning_parser.calls == 1);

  assert(!LoadPeerPressureConfig(nullptr, &bootstrap, &files, &parser,
                                 &config_services, &schema_marker));
  assert(!LoadPeerPressureConfig(&state, nullptr, &files, &parser,
                                 &config_services, &schema_marker));

  std::cout << "Kernel Expansion config loader tests passed\n";
}
