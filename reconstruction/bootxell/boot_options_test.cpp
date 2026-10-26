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
  std::cout << "BootXell option reconstruction tests passed\n";
}
