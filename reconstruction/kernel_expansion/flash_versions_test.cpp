#include "flash_versions.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>

namespace {

using namespace bandwagon::kernel_expansion;

void StoreBigEndian32(std::array<std::uint8_t, kNandXbppRecordSize>* data,
                      std::size_t offset, std::uint32_t value) {
  (*data)[offset] = static_cast<std::uint8_t>(value >> 24);
  (*data)[offset + 1] = static_cast<std::uint8_t>(value >> 16);
  (*data)[offset + 2] = static_cast<std::uint8_t>(value >> 8);
  (*data)[offset + 3] = static_cast<std::uint8_t>(value);
}

class FakeFlash final : public FlashVersionServices {
 public:
  std::uint32_t open_status = 0;
  std::uint32_t read_status = 0;
  std::uint32_t close_status = 0;
  std::uint32_t configuration = 0;
  FlashHandle opened_handle = 0x1234;
  bool populate_handle_on_failure = false;
  std::array<std::uint8_t, kNandXbppRecordSize> record{};
  int open_calls = 0;
  int configuration_calls = 0;
  int read_calls = 0;
  int close_calls = 0;
  FlashHandle read_handle = 0;
  std::uint64_t read_offset = 0;
  std::size_t read_size = 0;

  std::uint32_t Open(const FlashOpenRequest& request,
                     FlashHandle* handle) override {
    ++open_calls;
    assert(request.path == kFlashDevicePath);
    assert(request.desired_access == 0xc0100000u);
    assert(request.object_attributes == 0x40u);
    assert(request.file_attributes == 0x80u);
    assert(request.share_access == 3u);
    assert(request.create_disposition == 1u);
    assert(request.create_options == 0x20u);
    if (open_status == 0 || populate_handle_on_failure) {
      *handle = opened_handle;
    }
    return open_status;
  }

  std::uint32_t ReadFlashConfiguration() override {
    ++configuration_calls;
    return configuration;
  }

  std::uint32_t Read(FlashHandle handle, std::uint64_t offset,
                     std::uint8_t* buffer, std::size_t size) override {
    ++read_calls;
    read_handle = handle;
    read_offset = offset;
    read_size = size;
    if (read_status == 0) std::copy(record.begin(), record.end(), buffer);
    return read_status;
  }

  std::uint32_t Close(FlashHandle handle) override {
    ++close_calls;
    assert(handle == opened_handle);
    return close_status;
  }
};

}  // namespace

int main() {
  using namespace bandwagon::kernel_expansion;

  constexpr std::uint32_t expected[3][4] = {
      {0x007be000u, 0x00f7c000u, 0x01efc000u, 0x03dfc000u},
      {0x00f7c000u, 0x00f7c000u, 0x03be0000u, 0x03bc0000u},
      {0x00f7c000u, 0x03dfc000u, 0x03be0000u, 0x03bc0000u},
  };
  for (std::uint32_t row = 0; row < 3; ++row) {
    for (std::uint32_t column = 0; column < 4; ++column) {
      const std::uint32_t configuration =
          (row << 17) | (column << 4) | 0xa5a0000du;
      assert(ResolveNandXbppRecordOffset(configuration) ==
             expected[row][column]);
    }
  }
  for (std::uint32_t column = 0; column < 4; ++column) {
    assert(ResolveNandXbppRecordOffset((3u << 17) | (column << 4)) ==
           kInvalidNandXbppRecordOffset);
  }

  FakeFlash untouched;
  std::uint32_t current = 0xaaaaaaaau;
  std::uint32_t installed = 0xbbbbbbbbu;
  assert(QueryPeerPressureVersionsFromFlash(&untouched, nullptr, &installed) ==
         kVersionQueryStatusInvalidParameter);
  assert(installed == 0xbbbbbbbbu && untouched.open_calls == 0);
  assert(QueryPeerPressureVersionsFromFlash(&untouched, &current, nullptr) ==
         kVersionQueryStatusInvalidParameter);
  assert(current == 0xaaaaaaaau && untouched.open_calls == 0);
  assert(QueryPeerPressureVersionsFromFlash(nullptr, &current, &installed) ==
         kVersionQueryStatusInvalidParameter);
  assert(current == 0xaaaaaaaau && installed == 0xbbbbbbbbu);

  FakeFlash success;
  success.configuration = (1u << 17) | (2u << 4);
  success.close_status = 0xc0000001u;
  StoreBigEndian32(&success.record, 0x110, kNandXbppMagic);
  StoreBigEndian32(&success.record, 0x114, 0x01000123u);
  assert(QueryPeerPressureVersionsFromFlash(&success, &current, &installed) ==
         kVersionQueryStatusSuccess);
  assert(current == kCurrentPeerPressureVersion);
  assert(installed == 0x01000123u);
  assert(success.open_calls == 1 && success.configuration_calls == 1);
  assert(success.read_calls == 1 && success.close_calls == 1);
  assert(success.read_handle == success.opened_handle);
  assert(success.read_offset == 0x03be0000u);
  assert(success.read_size == kNandXbppRecordSize);

  for (const std::uint32_t failure : {0xc0000001u, 0x40000001u}) {
    FakeFlash open_failure;
    open_failure.open_status = failure;
    open_failure.populate_handle_on_failure = failure == 0xc0000001u;
    current = installed = 0xffffffffu;
    assert(QueryPeerPressureVersionsFromFlash(
               &open_failure, &current, &installed) == failure);
    assert(current == kCurrentPeerPressureVersion && installed == 0);
    assert(open_failure.configuration_calls == 0 &&
           open_failure.read_calls == 0);
    assert(open_failure.close_calls ==
           (open_failure.populate_handle_on_failure ? 1 : 0));
  }

  FakeFlash invalid_offset;
  invalid_offset.configuration = 3u << 17;
  assert(QueryPeerPressureVersionsFromFlash(
             &invalid_offset, &current, &installed) ==
         kVersionQueryStatusIoDeviceError);
  assert(installed == 0 && invalid_offset.read_calls == 0);
  assert(invalid_offset.close_calls == 1);

  for (const std::uint32_t failure : {0xc0000001u, 0x40000001u}) {
    FakeFlash read_failure;
    read_failure.read_status = failure;
    assert(QueryPeerPressureVersionsFromFlash(
               &read_failure, &current, &installed) == failure);
    assert(installed == 0 && read_failure.read_calls == 1);
    assert(read_failure.close_calls == 1);
  }

  FakeFlash bad_magic;
  StoreBigEndian32(&bad_magic.record, 0x114, 0x01000999u);
  assert(QueryPeerPressureVersionsFromFlash(&bad_magic, &current, &installed) ==
         kVersionQueryStatusDataError);
  assert(installed == 0 && bad_magic.close_calls == 1);

  FakeFlash zero_handle;
  zero_handle.opened_handle = 0;
  assert(QueryPeerPressureVersionsFromFlash(
             &zero_handle, &current, &installed) ==
         kVersionQueryStatusDataError);
  assert(zero_handle.read_calls == 1 && zero_handle.close_calls == 0);

  std::cout << "Kernel Expansion flash-version tests passed\n";
}
