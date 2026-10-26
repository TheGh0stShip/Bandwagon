#include "version.hpp"

namespace bandwagon::kernel_expansion {

bool IsVersionAtLeastComponentWise(const VersionBytes& candidate,
                                   const VersionBytes& minimum) {
  for (std::size_t i = 0; i < candidate.size(); ++i) {
    if (candidate[i] == kVersionWildcard || minimum[i] == kVersionWildcard) {
      continue;
    }
    if (candidate[i] < minimum[i]) return false;
  }
  return true;
}

}  // namespace bandwagon::kernel_expansion
