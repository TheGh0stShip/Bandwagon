#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace bandwagon::aurora_fixer {

struct ImportLibrary {
  std::string name;
  std::vector<std::uint32_t> stub_addresses;
};

// Decodes the XEX import-libraries descriptor consumed by
// PatchModuleImportByOrdinal. Numeric fields are big-endian. The output is
// replaced only on success.
bool ParseImportDescriptor(const std::uint8_t* data, std::size_t size,
                           std::vector<ImportLibrary>* output);

// Returns the first case-insensitive library match, as in the closed routine.
const ImportLibrary* FindImportLibrary(
    const std::vector<ImportLibrary>& libraries, std::string_view name);

}  // namespace bandwagon::aurora_fixer
