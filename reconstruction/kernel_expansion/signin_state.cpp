#include "signin_state.hpp"

namespace bandwagon::kernel_expansion {

std::uint32_t PromoteLocalSigninState(SigninStateServices* services,
                                      std::uint32_t user_index) {
  if (!services) return 0;
  const std::uint32_t state = services->GetSigninState(user_index);
  return state == static_cast<std::uint32_t>(UserSigninState::kLocal)
             ? static_cast<std::uint32_t>(UserSigninState::kOnline)
             : state;
}

}  // namespace bandwagon::kernel_expansion
