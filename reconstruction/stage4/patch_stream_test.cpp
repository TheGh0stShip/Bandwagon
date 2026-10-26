#include "patch_stream.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

namespace {

void Append32(std::vector<std::uint8_t>* bytes, std::uint32_t value) {
  bytes->push_back(static_cast<std::uint8_t>(value >> 24));
  bytes->push_back(static_cast<std::uint8_t>(value >> 16));
  bytes->push_back(static_cast<std::uint8_t>(value >> 8));
  bytes->push_back(static_cast<std::uint8_t>(value));
}

void AppendRecord(std::vector<std::uint8_t>* bytes, std::uint32_t target,
                  std::initializer_list<std::uint8_t> payload) {
  Append32(bytes, target);
  Append32(bytes, static_cast<std::uint32_t>(payload.size()));
  bytes->insert(bytes->end(), payload);
}

}  // namespace

int main() {
  using bandwagon::stage4::ExpandHypervisorAddress;
  using bandwagon::stage4::ParsePatchStream;

  assert(ExpandHypervisorAddress(0x00007698u) == 0x7698u);
  assert(ExpandHypervisorAddress(0x00015fd0u) == 0x200015fd0ULL);
  assert(ExpandHypervisorAddress(0x0002a308u) == 0x40002a308ULL);
  assert(ExpandHypervisorAddress(0x00031fb8u) == 0x600031fb8ULL);

  std::vector<std::uint8_t> stream;
  AppendRecord(&stream, 0x00015fd0u, {0x00, 0x00, 0x76, 0xb8});
  AppendRecord(&stream, 0x00031fb8u, {1, 2, 3});
  Append32(&stream, 0xffffffffu);
  std::vector<bandwagon::stage4::PatchRecord> records;
  assert(ParsePatchStream(stream.data(), stream.size(), &records));
  assert(records.size() == 2);
  assert(records[0].header_offset == 0);
  assert(records[0].effective_target == 0x200015fd0ULL);
  assert((records[0].payload == std::vector<std::uint8_t>{0, 0, 0x76, 0xb8}));
  assert(records[1].effective_target == 0x600031fb8ULL);

  const auto before = records;
  auto early = stream;
  early.insert(early.end(), 4, 0);
  assert(!ParsePatchStream(early.data(), early.size(), &records));
  assert(records[0].payload == before[0].payload);
  auto truncated = stream;
  truncated.pop_back();
  assert(!ParsePatchStream(truncated.data(), truncated.size(), &records));
  assert(!ParsePatchStream(nullptr, 0, &records));

  std::cout << "Stage4 patch-stream reconstruction tests passed\n";
}
