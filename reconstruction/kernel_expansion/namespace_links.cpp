#include "namespace_links.hpp"

namespace bandwagon::kernel_expansion {

bool DeleteObjectManagerSymbolicLink(SymbolicLinkServices* services,
                                     std::string_view link_name) {
  return services && services->Delete(link_name) == 0;
}

bool CreateObjectManagerSymbolicLink(SymbolicLinkServices* services,
                                     std::string_view link_name,
                                     std::string_view target_name) {
  return services && services->Create(link_name, target_name) == 0;
}

bool CreatePeerPressureRootSymbolicLink(SymbolicLinkServices* services) {
  return CreateObjectManagerSymbolicLink(
      services, kPeerPressureRootLink, kPeerPressureRootTarget);
}

}  // namespace bandwagon::kernel_expansion
