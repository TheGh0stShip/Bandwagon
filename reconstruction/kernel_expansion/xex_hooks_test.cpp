#include "xex_hooks.hpp"

#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

namespace {

void StoreLittleEndian16(std::uint8_t* output, std::uint16_t value) {
  output[0] = static_cast<std::uint8_t>(value);
  output[1] = static_cast<std::uint8_t>(value >> 8);
}

void StoreLittleEndian32(std::uint8_t* output, std::uint32_t value) {
  output[0] = static_cast<std::uint8_t>(value);
  output[1] = static_cast<std::uint8_t>(value >> 8);
  output[2] = static_cast<std::uint8_t>(value >> 16);
  output[3] = static_cast<std::uint8_t>(value >> 24);
}

void StoreBigEndian32(std::uint8_t* output, std::uint32_t value) {
  output[0] = static_cast<std::uint8_t>(value >> 24);
  output[1] = static_cast<std::uint8_t>(value >> 16);
  output[2] = static_cast<std::uint8_t>(value >> 8);
  output[3] = static_cast<std::uint8_t>(value);
}

std::uint32_t ReadBigEndian32(const std::uint8_t* value) {
  return (static_cast<std::uint32_t>(value[0]) << 24) |
         (static_cast<std::uint32_t>(value[1]) << 16) |
         (static_cast<std::uint32_t>(value[2]) << 8) | value[3];
}

class FakeServices final
    : public bandwagon::kernel_expansion::XexHookServices {
 public:
  bool find_result = true;
  std::size_t nt_offset = 0x80;
  std::vector<std::string> events;

  bool FindNtHeader(const std::uint8_t*, std::size_t,
                    std::size_t* output) override {
    events.emplace_back("find-nt");
    *output = nt_offset;
    return find_result;
  }
  void FlushCpuCache(std::size_t image_offset, std::size_t size) override {
    assert(image_offset == 0x400 && size == 0x1004);
    events.emplace_back("flush-cpu");
  }
  std::uint64_t PhysicalAddress(std::size_t image_offset) override {
    assert(image_offset == 0x400);
    events.emplace_back("physical");
    return 0x12345000;
  }
  void FlushHypervisorDataCache(std::uint64_t physical,
                                std::size_t size) override {
    assert(physical == 0x12345000 && size == 0x1004);
    events.emplace_back("flush-hv");
  }
};

std::vector<std::uint8_t> TestKernelImage() {
  std::vector<std::uint8_t> image(0x2000);
  constexpr std::size_t nt = 0x80;
  constexpr std::size_t optional_size = 0xf0;
  constexpr std::size_t sections = nt + 0x18 + optional_size;
  StoreLittleEndian16(image.data() + nt + 6, 2);
  StoreLittleEndian16(image.data() + nt + 0x14, optional_size);
  std::memcpy(image.data() + sections, ".text", 5);
  std::uint8_t* xedata_section = image.data() + sections + 0x28;
  std::memcpy(xedata_section, ".xedata", 7);
  StoreLittleEndian32(xedata_section + 0x0c, 0x400);
  StoreLittleEndian32(xedata_section + 0x10, 0x1000);

  std::uint8_t* xedata = image.data() + 0x400;
  StoreBigEndian32(xedata + 0x20, 0);
  StoreBigEndian32(xedata + 0x24, 0x100);
  StoreBigEndian32(xedata + 0x28, 0x2a4);
  return image;
}

}  // namespace

int main() {
  using namespace bandwagon::kernel_expansion;

  auto image = TestKernelImage();
  FakeServices services;
  assert(InstallKernelXexDataHandlers(image.data(), image.size(), &services));
  const std::size_t first_slot = 0x400 + 0x2c + 0x100 * 4;
  const std::size_t second_slot = 0x400 + 0x2c + (0x30c - 0x2a4) * 4;
  assert(ReadBigEndian32(image.data() + first_slot) == kFirstXexDataHandler);
  assert(ReadBigEndian32(image.data() + second_slot) == kSecondXexDataHandler);
  assert(ReadBigEndian32(image.data() + 0x400 + 0x24) == 0x101);
  assert((services.events == std::vector<std::string>{
      "find-nt", "flush-cpu", "physical", "flush-hv"}));

  image = TestKernelImage();
  StoreBigEndian32(image.data() + 0x400 + 0x28, 0x2a3);
  FakeServices bad_counts;
  assert(!InstallKernelXexDataHandlers(
      image.data(), image.size(), &bad_counts));
  assert((bad_counts.events == std::vector<std::string>{"find-nt"}));

  image = TestKernelImage();
  const std::size_t section = 0x80 + 0x18 + 0xf0 + 0x28;
  image[section] = 'X';
  FakeServices no_section;
  assert(!InstallKernelXexDataHandlers(
      image.data(), image.size(), &no_section));
  assert((no_section.events == std::vector<std::string>{"find-nt"}));

  assert(!InstallKernelXexDataHandlers(nullptr, 0, &services));
  assert(!InstallKernelXexDataHandlers(image.data(), image.size(), nullptr));

  std::cout << "Kernel Expansion XEX hook reconstruction tests passed\n";
}
