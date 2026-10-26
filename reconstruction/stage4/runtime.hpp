#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace bandwagon::stage4 {

constexpr std::uint64_t kCleanHvDestination = 0x8000010600030000ULL;
constexpr std::uint32_t kCleanHvCacheLineCount = 0x200;

class Platform {
 public:
  virtual ~Platform() = default;

  virtual void WriteSmcMmioWord(std::uint64_t address,
                                std::uint32_t value) = 0;
  virtual void SmcIoBarrier() = 0;
  virtual void RelocateCacheLines(const std::uint8_t* source,
                                  std::uint64_t destination,
                                  std::uint32_t count) = 0;
  virtual std::uint32_t ReadMappedWord(std::uint64_t address) = 0;
  virtual void WriteMappedWord(std::uint64_t address,
                               std::uint32_t value) = 0;
  virtual std::uint64_t GetFlagTableBase() = 0;
  virtual void WriteMappedHalfword(std::uint64_t address,
                                   std::uint16_t value) = 0;
  virtual void FlushAddress(std::uint64_t address) = 0;
  virtual void WriteBytes(std::uint64_t address, const std::uint8_t* data,
                          std::size_t size) = 0;
};

struct RuntimeInputs {
  std::uint64_t mode_register = 0;
  std::uint64_t r6 = 0;
  std::uint64_t input_r0 = 0;
  const std::uint8_t* clean_hv = nullptr;
  std::size_t clean_hv_size = 0;
  const std::uint8_t* patch_stream = nullptr;
  std::size_t patch_stream_size = 0;
  const std::uint8_t* address_value_table = nullptr;
  std::size_t address_value_table_size = 0;
};

struct RuntimeResult {
  bool succeeded = false;
  std::uint32_t return_value = 0;
};

// Readable source-level implementation of the resident Stage4 entry and its
// three modes. Platform isolates privileged hardware access for target builds
// and permits host-side verification. Inputs are validated before side effects;
// malformed embedded data therefore fails cleanly instead of reading past it.
RuntimeResult RunStage4(const RuntimeInputs& inputs, Platform* platform);

}  // namespace bandwagon::stage4
