#include "softmod_manifest.hpp"

#include <algorithm>
#include <stdexcept>

namespace bandwagon::updater {
namespace {

constexpr std::size_t kHeaderSize = 0x30;
constexpr std::size_t kHashOffset = 0x08;
constexpr std::size_t kHashCoverageOffset = 0x1c;

std::uint32_t Load32(const std::uint8_t* p) {
  return (static_cast<std::uint32_t>(p[0]) << 24) |
         (static_cast<std::uint32_t>(p[1]) << 16) |
         (static_cast<std::uint32_t>(p[2]) << 8) | p[3];
}

std::uint16_t Load16(const std::uint8_t* p) {
  return static_cast<std::uint16_t>((p[0] << 8) | p[1]);
}

void Store32(std::vector<std::uint8_t>& out, std::size_t at,
             std::uint32_t value) {
  out[at] = static_cast<std::uint8_t>(value >> 24);
  out[at + 1] = static_cast<std::uint8_t>(value >> 16);
  out[at + 2] = static_cast<std::uint8_t>(value >> 8);
  out[at + 3] = static_cast<std::uint8_t>(value);
}

void Store16(std::vector<std::uint8_t>& out, std::size_t at,
             std::uint16_t value) {
  out[at] = static_cast<std::uint8_t>(value >> 8);
  out[at + 1] = static_cast<std::uint8_t>(value);
}

void Require(bool condition, const char* message) {
  if (!condition) throw std::runtime_error(message);
}

}  // namespace

SoftmodManifest ParseSoftmodManifest(const std::uint8_t* data,
                                     std::size_t size, Sha1 sha1) {
  Require(data && sha1 && size >= kHeaderSize, "invalid manifest input");
  Require(std::equal(data, data + 4, "XBPP"), "invalid manifest magic");
  Require(Load32(data + 4) == size, "manifest size mismatch");
  const Sha1Digest calculated = sha1(data + kHashCoverageOffset,
                                     size - kHashCoverageOffset);
  Require(std::equal(calculated.begin(), calculated.end(), data + kHashOffset),
          "manifest SHA-1 mismatch");

  SoftmodManifest result;
  result.flags = Load32(data + 0x1c);
  result.smc_major = data[0x20];
  result.smc_minor = data[0x21];
  result.smc_reserved = Load16(data + 0x22);
  result.softmod_major = data[0x24];
  result.softmod_minor = data[0x25];
  result.softmod_revision = Load16(data + 0x26);
  result.entries_offset = Load32(data + 0x28);
  const std::uint32_t count = Load32(data + 0x2c);
  Require(result.entries_offset >= kHeaderSize && result.entries_offset <= size,
          "invalid entries offset");
  Require(std::all_of(data + kHeaderSize, data + result.entries_offset,
                      [](std::uint8_t value) { return value == 0; }),
          "nonzero header padding");

  std::size_t cursor = result.entries_offset;
  result.entries.reserve(count);
  for (std::uint32_t index = 0; index < count; ++index) {
    Require(size - cursor >= 25, "truncated manifest entry");
    SoftmodManifestEntry entry;
    std::copy(data + cursor, data + cursor + 20, entry.sha1.begin());
    entry.flags = Load32(data + cursor + 20);
    const std::size_t padded_path_size = data[cursor + 24];
    cursor += 25;
    Require(padded_path_size && padded_path_size <= size - cursor,
            "invalid manifest path length");
    const std::uint8_t* begin = data + cursor;
    const std::uint8_t* end = begin + padded_path_size;
    const std::uint8_t* nul = std::find(begin, end, 0);
    Require(nul != end, "unterminated manifest path");
    Require(std::all_of(nul, end, [](std::uint8_t value) { return value == 0; }),
            "nonzero manifest path padding");
    Require(std::all_of(begin, nul, [](std::uint8_t value) {
              return value > 0 && value < 0x80;
            }), "manifest path is not ASCII");
    entry.path.assign(reinterpret_cast<const char*>(begin),
                      reinterpret_cast<const char*>(nul));
    result.entries.push_back(std::move(entry));
    cursor += padded_path_size;
  }
  Require(cursor == size, "trailing manifest bytes");
  return result;
}

std::vector<std::uint8_t> BuildSoftmodManifest(const SoftmodManifest& manifest,
                                               Sha1 sha1) {
  Require(sha1 && manifest.entries_offset >= kHeaderSize,
          "invalid manifest build input");
  Require(manifest.entries.size() <= 0xffffffffu,
          "manifest has too many entries");
  std::vector<std::uint8_t> out(manifest.entries_offset, 0);
  std::copy_n("XBPP", 4, out.begin());
  Store32(out, 0x1c, manifest.flags);
  out[0x20] = manifest.smc_major;
  out[0x21] = manifest.smc_minor;
  Store16(out, 0x22, manifest.smc_reserved);
  out[0x24] = manifest.softmod_major;
  out[0x25] = manifest.softmod_minor;
  Store16(out, 0x26, manifest.softmod_revision);
  Store32(out, 0x28, manifest.entries_offset);
  Store32(out, 0x2c, static_cast<std::uint32_t>(manifest.entries.size()));

  for (const auto& entry : manifest.entries) {
    Require(!entry.path.empty() &&
                std::all_of(entry.path.begin(), entry.path.end(),
                            [](unsigned char c) { return c < 0x80; }),
            "invalid manifest path");
    const std::size_t raw_size = entry.path.size() + 1;
    const std::size_t padded_size = raw_size + ((4 - ((25 + raw_size) & 3)) & 3);
    Require(padded_size <= 0xff, "manifest path is too large");
    const std::size_t at = out.size();
    out.resize(at + 25 + padded_size, 0);
    std::copy(entry.sha1.begin(), entry.sha1.end(), out.begin() + at);
    Store32(out, at + 20, entry.flags);
    out[at + 24] = static_cast<std::uint8_t>(padded_size);
    std::copy(entry.path.begin(), entry.path.end(), out.begin() + at + 25);
  }
  Require(out.size() <= 0xffffffffu, "manifest exceeds 32-bit format limits");
  Store32(out, 4, static_cast<std::uint32_t>(out.size()));
  const Sha1Digest digest = sha1(out.data() + kHashCoverageOffset,
                                out.size() - kHashCoverageOffset);
  std::copy(digest.begin(), digest.end(), out.begin() + kHashOffset);
  return out;
}

}  // namespace bandwagon::updater
