#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace bandwagon::bootxell {

struct BootOption {
  std::string name;
  std::string file_name;
  std::uint64_t base_address = 0;
  bool bypass_xell_scanning = false;
};

struct BootOptionParserState {
  std::vector<BootOption> options;
  BootOption* current = nullptr;
};

struct RbtrLaunchRequest {
  std::uint32_t magic;
  std::uint32_t command;
  std::uint64_t base_address;
  std::uint64_t physical_alias;
  std::uint32_t word_count;
};

// Callback shape used by inih. A null section/name/value tuple finalizes the
// current record, matching the closed program's parser wrapper.
int ParseBootIniOption(void* user, const char* section, const char* name,
                       const char* value);
void FinalizeBootIniOptions(BootOptionParserState& state);

bool HasSafeXellTrailer(const std::uint8_t* image, std::size_t size);
bool IsLaunchImageAllowed(const BootOption& option, const std::uint8_t* image,
                          std::size_t size);

std::string BuildOtherOsPath(const BootOption& option);
RbtrLaunchRequest BuildRbtrLaunchRequest(const BootOption& option,
                                         std::uint32_t physical_address,
                                         std::uint32_t file_size);

}  // namespace bandwagon::bootxell
