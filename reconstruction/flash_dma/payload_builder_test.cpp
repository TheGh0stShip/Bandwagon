#include "payload_builder.hpp"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

namespace {

std::uint32_t ReadBigEndian(
    const std::array<std::uint8_t, bandwagon::flash_dma::kPayloadSize>& data,
    std::size_t offset) {
  return (static_cast<std::uint32_t>(data[offset]) << 24) |
         (static_cast<std::uint32_t>(data[offset + 1]) << 16) |
         (static_cast<std::uint32_t>(data[offset + 2]) << 8) |
         data[offset + 3];
}

}  // namespace

int main(int argc, char** argv) {
  using namespace bandwagon::flash_dma;
  assert(argc == 2);
  std::ifstream input(argv[1], std::ios::binary);
  assert(input);
  const std::vector<std::uint8_t> expected{
      std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
  const auto built = BuildFlashDmaPayload();

  assert(expected.size() == kPayloadSize);
  assert(std::equal(built.begin(), built.end(), expected.begin()));
  assert(FlashDmaOperations().size() == 14);
  assert(FlashDmaOperations().front().kind ==
         OperationKind::kSwitchToPayloadStack);
  assert(FlashDmaOperations().back().kind ==
         OperationKind::kProvideScratchSector);
  assert(OperationName(OperationKind::kAllocateMappedMemory) ==
         "allocate mapped memory");

  assert(ReadBigEndian(built, 0x01f0) == kPayloadBase + 0x200);
  assert(ReadBigEndian(built, 0x0534) == 0x80080d28);
  assert(ReadBigEndian(built, 0x0c34) == 0x8006b928);
  assert(ReadBigEndian(built, 0x0e30) == 0x99ffff00);
  assert(ReadBigEndian(built, 0x1400) == 0x00b11c00);
  assert(ReadBigEndian(built, 0x1600) == 0x41414141);
  assert(ReadBigEndian(built, 0x17fc) == 0x41414141);
  assert(ReadBigEndian(built, 0x1800) == 0);

  // The serializer remains a real builder: changing the symbol table changes
  // only the fields that refer to that symbol, rather than copying a blob.
  Retail17559Symbols changed;
  changed.unresolved_global_801c7f80 = 0x12345678;
  const auto rebuilt = BuildFlashDmaPayload(changed);
  assert(ReadBigEndian(rebuilt, 0x0804) == 0x12345678);
  assert(ReadBigEndian(rebuilt, 0x08d4) == 0x801a614c);

  std::cout << "Flash DMA typed reconstruction tests passed\n";
}
