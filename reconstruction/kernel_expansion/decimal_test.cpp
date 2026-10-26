#include "decimal.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <limits>

int main() {
  using bandwagon::kernel_expansion::ParseSignedDecimal;

  assert(ParseSignedDecimal("0") == 0);
  assert(ParseSignedDecimal("12345") == 12345);
  assert(ParseSignedDecimal("-42") == -42);
  assert(ParseSignedDecimal("19suffix") == 19);
  assert(ParseSignedDecimal("-7.5") == -7);

  assert(ParseSignedDecimal("") == 0);
  assert(ParseSignedDecimal("-") == 0);
  assert(ParseSignedDecimal("+8") == 0);
  assert(ParseSignedDecimal(" 8") == 0);
  assert(ParseSignedDecimal("x12") == 0);
  assert(ParseSignedDecimal(nullptr) == 0);

  assert(ParseSignedDecimal("4294967295") == -1);
  assert(ParseSignedDecimal("4294967296") == 0);
  assert(ParseSignedDecimal("-2147483648") ==
         std::numeric_limits<std::int32_t>::min());

  std::cout << "Kernel Expansion decimal parser tests passed\n";
}
