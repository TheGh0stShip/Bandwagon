#include "boot_options.hpp"

#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

namespace bandwagon::bootxell {
namespace {

constexpr char kSection[] = "BootOption";
constexpr char kBypassAcknowledgement[] =
    "I Understand This Can Break My Console";
constexpr std::uint8_t kSafeTrailer[16] = {
    'x', 'x', 'x', 'x', 'x', 'x', 'x', 'x',
    'x', 'x', 'x', 'x', 'x', 'x', 'x', 'x'};

std::string Unquote(const char* value) {
  if (!value) return {};
  std::string result(value);
  if (!result.empty() && result.front() == '"') result.erase(result.begin());
  if (!result.empty() && result.back() == '"') result.pop_back();
  return result;
}

}  // namespace

void FinalizeBootIniOptions(BootOptionParserState& state) {
  if (!state.current) return;
  state.options.push_back(std::move(*state.current));
  delete state.current;
  state.current = nullptr;
}

int ParseBootIniOption(void* user, const char* section, const char* name,
                       const char* value) {
  if (!user) return 0;
  auto& state = *static_cast<BootOptionParserState*>(user);

  if (!section && !name && !value) {
    FinalizeBootIniOptions(state);
    return 1;
  }
  if (section && !name && !value) {
    if (std::strcmp(section, kSection) == 0) {
      // The wrapper finalizes each previous record before announcing the next
      // section; tolerate direct callback use without that synthetic event.
      FinalizeBootIniOptions(state);
      state.current = new BootOption();
    }
    return 1;
  }
  if (!section || !name || !value || std::strcmp(section, kSection) != 0 ||
      !state.current) {
    return 1;
  }

  if (std::strcmp(name, "Name") == 0) {
    state.current->name = Unquote(value);
  } else if (std::strcmp(name, "FileName") == 0) {
    state.current->file_name = Unquote(value);
  } else if (std::strcmp(name, "BaseAddress") == 0) {
    errno = 0;
    char* end = nullptr;
    const unsigned long long parsed = std::strtoull(value, &end, 16);
    if (errno == 0 && end != value) state.current->base_address = parsed;
  } else if (std::strcmp(name, "BypassXellScanning") == 0) {
    state.current->bypass_xell_scanning =
        Unquote(value) == kBypassAcknowledgement;
  }
  return 1;
}

bool HasSafeXellTrailer(const std::uint8_t* image, std::size_t size) {
  return image && size >= sizeof(kSafeTrailer) &&
         std::equal(std::begin(kSafeTrailer), std::end(kSafeTrailer),
                    image + size - sizeof(kSafeTrailer));
}

bool IsLaunchImageAllowed(const BootOption& option, const std::uint8_t* image,
                          std::size_t size) {
  return option.bypass_xell_scanning || HasSafeXellTrailer(image, size);
}

}  // namespace bandwagon::bootxell
