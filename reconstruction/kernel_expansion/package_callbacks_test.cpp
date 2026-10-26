#include "package_callbacks.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>

int main() {
  using namespace bandwagon::kernel_expansion;

  std::uint32_t primary = 0;
  std::uint32_t auxiliary = 9;
  assert(InitializeCallbackOutputs(&primary, &auxiliary) == 0);
  assert(primary == 1 && auxiliary == 0);
  assert(InitializeCallbackOutputs(nullptr, nullptr) == 0);

  std::uint32_t mapped = 0;
  auxiliary = 9;
  assert(MapCallbackValue(kMappedCallbackValue, &mapped, &auxiliary) == 0);
  assert(mapped == 1 && auxiliary == 0);

  mapped = 0;
  assert(MapCallbackValue(kMappedCallbackValue - 1, &mapped, nullptr) == 0);
  assert(mapped == 0xffffffff);
  assert(MapCallbackValue(kMappedCallbackValue, nullptr, nullptr) == 0);

  std::uint8_t package_type = 0;
  assert(ClassifyStfsPackageMagic(kStfsConMagic, &package_type) == 0);
  assert(package_type == static_cast<std::uint8_t>(StfsPackageType::kCon));
  assert(ClassifyStfsPackageMagic(kStfsLiveMagic, &package_type) == 0);
  assert(package_type == static_cast<std::uint8_t>(StfsPackageType::kLive));
  assert(ClassifyStfsPackageMagic(kStfsPirsMagic, &package_type) == 0);
  assert(package_type == static_cast<std::uint8_t>(StfsPackageType::kPirs));

  package_type = 0xa5;
  assert(ClassifyStfsPackageMagic(0x12345678, &package_type) ==
         kUnrecognizedPackageStatus);
  assert(package_type == 0xa5);
  assert(ClassifyStfsPackageMagic(0x12345678, nullptr) == 0);

  std::cout << "Kernel Expansion package callback tests passed\n";
}
