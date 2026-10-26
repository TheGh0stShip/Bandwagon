#include "version.hpp"

#include <cassert>
#include <iostream>

int main() {
  using namespace bandwagon::kernel_expansion;

  assert(IsVersionAtLeastComponentWise({1, 2, 3, 4}, {1, 2, 3, 4}));
  assert(IsVersionAtLeastComponentWise({2, 3, 4, 5}, {1, 2, 3, 4}));
  assert(!IsVersionAtLeastComponentWise({1, 2, 2, 9}, {1, 2, 3, 4}));

  // The recovered comparison is component-wise rather than lexicographic.
  assert(!IsVersionAtLeastComponentWise({2, 0, 0, 0}, {1, 9, 0, 0}));

  assert(IsVersionAtLeastComponentWise(
      {kVersionWildcard, 0, 3, 4}, {9, 0, 3, 4}));
  assert(IsVersionAtLeastComponentWise(
      {1, 0, 3, 4}, {kVersionWildcard, 0, 3, 4}));
  assert(IsVersionAtLeastComponentWise(
      {kVersionWildcard, kVersionWildcard, kVersionWildcard,
       kVersionWildcard},
      {9, 9, 9, 9}));

  std::cout << "Kernel Expansion version tests passed\n";
}
