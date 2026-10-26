#include "signin_state.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>

namespace {

class FakeSigninState final
    : public bandwagon::kernel_expansion::SigninStateServices {
 public:
  std::uint32_t state = 0;
  std::uint32_t last_user = 0xffffffff;

  std::uint32_t GetSigninState(std::uint32_t user_index) override {
    last_user = user_index;
    return state;
  }
};

}  // namespace

int main() {
  using namespace bandwagon::kernel_expansion;

  FakeSigninState services;
  services.state = static_cast<std::uint32_t>(UserSigninState::kNotSignedIn);
  assert(PromoteLocalSigninState(&services, 3) == 0);
  assert(services.last_user == 3);

  services.state = static_cast<std::uint32_t>(UserSigninState::kLocal);
  assert(PromoteLocalSigninState(&services, 1) ==
         static_cast<std::uint32_t>(UserSigninState::kOnline));

  services.state = static_cast<std::uint32_t>(UserSigninState::kOnline);
  assert(PromoteLocalSigninState(&services, 2) == 2);

  services.state = 7;
  assert(PromoteLocalSigninState(&services, 0) == 7);
  assert(PromoteLocalSigninState(nullptr, 0) == 0);

  std::cout << "Kernel Expansion sign-in state tests passed\n";
}
