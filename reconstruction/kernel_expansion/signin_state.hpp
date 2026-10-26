#pragma once

#include <array>
#include <cstddef>
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

struct SigninInfo {
  std::array<std::uint8_t, 0x0c> opaque_prefix{};
  std::uint32_t state = 0;
};

static_assert(offsetof(SigninInfo, state) == 0x0c);

class SigninInfoServices {
 public:
  virtual ~SigninInfoServices() = default;
  virtual std::uint32_t GetSigninInfo(std::uint32_t user_index,
                                      std::uint32_t flags,
                                      SigninInfo* info) = 0;
};

// Lift of the XamUserGetSigninState wrapper at 0x801f4d08. A locally signed-in
// user is reported as online; every other raw state is forwarded unchanged.
std::uint32_t PromoteLocalSigninState(SigninStateServices* services,
                                      std::uint32_t user_index);

// Companion lift at 0x801f4ed8. On a successful XAM query, the state field at
// offset 0x0c is promoted from local to online. The XAM status is forwarded.
std::uint32_t PromoteLocalSigninInfo(SigninInfoServices* services,
                                     std::uint32_t user_index,
                                     std::uint32_t flags,
                                     SigninInfo* info);

}  // namespace bandwagon::kernel_expansion
