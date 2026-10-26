#include "boot_options.hpp"

#include <cassert>
#include <iostream>
#include <vector>

using bandwagon::bootxell::BootOptionParserState;
using bandwagon::bootxell::ParseBootIniOption;

int main() {
  BootOptionParserState state;
  assert(ParseBootIniOption(&state, "BootOption", nullptr, nullptr));
  assert(ParseBootIniOption(&state, "BootOption", "Name", "\"Xell R99\""));
  assert(ParseBootIniOption(&state, "BootOption", "FileName",
                            "\"linux\\xell.bin\""));
  assert(ParseBootIniOption(&state, "BootOption", "BaseAddress",
                            "0x800000001c000000"));
  assert(ParseBootIniOption(&state, "BootOption", "BypassXellScanning",
                            "no"));
  assert(ParseBootIniOption(&state, nullptr, nullptr, nullptr));
  assert(state.options.size() == 1);
  assert(state.options[0].name == "Xell R99");
  assert(state.options[0].file_name == "linux\\xell.bin");
  assert(state.options[0].base_address == 0x800000001c000000ULL);
  assert(!state.options[0].bypass_xell_scanning);

  std::vector<std::uint8_t> safe(32, 0);
  std::fill(safe.end() - 16, safe.end(), 'x');
  assert(bandwagon::bootxell::HasSafeXellTrailer(safe.data(), safe.size()));
  assert(bandwagon::bootxell::IsLaunchImageAllowed(state.options[0],
                                                   safe.data(), safe.size()));
  safe.back() = 0;
  assert(!bandwagon::bootxell::IsLaunchImageAllowed(state.options[0],
                                                    safe.data(), safe.size()));
  state.options[0].bypass_xell_scanning = true;
  assert(bandwagon::bootxell::IsLaunchImageAllowed(state.options[0],
                                                   safe.data(), safe.size()));

  BootOptionParserState acknowledged;
  ParseBootIniOption(&acknowledged, "BootOption", nullptr, nullptr);
  ParseBootIniOption(&acknowledged, "BootOption", "BypassXellScanning",
                     "\"I Understand This Can Break My Console\"");
  ParseBootIniOption(&acknowledged, nullptr, nullptr, nullptr);
  assert(acknowledged.options[0].bypass_xell_scanning);

  assert(bandwagon::bootxell::BuildOtherOsPath(state.options[0]) ==
         "PPROOT:\\OtherOS\\linux\\xell.bin");

  const auto request = bandwagon::bootxell::BuildRbtrLaunchRequest(
      state.options[0], 0x1c100080u, 129u);
  assert(request.magic == 0x72627472u);
  assert(request.command == 4u);
  assert(request.base_address == 0x800000001c000000ULL);
  assert(request.physical_alias == 0x800000001c100080ULL);
  assert(request.word_count == 64u);  // 129 rounds to 256 bytes.

  bandwagon::bootxell::BootOption low_base;
  low_base.base_address = 0x1c000000u;
  const auto defaulted = bandwagon::bootxell::BuildRbtrLaunchRequest(
      low_base, 0x00123400u, 128u);
  assert(defaulted.base_address == 0x800000001c040000ULL);
  assert(defaulted.word_count == 32u);

  bandwagon::bootxell::BootOption second;
  second.name = "Recovery";
  state.options.push_back(second);
  assert(bandwagon::bootxell::FindBootOption(state.options, "Xell R99") ==
         &state.options[0]);
  assert(bandwagon::bootxell::FindBootOption(state.options, "Recovery") ==
         &state.options[1]);
  assert(bandwagon::bootxell::FindBootOption(state.options, "Missing") ==
         nullptr);
  assert(bandwagon::bootxell::FindBootOption(state.options, nullptr) ==
         nullptr);

  std::cout << "BootXell option reconstruction tests passed\n";
}
