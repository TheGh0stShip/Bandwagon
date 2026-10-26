#include "xell_scan.hpp"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

namespace {

class FakeCompression final : public bandwagon::bootxell::XellCompression {
 public:
  std::vector<std::uint8_t> expanded;
  bool decompress_result = true;
  bool compress_result = true;
  int decompress_calls = 0;
  int compress_calls = 0;

  bool Decompress(const std::uint8_t*, std::size_t input_size,
                  std::size_t output_limit,
                  std::vector<std::uint8_t>* output) override {
    ++decompress_calls;
    assert(input_size != 0);
    assert(output_limit == bandwagon::bootxell::kXellDecompressionLimit);
    *output = expanded;
    return decompress_result;
  }
  bool Compress(const std::vector<std::uint8_t>& input,
                std::uint8_t* output,
                std::size_t* output_capacity) override {
    ++compress_calls;
    assert(std::find(input.begin(), input.end(), 1) != input.end());
    if (*output_capacity < 2) return false;
    output[0] = 0xaa;
    output[1] = 0x55;
    *output_capacity = 2;
    return compress_result;
  }
};

}  // namespace

int main() {
  using namespace bandwagon::bootxell;

  std::string text = "prefix/updxell.bin middle flashdmp.bin updflash.bin";
  std::vector<std::uint8_t> direct(text.begin(), text.end());
  FakeCompression unused;
  assert(SanitizeXellImage(direct.data(), direct.size(), &unused));
  assert(unused.decompress_calls == 0 && unused.compress_calls == 0);
  assert(std::search(direct.begin(), direct.end(), text.begin() + 7,
                     text.begin() + 18) == direct.end());
  assert(std::count(direct.begin(), direct.end(), 1) == 3);

  std::vector<std::uint8_t> clean{'n', 'o', 'n', 'e'};
  assert(!SanitizeDangerousXellNames(clean.data(), clean.size()));

  const std::size_t image_size = kXellGzipDataOffset + 32 +
                                 kXellScanMarkerSize;
  std::vector<std::uint8_t> gzip(image_size, 0xcc);
  gzip[kXellGzipHeaderOffset] = 0x1f;
  gzip[kXellGzipHeaderOffset + 1] = 0x8b;
  gzip[kXellGzipHeaderOffset + 2] = 8;
  std::fill(gzip.end() - kXellScanMarkerSize, gzip.end(), 'x');
  FakeCompression codec;
  const std::string embedded = "boot/updflash.bin";
  codec.expanded.assign(embedded.begin(), embedded.end());
  assert(SanitizeXellImage(gzip.data(), gzip.size(), &codec));
  assert(codec.decompress_calls == 1 && codec.compress_calls == 1);
  assert(gzip[kXellGzipDataOffset] == 0xaa &&
         gzip[kXellGzipDataOffset + 1] == 0x55);
  assert(std::all_of(gzip.end() - kXellScanMarkerSize, gzip.end(),
                     [](std::uint8_t value) { return value == 'x'; }));

  gzip[kXellGzipHeaderOffset] = 0;
  assert(!SanitizeXellImage(gzip.data(), gzip.size(), &codec));

  std::cout << "BootXell sanitizer reconstruction tests passed\n";
}
