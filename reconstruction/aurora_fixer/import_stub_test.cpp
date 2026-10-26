#include "import_stub.hpp"

#include "absolute_branch.hpp"

#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>

using bandwagon::aurora_fixer::ImportPatchKind;

int main() {
  std::array<std::uint8_t, 16> direct = {
      0x80, 0x01, 0x23, 0x40, 0xaa, 0xbb, 0xcc, 0xdd,
      0xee, 0xff, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
  assert(bandwagon::aurora_fixer::PatchImportStub(
             direct.data(), direct.size(), 0x80012340, 0x880102f8) ==
         ImportPatchKind::kDirectPointer);
  assert((direct[0] == 0x88 && direct[1] == 0x01 && direct[2] == 0x02 &&
          direct[3] == 0xf8));
  assert(direct[4] == 0xaa);  // Direct imports replace only one word.

  auto branch = bandwagon::aurora_fixer::EncodeAbsoluteBranch(0x80018000, true);
  assert(bandwagon::aurora_fixer::PatchImportStub(
             branch.data(), branch.size(), 0x80018000, 0x880102f8) ==
         ImportPatchKind::kAbsoluteBranchStub);
  assert(branch ==
         bandwagon::aurora_fixer::EncodeAbsoluteBranch(0x880102f8, false));

  auto unrelated = bandwagon::aurora_fixer::EncodeAbsoluteBranch(0x80123456, false);
  const auto before = unrelated;
  assert(bandwagon::aurora_fixer::PatchImportStub(
             unrelated.data(), unrelated.size(), 0x80018000, 0x880102f8) ==
         ImportPatchKind::kNoMatch);
  assert(unrelated == before);
  std::cout << "AuroraFixer import-stub reconstruction tests passed\n";
}
