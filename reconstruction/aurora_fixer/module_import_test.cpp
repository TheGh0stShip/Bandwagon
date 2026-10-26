#include "module_import.hpp"

#include "absolute_branch.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

namespace {

void Store16(std::vector<std::uint8_t>& bytes, std::size_t at,
             std::uint16_t value) {
  bytes[at] = static_cast<std::uint8_t>(value >> 8);
  bytes[at + 1] = static_cast<std::uint8_t>(value);
}

void Store32(std::vector<std::uint8_t>& bytes, std::size_t at,
             std::uint32_t value) {
  bytes[at] = static_cast<std::uint8_t>(value >> 24);
  bytes[at + 1] = static_cast<std::uint8_t>(value >> 16);
  bytes[at + 2] = static_cast<std::uint8_t>(value >> 8);
  bytes[at + 3] = static_cast<std::uint8_t>(value);
}

std::vector<std::uint8_t> MakeDescriptor() {
  const char name[] = "xboxkrnl.exe\0";
  constexpr std::size_t names_size = sizeof(name) - 1;
  constexpr std::size_t record_size = 48;
  std::vector<std::uint8_t> descriptor(12 + names_size + record_size, 0);
  Store32(descriptor, 0, static_cast<std::uint32_t>(descriptor.size()));
  Store32(descriptor, 4, names_size);
  Store32(descriptor, 8, 1);
  std::copy_n(name, names_size, descriptor.begin() + 12);
  const std::size_t record = 12 + names_size;
  Store32(descriptor, record, record_size);
  Store16(descriptor, record + 36, 0);
  Store16(descriptor, record + 38, 2);
  Store32(descriptor, record + 40, 0x82000100u);
  Store32(descriptor, record + 44, 0x82000110u);
  return descriptor;
}

}  // namespace

int main() {
  constexpr std::uint32_t image_base = 0x82000000u;
  constexpr std::uint32_t resolved = 0x80018000u;
  constexpr std::uint32_t replacement = 0x880102f8u;
  std::vector<std::uint8_t> image(0x200, 0);
  Store32(image, 0x100, resolved);
  const auto original_branch =
      bandwagon::aurora_fixer::EncodeAbsoluteBranch(resolved, true);
  std::copy(original_branch.begin(), original_branch.end(),
            image.begin() + 0x110);
  const auto descriptor = MakeDescriptor();

  const auto result = bandwagon::aurora_fixer::PatchModuleImportOffline(
      descriptor.data(), descriptor.size(), "XBOXKRNL.EXE", resolved,
      replacement, image_base, image.data(), image.size());
  assert(result.patched());
  assert(result.direct_pointers == 1);
  assert(result.branch_stubs == 1);
  assert((image[0x100] == 0x88 && image[0x101] == 0x01 &&
          image[0x102] == 0x02 && image[0x103] == 0xf8));
  const auto replacement_branch =
      bandwagon::aurora_fixer::EncodeAbsoluteBranch(replacement, false);
  assert(std::equal(replacement_branch.begin(), replacement_branch.end(),
                    image.begin() + 0x110));

  const auto missing = bandwagon::aurora_fixer::PatchModuleImportOffline(
      descriptor.data(), descriptor.size(), "xbdm.xex", resolved, replacement,
      image_base, image.data(), image.size());
  assert(!missing.patched());

  std::cout << "AuroraFixer module-import reconstruction tests passed\n";
}
