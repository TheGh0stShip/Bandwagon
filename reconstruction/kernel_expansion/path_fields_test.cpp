#include "path_fields.hpp"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <string>
#include <string_view>

namespace {

class FakePathProbe final
    : public bandwagon::kernel_expansion::PathProbeServices {
 public:
  int open_result = 0;
  bandwagon::kernel_expansion::FileHandle returned_handle = 0x1234;
  bool opened = false;
  bool closed = false;

  int OpenExisting(
      std::string_view path,
      const bandwagon::kernel_expansion::ExistingPathOpenOptions& options,
      bandwagon::kernel_expansion::FileHandle* handle) override {
    assert(path == "PPROOT:\\System\\Config.ini");
    assert(options.desired_access == 0x80100080);
    assert(options.object_attributes == 0x40);
    assert(options.file_attributes == 0x80);
    assert(options.share_access == 1);
    assert(options.disposition == 1);
    assert(options.create_options == 0x60);
    assert(handle);
    opened = true;
    *handle = returned_handle;
    return open_result;
  }

  void Close(bandwagon::kernel_expansion::FileHandle handle) override {
    assert(handle == returned_handle);
    closed = true;
  }
};

}  // namespace

int main() {
  using namespace bandwagon::kernel_expansion;

  FakePathProbe success;
  assert(CanOpenExistingPath(&success, "PPROOT:\\System\\Config.ini"));
  assert(success.opened && success.closed);

  FakePathProbe failure;
  failure.open_result = -1;
  assert(!CanOpenExistingPath(&failure, "PPROOT:\\System\\Config.ini"));
  assert(failure.opened && !failure.closed);
  assert(!CanOpenExistingPath(nullptr, "PPROOT:\\System\\Config.ini"));

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
