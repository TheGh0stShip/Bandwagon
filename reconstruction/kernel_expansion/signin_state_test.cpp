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

class FakeSigninInfo final
    : public bandwagon::kernel_expansion::SigninInfoServices {
 public:
  std::uint32_t status = 0;
  std::uint32_t returned_state = 1;

  std::uint32_t GetSigninInfo(
      std::uint32_t user_index, std::uint32_t flags,
      bandwagon::kernel_expansion::SigninInfo* info) override {
    assert(user_index == 2 && flags == 0x40);
    if (info) info->state = returned_state;
    return status;
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

  SigninInfo info;
  FakeSigninInfo info_services;
  assert(PromoteLocalSigninInfo(&info_services, 2, 0x40, &info) == 0);
  assert(info.state == static_cast<std::uint32_t>(UserSigninState::kOnline));

  info_services.returned_state = 0;
  assert(PromoteLocalSigninInfo(&info_services, 2, 0x40, &info) == 0);
  assert(info.state == 0);

  info_services.returned_state = 1;
  info_services.status = 0x80000001;
  assert(PromoteLocalSigninInfo(&info_services, 2, 0x40, &info) ==
         0x80000001);
  assert(info.state == 1);

  info_services.status = 0;
  assert(PromoteLocalSigninInfo(&info_services, 2, 0x40, nullptr) == 0);
  assert(PromoteLocalSigninInfo(nullptr, 2, 0x40, &info) == 0);

  std::cout << "Kernel Expansion sign-in state tests passed\n";
}
