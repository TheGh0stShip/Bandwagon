#include "import_descriptor.hpp"

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

}  // namespace

int main() {
  const char names[] = "xam.xex\0xboxkrnl.exe\0";
  constexpr std::size_t names_size = sizeof(names) - 1;
  constexpr std::size_t record_size = 48;
  std::vector<std::uint8_t> descriptor(12 + names_size + record_size, 0);
  Store32(descriptor, 0, static_cast<std::uint32_t>(descriptor.size()));
  Store32(descriptor, 4, names_size);
  Store32(descriptor, 8, 1);  // Library count is not consulted by the binary.
  for (std::size_t i = 0; i < names_size; ++i) descriptor[12 + i] = names[i];
  const std::size_t record = 12 + names_size;
  Store32(descriptor, record, record_size);
  Store16(descriptor, record + 36, 1);
  Store16(descriptor, record + 38, 2);
  Store32(descriptor, record + 40, 0x82001000u);
  Store32(descriptor, record + 44, 0x82001010u);

  std::vector<bandwagon::aurora_fixer::ImportLibrary> libraries;
  assert(bandwagon::aurora_fixer::ParseImportDescriptor(
      descriptor.data(), descriptor.size(), &libraries));
  assert(libraries.size() == 1);
  assert(libraries[0].name == "xboxkrnl.exe");
  assert((libraries[0].stub_addresses ==
          std::vector<std::uint32_t>{0x82001000u, 0x82001010u}));
  assert(bandwagon::aurora_fixer::FindImportLibrary(libraries,
                                                    "XBOXKRNL.EXE") ==
         &libraries[0]);
  assert(bandwagon::aurora_fixer::FindImportLibrary(libraries, "xbdm.xex") ==
         nullptr);

  auto malformed = descriptor;
  Store32(malformed, record, 44);  // Too short for two import addresses.
  const auto before = libraries;
  assert(!bandwagon::aurora_fixer::ParseImportDescriptor(
      malformed.data(), malformed.size(), &libraries));
  assert(libraries[0].stub_addresses == before[0].stub_addresses);
  assert(!bandwagon::aurora_fixer::ParseImportDescriptor(nullptr, 0,
                                                         &libraries));

  std::cout << "AuroraFixer import-descriptor reconstruction tests passed\n";
}
