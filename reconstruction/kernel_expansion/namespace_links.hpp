#pragma once

#include <string_view>

namespace bandwagon::kernel_expansion {

class SymbolicLinkServices {
 public:
  virtual ~SymbolicLinkServices() = default;
  virtual int Delete(std::string_view link_name) = 0;
  virtual int Create(std::string_view link_name,
                     std::string_view target_name) = 0;
};

// Lifts of the object-manager symbolic-link wrappers at 0x801f0480 and
// 0x801f05f0. Both convert a zero kernel status to true.
bool DeleteObjectManagerSymbolicLink(SymbolicLinkServices* services,
                                     std::string_view link_name);
bool CreateObjectManagerSymbolicLink(SymbolicLinkServices* services,
                                     std::string_view link_name,
                                     std::string_view target_name);

}  // namespace bandwagon::kernel_expansion
