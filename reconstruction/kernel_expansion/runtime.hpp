#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace bandwagon::kernel_expansion {

inline constexpr std::string_view kBootXellPath =
    "PPROOT:\\OtherOS\\BootXell.xex";
inline constexpr std::uint32_t kXexLoadFlags = 8;
inline constexpr std::uint32_t kXexEntryOrdinal = 1;

using ModuleHandle = std::uintptr_t;
using Procedure = std::uintptr_t;

class ModuleServices {
 public:
  virtual ~ModuleServices() = default;

  virtual int LoadImage(std::string_view path, std::uint32_t flags,
                        std::uint32_t minimum_version,
                        ModuleHandle* module) = 0;
  virtual int ResolveProcedure(ModuleHandle module, std::uint32_t ordinal,
                               Procedure* procedure) = 0;
  virtual void InvokeProcedure(Procedure procedure) = 0;
  virtual void UnloadImage(ModuleHandle module) = 0;
};

// Readable lift of LaunchBootXell at 0x801f09d0. The original loads the XEX,
// resolves ordinal one, invokes it when resolution succeeds, and unloads any
// returned module handle on every exit path.
void LaunchBootXell(ModuleServices* services);

// Exact reflected CRC-32 loop recovered from 0x801fa540. Unlike a conventional
// GPT validator, the recovered routine includes the stored checksum field in
// the bytes it hashes; this API deliberately preserves that observed behavior.
std::uint32_t ComputeRecoveredGptChecksum(const std::uint8_t* data,
                                          std::size_t size);

// Safe host lift of the routine at 0x801fa540. It checks the 12-byte
// signature/revision prefix, header-size bounds, recovered checksum relation,
// 0x80-byte partition-entry size, and at most 0x80 partition entries.
bool IsRecognizedGptHeader(const std::uint8_t* data, std::size_t size);

// Direct lift of the local character predicate at 0x801fa640.
bool IsAsciiAlphaNumeric(std::uint8_t value);

}  // namespace bandwagon::kernel_expansion
