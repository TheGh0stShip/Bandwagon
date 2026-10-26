#pragma once

#include "xell_scan.hpp"

#include <cstddef>
#include <cstdint>

namespace bandwagon::bootxell {

// Readable reconstructions of the two app-local miniz derivatives at
// 0x91a05260 and 0x91a05338. Unlike miniz's stock helpers, both select a raw
// DEFLATE stream (window bits -15) because SanitizeXellImage has already
// removed the ten-byte gzip header.
int InflateRawDeflateBuffer(std::uint8_t* destination,
                            std::size_t* destination_size,
                            const std::uint8_t* source,
                            std::size_t source_size);
int DeflateRawDeflateBuffer(std::uint8_t* destination,
                            std::size_t* destination_size,
                            const std::uint8_t* source,
                            std::size_t source_size);

// Concrete compression boundary used by the source replacement. The original
// app-local wrappers above remain small; all codec machinery stays in the
// pinned miniz 3.1.1 source tree.
class MinizXellCompression final : public XellCompression {
 public:
  bool Decompress(const std::uint8_t* input, std::size_t input_size,
                  std::size_t output_limit,
                  std::vector<std::uint8_t>* output) override;
  bool Compress(const std::vector<std::uint8_t>& input,
                std::uint8_t* output,
                std::size_t* output_capacity) override;
};

}  // namespace bandwagon::bootxell
