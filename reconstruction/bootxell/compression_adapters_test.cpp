#include "compression_adapters.hpp"

#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <limits>
#include <string_view>
#include <vector>

#include "miniz.h"

int main() {
  using namespace bandwagon::bootxell;
  constexpr std::string_view input =
      "xell raw deflate adapter: updxell.bin / flashdmp.bin / updflash.bin";

  std::array<std::uint8_t, 256> compressed{};
  std::size_t compressed_size = compressed.size();
  assert(DeflateRawDeflateBuffer(
             compressed.data(), &compressed_size,
             reinterpret_cast<const std::uint8_t*>(input.data()),
             input.size()) == MZ_OK);
  assert(compressed_size != 0 && compressed_size < compressed.size());

  // A raw stream must not carry the zlib 0x78 header or gzip 0x1f8b header.
  assert(!(compressed[0] == 0x78));
  assert(!(compressed[0] == 0x1f && compressed[1] == 0x8b));

  std::array<std::uint8_t, 256> expanded{};
  std::size_t expanded_size = expanded.size();
  assert(InflateRawDeflateBuffer(expanded.data(), &expanded_size,
                                 compressed.data(),
                                 compressed_size) == MZ_OK);
  assert(expanded_size == input.size());
  assert(std::memcmp(expanded.data(), input.data(), input.size()) == 0);

  std::array<std::uint8_t, 4> too_small{};
  std::size_t too_small_size = too_small.size();
  assert(DeflateRawDeflateBuffer(
             too_small.data(), &too_small_size,
             reinterpret_cast<const std::uint8_t*>(input.data()),
             input.size()) == MZ_BUF_ERROR);

  std::size_t impossible =
      static_cast<std::size_t>(std::numeric_limits<mz_uint32>::max()) + 1u;
  assert(InflateRawDeflateBuffer(expanded.data(), &impossible,
                                 compressed.data(), compressed_size) ==
         MZ_PARAM_ERROR);

  MinizXellCompression adapter;
  std::vector<std::uint8_t> vector_expanded;
  assert(adapter.Decompress(compressed.data(), compressed_size, 256,
                            &vector_expanded));
  assert(vector_expanded.size() == input.size());

  std::cout << "BootXell raw-deflate adapter tests passed\n";
}
