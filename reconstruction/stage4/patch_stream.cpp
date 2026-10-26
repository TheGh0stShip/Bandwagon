#include "patch_stream.hpp"

#include <utility>

namespace bandwagon::stage4 {
namespace {

std::uint32_t LoadBigEndian32(const std::uint8_t* input) {
  return (static_cast<std::uint32_t>(input[0]) << 24) |
         (static_cast<std::uint32_t>(input[1]) << 16) |
         (static_cast<std::uint32_t>(input[2]) << 8) | input[3];
}

}  // namespace

std::uint64_t ExpandHypervisorAddress(std::uint32_t raw_address) {
  return static_cast<std::uint64_t>(raw_address) |
         (static_cast<std::uint64_t>(raw_address & 0x00030000u) << 17);
}

bool ParsePatchStream(const std::uint8_t* data, std::size_t size,
                      std::vector<PatchRecord>* output) {
  if (!data || !output || size < 4) return false;
  std::vector<PatchRecord> parsed;
  std::size_t cursor = 0;
  while (cursor < size) {
    if (size - cursor < 4) return false;
    const std::uint32_t target = LoadBigEndian32(data + cursor);
    if (target == 0xffffffffu) {
      if (cursor + 4 != size) return false;
      *output = std::move(parsed);
      return true;
    }
    if (size - cursor < 8) return false;
    const std::uint32_t payload_size = LoadBigEndian32(data + cursor + 4);
    const std::size_t payload_offset = cursor + 8;
    if (payload_size > size - payload_offset) return false;
    const std::size_t payload_end = payload_offset + payload_size;
    PatchRecord record;
    record.header_offset = cursor;
    record.raw_target = target;
    record.effective_target = ExpandHypervisorAddress(target);
    record.payload.assign(data + payload_offset, data + payload_end);
    parsed.push_back(std::move(record));
    cursor = payload_end;
  }
  return false;
}

}  // namespace bandwagon::stage4
