#include "oem437.hpp"

#include <cassert>
#include <iostream>

int main() {
  using namespace bandwagon::kernel_expansion;

  assert(DecodeOem437Character('A', 437) == 'A');
  assert(DecodeOem437Character(0x12340041, 0) == 0x12340041);

  assert(DecodeOem437Character(0x80, 437) == 0x00c7);
  assert(DecodeOem437Character(0x81, 437) == 0x00fc);
  assert(DecodeOem437Character(0x9e, 437) == 0x20a7);
  assert(DecodeOem437Character(0xdb, 437) == 0x2588);
  assert(DecodeOem437Character(0xe0, 437) == 0x03b1);
  assert(DecodeOem437Character(0xff, 437) == 0x00a0);

  assert(DecodeOem437Character(0xabcd0080, 0x123401b5) == 0x00c7);
  assert(DecodeOem437Character(0x80, 850) == 0);
  assert(DecodeOem437Character(0x100, 437) == 0);
  assert(DecodeOem437Character(0xffff, 437) == 0);

  std::cout << "Kernel Expansion OEM 437 tests passed\n";
}
