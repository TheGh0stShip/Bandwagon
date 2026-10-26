#pragma once

#include <cstdint>

namespace bandwagon::kernel_expansion {

inline constexpr std::uint32_t kMappedCallbackValue = 0x000d0000;
inline constexpr std::uint32_t kUnrecognizedPackageStatus = 0x80070570;

inline constexpr std::uint32_t kStfsConMagic = 0x434f4e20;
inline constexpr std::uint32_t kStfsLiveMagic = 0x4c495645;
inline constexpr std::uint32_t kStfsPirsMagic = 0x50495253;

enum class StfsPackageType : std::uint8_t {
  kCon = 1,
  kLive = 3,
  kPirs = 4,
};

// Lifts of three indirectly reached leaf callbacks at 0x801f8210,
// 0x801f8238, and 0x801f8270. The first two routines write 32-bit callback
// outputs; the STFS classifier writes a single-byte package type.
std::uint32_t InitializeCallbackOutputs(std::uint32_t* primary,
                                        std::uint32_t* auxiliary);

std::uint32_t MapCallbackValue(std::uint32_t value,
                               std::uint32_t* mapped,
                               std::uint32_t* auxiliary);

std::uint32_t ClassifyStfsPackageMagic(std::uint32_t magic,
                                       std::uint8_t* package_type);

}  // namespace bandwagon::kernel_expansion
