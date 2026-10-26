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
using FileHandle = std::uintptr_t;

inline constexpr std::uint32_t kStatusPending = 0x00000103;

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

class FileServices {
 public:
  virtual ~FileServices() = default;

  virtual int OpenReadOnly(std::string_view path, bool object_manager_path,
                           FileHandle* handle) = 0;
  virtual int QueryFileSize(FileHandle handle, std::uint32_t* size) = 0;
  virtual std::uint8_t* Allocate(std::uint32_t size) = 0;
  virtual int Read(FileHandle handle, std::uint8_t* buffer,
                   std::uint32_t size, std::int32_t* io_status) = 0;
  virtual int WaitForRead(FileHandle handle) = 0;
  virtual void Free(std::uint8_t* buffer) = 0;
  virtual void Close(FileHandle handle) = 0;
};

// Readable lift of ReadFileToAllocatedBuffer at 0x801f14d0. On success the
// returned size includes the appended NUL byte, matching the recovered caller
// contract. On failure the caller's output values are left untouched.
bool ReadFileToAllocatedBuffer(FileServices* services, std::string_view path,
                               std::uint8_t** data,
                               std::uint32_t* allocation_size);

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
