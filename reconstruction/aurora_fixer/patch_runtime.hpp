#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace bandwagon::aurora_fixer {

class PatchRuntimePlatform {
 public:
  virtual ~PatchRuntimePlatform() = default;
  virtual void ReadMemory(std::uint32_t address, std::uint8_t* output,
                          std::size_t size) = 0;
  virtual void WriteMemory(std::uint32_t address, const std::uint8_t* data,
                           std::size_t size) = 0;
  virtual std::uint32_t MmGetPhysicalAddress(std::uint32_t address) = 0;
  virtual void KeFlushCacheRange(std::uint32_t address,
                                 std::uint32_t size) = 0;
  virtual void HvxFlushDcacheRange(std::uint32_t address,
                                   std::uint32_t size) = 0;
};

// Source-level lift of AuroraFixer's PatchAbsoluteBranch. If saved_bytes is
// non-null, the replaced 16 bytes are copied there before mutation.
void PatchAbsoluteBranch(std::uint32_t address, std::uint32_t destination,
                         bool link, std::array<std::uint8_t, 16>* saved_bytes,
                         PatchRuntimePlatform* platform);

}  // namespace bandwagon::aurora_fixer
