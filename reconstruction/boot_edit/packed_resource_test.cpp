#include "packed_resource.hpp"

#include <cassert>
#include <iostream>

namespace pr = bandwagon::boot_edit::packed_resource;

namespace {
void WriteBe16(std::vector<std::uint8_t>* bytes, std::size_t offset,
               std::uint16_t value) {
  (*bytes)[offset] = static_cast<std::uint8_t>(value >> 8);
  (*bytes)[offset + 1] = static_cast<std::uint8_t>(value);
}
void WriteBe32(std::vector<std::uint8_t>* bytes, std::size_t offset,
               std::uint32_t value) {
  (*bytes)[offset] = static_cast<std::uint8_t>(value >> 24);
  (*bytes)[offset + 1] = static_cast<std::uint8_t>(value >> 16);
  (*bytes)[offset + 2] = static_cast<std::uint8_t>(value >> 8);
  (*bytes)[offset + 3] = static_cast<std::uint8_t>(value);
}

std::vector<std::uint8_t> BuildArchive() {
  std::vector<std::uint8_t> file(160, 0);
  WriteBe32(&file, 0, pr::kXpr2Magic);
  WriteBe32(&file, 4, 116);  // 128-byte metadata including the 12-byte header.
  WriteBe32(&file, 8, 32);
  WriteBe32(&file, 12, 1);
  WriteBe16(&file, 16, pr::kTextureRecord);
  WriteBe32(&file, 20, 36);   // object at metadata + 12 + 36 = 48
  WriteBe32(&file, 24, 64);
  WriteBe32(&file, 28, 100);  // name at 112
  WriteBe32(&file, 48 + 44, 4);
  WriteBe32(&file, 48 + 60, 8);
  const char name[] = "Logo";
  for (std::size_t i = 0; i < sizeof(name); ++i) file[112 + i] = name[i];
  for (std::size_t i = 128; i < file.size(); ++i)
    file[i] = static_cast<std::uint8_t>(i);
  return file;
}
}  // namespace

int main() {
  const auto loaded = pr::LoadPackedResource(BuildArchive());
  assert(loaded);
  assert(loaded.resource.loaded && loaded.resource.metadata_size == 128);
  assert(loaded.resource.physical_data.size() == 32);
  assert(loaded.resource.records.size() == 1);
  const auto& record = loaded.resource.records[0];
  assert(record.type == pr::kTextureRecord && record.name == "Logo");
  assert(record.texture_data_offset_0 == 4 && record.texture_data_offset_1 == 8);
  assert(pr::FindResource(loaded.resource, "logo") == &record);
  assert(pr::ResourceSize(loaded.resource, "LOGO") == 64);
  std::size_t available = 0;
  const auto* data = pr::ResourceData(loaded.resource, "Logo", &available);
  assert(data == loaded.resource.metadata.data() + 48 && available == 80);
  assert(pr::FindResource(loaded.resource, "missing") == nullptr);
  assert(pr::ResourceSize(loaded.resource, "missing") == 0);

  auto wrong_magic = BuildArchive();
  wrong_magic[0] = 0;
  assert(pr::LoadPackedResource(wrong_magic).error == pr::LoadError::kWrongMagic);
  auto bad_table = BuildArchive();
  WriteBe32(&bad_table, 12, 100);
  assert(pr::LoadPackedResource(bad_table).error == pr::LoadError::kInvalidRecordTable);
  auto bad_name = BuildArchive();
  WriteBe32(&bad_name, 28, 1000);
  assert(pr::LoadPackedResource(bad_name).error == pr::LoadError::kInvalidName);
  auto bad_texture = BuildArchive();
  WriteBe32(&bad_texture, 48 + 44, 1000);
  assert(pr::LoadPackedResource(bad_texture).error == pr::LoadError::kInvalidTextureRecord);

  std::cout << "BootEdit packed-resource reconstruction tests passed\n";
}
