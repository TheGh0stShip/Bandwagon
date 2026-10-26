#include "path_fields.hpp"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <string>
#include <string_view>

int main() {
  using namespace bandwagon::kernel_expansion;

  PathFieldBlock fields;
  fields.opaque_prefix.fill(0xa5);
  fields.path.fill(static_cast<char>(0xcc));
  fields.parent.fill(static_cast<char>(0xdd));
  CopyPathAndParentDirectory(&fields, "PPROOT:\\System\\UpdateData.zip", 1);
  assert(std::string_view(fields.path.data()) ==
         "PPROOT:\\System\\UpdateData.zip");
  assert(std::string_view(fields.parent.data()) == "PPROOT:\\System");
  assert(std::all_of(fields.opaque_prefix.begin(), fields.opaque_prefix.end(),
                     [](std::uint8_t value) { return value == 0xa5; }));

  fields.parent.fill(static_cast<char>(0xdd));
  CopyPathAndParentDirectory(&fields, "PPROOT:\\file.bin", 0);
  assert(std::string_view(fields.path.data()) == "PPROOT:\\file.bin");
  assert(fields.parent[0] == '\0');
  assert(fields.parent[1] == static_cast<char>(0xdd));

  fields.parent.fill(static_cast<char>(0xdd));
  CopyPathAndParentDirectory(&fields, "PPROOT:\\file.bin", 2);
  assert(fields.parent[0] == '\0');

  fields.parent.fill(static_cast<char>(0xdd));
  CopyPathAndParentDirectory(&fields, "\\rooted", 1);
  assert(fields.parent[0] == '\0');

  std::string long_path(kPathFieldCapacity + 32, 'x');
  fields.path.fill(0);
  fields.parent.fill(static_cast<char>(0xdd));
  CopyPathAndParentDirectory(&fields, long_path.c_str(), 1);
  assert(std::all_of(fields.path.begin(), fields.path.end(),
                     [](char value) { return value == 'x'; }));
  assert(fields.parent[0] == '\0');

  const auto unchanged = fields;
  CopyPathAndParentDirectory(nullptr, "ignored", 1);
  CopyPathAndParentDirectory(&fields, nullptr, 1);
  assert(fields.path == unchanged.path && fields.parent == unchanged.parent);

  std::cout << "Kernel Expansion path-field tests passed\n";
}
