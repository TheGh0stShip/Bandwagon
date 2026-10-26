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
constexpr std::uint64_t kDefaultBaseAddress = 0x800000001c040000ULL;
constexpr std::uint64_t kHighHalfAlias = 0x8000000000000000ULL;

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

std::string BuildOtherOsPath(const BootOption& option) {
  return "PPROOT:\\OtherOS\\" + option.file_name;
}

RbtrLaunchRequest BuildRbtrLaunchRequest(const BootOption& option,
                                         std::uint32_t physical_address,
                                         std::uint32_t file_size) {
  // LaunchBootOption accepts a configured base only when it is already in the
  // high half of the 64-bit address space. Its allocation length is rounded
  // to 128 bytes before being converted to a count of 32-bit words.
  const std::uint64_t base = (option.base_address & kHighHalfAlias) != 0
                                 ? option.base_address
                                 : kDefaultBaseAddress;
  const std::uint32_t aligned_size = (file_size + 127u) & ~127u;
  return {
      0x72627472u,  // "rbtr"
      4u,
      base,
      kHighHalfAlias | physical_address,
      aligned_size >> 2,
  };
}

}  // namespace bandwagon::bootxell
