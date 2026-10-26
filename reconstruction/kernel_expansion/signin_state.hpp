#pragma once

#include <cstdint>

namespace bandwagon::kernel_expansion {

enum class UserSigninState : std::uint32_t {
  kNotSignedIn = 0,
  kLocal = 1,
  kOnline = 2,
};

class SigninStateServices {
 public:
  virtual ~SigninStateServices() = default;
  virtual std::uint32_t GetSigninState(std::uint32_t user_index) = 0;
};

// Lift of the XamUserGetSigninState wrapper at 0x801f4d08. A locally signed-in
// user is reported as online; every other raw state is forwarded unchanged.
std::uint32_t PromoteLocalSigninState(SigninStateServices* services,
                                      std::uint32_t user_index);

}  // namespace bandwagon::kernel_expansion
