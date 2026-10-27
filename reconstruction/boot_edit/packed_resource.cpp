#include "packed_resource.hpp"

#include <algorithm>
#include <cctype>

namespace bandwagon::boot_edit::packed_resource {
namespace {

std::uint16_t ReadBe16(const std::vector<std::uint8_t>& bytes,
                       std::size_t offset) {
  return static_cast<std::uint16_t>((bytes[offset] << 8) | bytes[offset + 1]);
}

std::uint32_t ReadBe32(const std::vector<std::uint8_t>& bytes,
                       std::size_t offset) {
  return (static_cast<std::uint32_t>(bytes[offset]) << 24) |
         (static_cast<std::uint32_t>(bytes[offset + 1]) << 16) |
         (static_cast<std::uint32_t>(bytes[offset + 2]) << 8) |
         static_cast<std::uint32_t>(bytes[offset + 3]);
}

bool AddWithin(std::uint32_t left, std::uint32_t right, std::size_t limit,
               std::size_t* result) {
  const std::uint64_t sum = static_cast<std::uint64_t>(left) + right;
  if (sum > limit) return false;
  *result = static_cast<std::size_t>(sum);
  return true;
}

bool EqualCaseInsensitive(std::string_view left, std::string_view right) {
  if (left.size() != right.size()) return false;
  for (std::size_t index = 0; index < left.size(); ++index) {
    const auto a = static_cast<unsigned char>(left[index]);
    const auto b = static_cast<unsigned char>(right[index]);
    if (std::tolower(a) != std::tolower(b)) return false;
  }
  return true;
}

}  // namespace

LoadResult LoadPackedResource(const std::vector<std::uint8_t>& file) {
  LoadResult result;
  if (file.size() < kHeaderSize) {
    result.error = LoadError::kTruncatedHeader;
    return result;
  }
  if (ReadBe32(file, 0) != kXpr2Magic) {
    result.error = LoadError::kWrongMagic;
    return result;
  }

  const std::uint32_t metadata_payload_size = ReadBe32(file, 4);
  const std::uint32_t physical_size = ReadBe32(file, 8);
  std::size_t metadata_size = 0;
  if (!AddWithin(metadata_payload_size, kHeaderSize, file.size(),
                 &metadata_size) || metadata_size < 16) {
    result.error = LoadError::kInvalidMetadataSize;
    return result;
  }
  std::size_t total_size = 0;
  if (!AddWithin(static_cast<std::uint32_t>(metadata_size), physical_size,
                 file.size(), &total_size) || total_size > file.size()) {
    result.error = LoadError::kTruncatedFile;
    return result;
  }

  auto& resource = result.resource;
  resource.metadata_size = static_cast<std::uint32_t>(metadata_size);
  resource.physical_data_size = physical_size;
  resource.metadata.assign(file.begin(), file.begin() + metadata_size);
  resource.physical_data.assign(file.begin() + metadata_size,
                                file.begin() + total_size);

  const std::uint32_t count = ReadBe32(resource.metadata, 12);
  const std::uint64_t table_end = 16ull + static_cast<std::uint64_t>(count) * kRecordSize;
  if (table_end > resource.metadata.size()) {
    result.error = LoadError::kInvalidRecordTable;
    return result;
  }
  resource.records.reserve(count);
  for (std::uint32_t index = 0; index < count; ++index) {
    const std::size_t offset = 16 + index * kRecordSize;
    ResourceRecord record;
    record.type = ReadBe16(resource.metadata, offset);
    record.object_offset = ReadBe32(resource.metadata, offset + 4);
    record.size = ReadBe32(resource.metadata, offset + 8);
    record.name_offset = ReadBe32(resource.metadata, offset + 12);

    std::size_t name = 0;
    if (!AddWithin(record.name_offset, kHeaderSize, resource.metadata.size(),
                   &name)) {
      result.error = LoadError::kInvalidName;
      return result;
    }
    const auto terminator = std::find(resource.metadata.begin() + name,
                                      resource.metadata.end(), 0);
    if (terminator == resource.metadata.end()) {
      result.error = LoadError::kInvalidName;
      return result;
    }
    record.name.assign(resource.metadata.begin() + name, terminator);

    std::size_t object = 0;
    if (!AddWithin(record.object_offset, kHeaderSize, resource.metadata.size(),
                   &object)) {
      result.error = LoadError::kInvalidObject;
      return result;
    }
    if (record.type == kTextureRecord) {
      if (object + 64 > resource.metadata.size()) {
        result.error = LoadError::kInvalidTextureRecord;
        return result;
      }
      const std::uint32_t first = ReadBe32(resource.metadata, object + 44);
      const std::uint32_t second = ReadBe32(resource.metadata, object + 60);
      if (first > resource.physical_data.size() ||
          second > resource.physical_data.size()) {
        result.error = LoadError::kInvalidTextureRecord;
        return result;
      }
      record.texture_data_offset_0 = first;
      record.texture_data_offset_1 = second;
    }
    resource.records.push_back(std::move(record));
  }
  resource.loaded = true;
  return result;
}

const ResourceRecord* FindResource(const PackedResource& resource,
                                   std::string_view name) {
  for (const auto& record : resource.records)
    if (EqualCaseInsensitive(name, record.name)) return &record;
  return nullptr;
}

std::uint32_t ResourceSize(const PackedResource& resource,
                           std::string_view name) {
  const ResourceRecord* record = FindResource(resource, name);
  return record ? record->size : 0;
}

const std::uint8_t* ResourceData(const PackedResource& resource,
                                 std::string_view name,
                                 std::size_t* available) {
  const ResourceRecord* record = FindResource(resource, name);
  if (!record) return nullptr;
  const std::size_t offset = kHeaderSize + record->object_offset;
  if (offset > resource.metadata.size()) return nullptr;
  if (available) *available = resource.metadata.size() - offset;
  return resource.metadata.data() + offset;
}

}  // namespace bandwagon::boot_edit::packed_resource
