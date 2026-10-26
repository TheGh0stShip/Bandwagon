#include "trigger_payload.hpp"

#include <array>
#include <cassert>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

int main(int argc, char** argv) {
  using namespace bandwagon::hdd_security;
  assert(argc == 2);
  std::ifstream input(argv[1], std::ios::binary);
  assert(input);
  const std::vector<std::uint8_t> expected{
      std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
  const auto built = BuildTriggerPayload();
  assert(expected.size() == built.size());
  assert(std::equal(built.begin(), built.end(), expected.begin()));
  assert(kRetail17559Trigger.stack_pivot_entry == 0x800d27a0);
  assert(kRetail17559Trigger.marker == 0x58425050);
  assert(built[20] == 'X' && built[21] == 'B' && built[22] == 'P' &&
         built[23] == 'P');
  std::cout << "HDD security trigger typed reconstruction tests passed\n";
}
