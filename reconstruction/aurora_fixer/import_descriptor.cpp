#include "import_descriptor.hpp"

#include <cctype>
#include <limits>

namespace bandwagon::aurora_fixer {
namespace {

std::uint16_t LoadBigEndian16(const std::uint8_t* input) {
  return static_cast<std::uint16_t>(
      (static_cast<std::uint16_t>(input[0]) << 8) | input[1]);
}

std::uint32_t LoadBigEndian32(const std::uint8_t* input) {
  return (static_cast<std::uint32_t>(input[0]) << 24) |
         (static_cast<std::uint32_t>(input[1]) << 16) |
         (static_cast<std::uint32_t>(input[2]) << 8) | input[3];
}

bool EqualIgnoreCase(std::string_view left, std::string_view right) {
  if (left.size() != right.size()) return false;
  for (std::size_t i = 0; i < left.size(); ++i) {
    const auto a = static_cast<unsigned char>(left[i]);
    const auto b = static_cast<unsigned char>(right[i]);
    if (std::tolower(a) != std::tolower(b)) return false;
  }
  return true;
}

bool ReadNames(const std::uint8_t* data, std::size_t size,
               std::vector<std::string>* names) {
  std::size_t offset = 0;
  while (offset < size) {
    std::size_t end = offset;
    while (end < size && data[end] != 0) ++end;
    if (end == size) return false;
    names->emplace_back(reinterpret_cast<const char*>(data + offset),
                        end - offset);
    offset = end + 1;
  }
  return true;
}

}  // namespace

bool ParseImportDescriptor(const std::uint8_t* data, std::size_t size,
                           std::vector<ImportLibrary>* output) {
  if (!data || !output || size < 12) return false;
  const std::uint32_t descriptor_size = LoadBigEndian32(data);
  const std::uint32_t names_size = LoadBigEndian32(data + 4);
  if (descriptor_size < 12 || descriptor_size > size ||
      names_size > descriptor_size - 12) {
    return false;
  }

  std::vector<std::string> names;
  if (!ReadNames(data + 12, names_size, &names)) return false;

  std::vector<ImportLibrary> parsed;
  std::size_t offset = 12 + names_size;
  while (offset < descriptor_size) {
    if (descriptor_size - offset < 40) return false;
    const std::uint32_t record_size = LoadBigEndian32(data + offset);
    const std::uint16_t import_count = LoadBigEndian16(data + offset + 38);
    const std::size_t table_bytes = static_cast<std::size_t>(import_count) * 4;
    if (record_size < 40 || record_size > descriptor_size - offset ||
        table_bytes > record_size - 40) {
      return false;
    }

    // The PPC implementation loads byte +37 rather than the complete
    // big-endian uint16 at +36, so only the low eight index bits participate.
    const std::size_t name_index = data[offset + 37];
    if (name_index >= names.size()) return false;
    ImportLibrary library;
    library.name = names[name_index];
    library.stub_addresses.reserve(import_count);
    for (std::size_t i = 0; i < import_count; ++i) {
      library.stub_addresses.push_back(
          LoadBigEndian32(data + offset + 40 + i * 4));
    }
    parsed.push_back(std::move(library));
    offset += record_size;
  }
  if (offset != descriptor_size) return false;
  *output = std::move(parsed);
  return true;
}

const ImportLibrary* FindImportLibrary(
    const std::vector<ImportLibrary>& libraries, std::string_view name) {
  for (const auto& library : libraries) {
    if (EqualIgnoreCase(library.name, name)) return &library;
  }
  return nullptr;
}

}  // namespace bandwagon::aurora_fixer
