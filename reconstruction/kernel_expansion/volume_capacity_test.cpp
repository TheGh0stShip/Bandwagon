#include "volume_capacity.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>

int main() {
  using namespace bandwagon::kernel_expansion;

  std::uint64_t capacity = 99;
  FatVolumeCapacitySource ready{true, 102, 8, 512};
  assert(QueryFatVolumeCapacity(&ready, &capacity) == 0);
  assert(capacity == 409600);

  FatVolumeCapacitySource unavailable{false, 102, 8, 512};
  capacity = 99;
  assert(QueryFatVolumeCapacity(&unavailable, &capacity) == 0);
  assert(capacity == 0);

  capacity = 99;
  assert(QueryFatVolumeCapacity(nullptr, &capacity) == 0);
  assert(capacity == 0);

  FatVolumeCapacitySource subtract_wrap{true, 0, 1, 1};
  assert(QueryFatVolumeCapacity(&subtract_wrap, &capacity) == 0);
  assert(capacity == 0xfffffffeu);

  FatVolumeCapacitySource multiply_wrap{true, 0x00010002, 0x100, 0x100};
  assert(QueryFatVolumeCapacity(&multiply_wrap, &capacity) == 0);
  assert(capacity == 0);

  assert(QueryFatVolumeCapacity(&ready, nullptr) == 0);

  std::cout << "Kernel Expansion volume capacity tests passed\n";
}
