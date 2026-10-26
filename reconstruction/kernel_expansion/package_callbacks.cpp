#include "package_callbacks.hpp"

namespace bandwagon::kernel_expansion {

std::uint32_t InitializeCallbackOutputs(std::uint32_t* primary,
                                        std::uint32_t* auxiliary) {
  if (primary) *primary = 1;
  if (auxiliary) *auxiliary = 0;
  return 0;
}

std::uint32_t MapCallbackValue(std::uint32_t value,
                               std::uint32_t* mapped,
                               std::uint32_t* auxiliary) {
  if (mapped) *mapped = value == kMappedCallbackValue ? 1 : 0xffffffff;
  if (auxiliary) *auxiliary = 0;
  return 0;
}

std::uint32_t ClassifyStfsPackageMagic(std::uint32_t magic,
                                       std::uint8_t* package_type) {
  if (!package_type) return 0;

  StfsPackageType type;
  if (magic == kStfsConMagic) {
    type = StfsPackageType::kCon;
  } else if (magic == kStfsLiveMagic) {
    type = StfsPackageType::kLive;
  } else if (magic == kStfsPirsMagic) {
    type = StfsPackageType::kPirs;
  } else {
    return kUnrecognizedPackageStatus;
  }

  *package_type = static_cast<std::uint8_t>(type);
  return 0;
}

}  // namespace bandwagon::kernel_expansion
