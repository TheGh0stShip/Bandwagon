#pragma once

#include <cstdint>

namespace bandwagon::kernel_expansion {

inline constexpr std::int32_t kStatusUnsuccessful =
    static_cast<std::int32_t>(0xc0000001u);
inline constexpr std::int32_t kStatusObjectNameInvalid =
    static_cast<std::int32_t>(0xc0000033u);
inline constexpr std::int32_t kStatusObjectNameNotFound =
    static_cast<std::int32_t>(0xc0000034u);
inline constexpr std::int32_t kStatusObjectNameCollision =
    static_cast<std::int32_t>(0xc0000035u);
inline constexpr std::int32_t kStatusObjectPathNotFound =
    static_cast<std::int32_t>(0xc000003au);

inline constexpr std::int32_t kFatResultNoFile = 4;
inline constexpr std::int32_t kFatResultNoPath = 5;
inline constexpr std::int32_t kFatResultInvalidName = 6;
inline constexpr std::int32_t kFatResultExist = 8;

inline constexpr std::uint32_t kSecondaryNameCollision = 4;
inline constexpr std::uint32_t kSecondaryNameFailure = 5;

inline constexpr char kUnsupportedFatResultFormat[] =
    "ExFatMapFFToNTStatus: received unsupported status value %d\n";

class FileSystemResultDiagnostics {
 public:
  virtual ~FileSystemResultDiagnostics() = default;
  virtual void PrintUnsupportedResult(const char* format,
                                      std::int32_t result) = 0;
  virtual void DebugBreak() = 0;
};

// Lift of ExFatMapFFToNTStatus at 0x801f9768. The optional secondary output
// reproduces the original four recognized-result values and remains zero for
// an unsupported result after its diagnostic and breakpoint.
std::int32_t MapFatFsResultToNtStatus(
    std::int32_t result, std::uint32_t* secondary_code,
    FileSystemResultDiagnostics* diagnostics);

}  // namespace bandwagon::kernel_expansion
