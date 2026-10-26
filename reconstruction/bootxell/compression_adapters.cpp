#include "compression_adapters.hpp"

#include <limits>

#include "miniz.h"

namespace bandwagon::bootxell {
namespace {

constexpr int kRawDeflateWindowBits = -MZ_DEFAULT_WINDOW_BITS;

bool FitsMiniz32(std::size_t value) {
  return value <= std::numeric_limits<mz_uint32>::max();
}

}  // namespace

int InflateRawDeflateBuffer(std::uint8_t* destination,
                            std::size_t* destination_size,
                            const std::uint8_t* source,
                            std::size_t source_size) {
  if (!destination_size || !FitsMiniz32(*destination_size) ||
      !FitsMiniz32(source_size)) {
    return MZ_PARAM_ERROR;
  }

  mz_stream stream{};
  stream.next_in = source;
  stream.avail_in = static_cast<mz_uint32>(source_size);
  stream.next_out = destination;
  stream.avail_out = static_cast<mz_uint32>(*destination_size);

  int status = mz_inflateInit2(&stream, kRawDeflateWindowBits);
  if (status != MZ_OK) return status;

  status = mz_inflate(&stream, MZ_FINISH);
  if (status != MZ_STREAM_END) {
    mz_inflateEnd(&stream);
    return status == MZ_BUF_ERROR && stream.avail_in == 0 ? MZ_DATA_ERROR
                                                          : status;
  }

  *destination_size = static_cast<std::size_t>(stream.total_out);
  return mz_inflateEnd(&stream);
}

int DeflateRawDeflateBuffer(std::uint8_t* destination,
                            std::size_t* destination_size,
                            const std::uint8_t* source,
                            std::size_t source_size) {
  if (!destination_size || !FitsMiniz32(*destination_size) ||
      !FitsMiniz32(source_size)) {
    return MZ_PARAM_ERROR;
  }

  mz_stream stream{};
  stream.next_in = source;
  stream.avail_in = static_cast<mz_uint32>(source_size);
  stream.next_out = destination;
  stream.avail_out = static_cast<mz_uint32>(*destination_size);

  int status = mz_deflateInit2(&stream, MZ_UBER_COMPRESSION, MZ_DEFLATED,
                               kRawDeflateWindowBits, 9,
                               MZ_DEFAULT_STRATEGY);
  if (status != MZ_OK) return status;

  status = mz_deflate(&stream, MZ_FINISH);
  if (status != MZ_STREAM_END) {
    mz_deflateEnd(&stream);
    return status == MZ_OK ? MZ_BUF_ERROR : status;
  }

  *destination_size = static_cast<std::size_t>(stream.total_out);
  return mz_deflateEnd(&stream);
}

bool MinizXellCompression::Decompress(
    const std::uint8_t* input, std::size_t input_size,
    std::size_t output_limit, std::vector<std::uint8_t>* output) {
  if (!output || !FitsMiniz32(output_limit)) return false;
  output->assign(output_limit, 0);
  std::size_t actual_size = output_limit;
  const int status = InflateRawDeflateBuffer(
      output->data(), &actual_size, input, input_size);
  if (status != MZ_OK) {
    output->clear();
    return false;
  }
  output->resize(actual_size);
  return true;
}

bool MinizXellCompression::Compress(
    const std::vector<std::uint8_t>& input, std::uint8_t* output,
    std::size_t* output_capacity) {
  return DeflateRawDeflateBuffer(output, output_capacity, input.data(),
                                 input.size()) == MZ_OK;
}

}  // namespace bandwagon::bootxell
