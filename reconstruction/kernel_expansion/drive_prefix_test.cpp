#include "drive_prefix.hpp"

#include <cassert>
#include <iostream>

int main() {
  using bandwagon::kernel_expansion::ParseDriveNumberPrefix;

  const char16_t zero_path[] = u"0:\\System\\Config.ini";
  const char16_t* cursor = zero_path;
  assert(ParseDriveNumberPrefix(&cursor) == 0);
  assert(cursor == zero_path + 2);

  const char16_t nine_path[] = u"9:content";
  cursor = nine_path;
  assert(ParseDriveNumberPrefix(&cursor) == 9);
  assert(cursor == nine_path + 2);

  const char16_t no_prefix[] = u"content";
  cursor = no_prefix;
  assert(ParseDriveNumberPrefix(&cursor) == 0);
  assert(cursor == no_prefix);

  const char16_t invalid_digit[] = u"A:content";
  cursor = invalid_digit;
  assert(ParseDriveNumberPrefix(&cursor) == -1);
  assert(cursor == invalid_digit);

  const char16_t too_long[] = u"10:content";
  cursor = too_long;
  assert(ParseDriveNumberPrefix(&cursor) == -1);
  assert(cursor == too_long);

  const char16_t empty_prefix[] = u":content";
  cursor = empty_prefix;
  assert(ParseDriveNumberPrefix(&cursor) == -1);
  assert(cursor == empty_prefix);

  const char16_t control_terminated[] = {u'1', 0x001f, u':', u'x', 0};
  cursor = control_terminated;
  assert(ParseDriveNumberPrefix(&cursor) == 0);
  assert(cursor == control_terminated);

  const char16_t* null_string = nullptr;
  assert(ParseDriveNumberPrefix(&null_string) == -1);
  assert(ParseDriveNumberPrefix(nullptr) == -1);

  std::cout << "Kernel Expansion drive-prefix tests passed\n";
}
