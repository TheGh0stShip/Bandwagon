#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace bandwagon::boot_edit::packed_resource {

inline constexpr std::uint32_t kXpr2Magic = 0x58505232u;
inline constexpr std::uint16_t kTextureRecord = 0x5458u;
inline constexpr std::size_t kHeaderSize = 12;
inline constexpr std::size_t kRecordSize = 16;

struct ResourceRecord {
  std::uint16_t type = 0;
  std::uint32_t object_offset = 0;
  std::uint32_t size = 0;
  std::uint32_t name_offset = 0;
  std::string name;
  std::optional<std::uint32_t> texture_data_offset_0;
  std::optional<std::uint32_t> texture_data_offset_1;
};

struct PackedResource {
  bool loaded = false;
  std::uint32_t metadata_size = 0;
  std::uint32_t physical_data_size = 0;
  std::vector<std::uint8_t> metadata;
  std::vector<std::uint8_t> physical_data;
  std::vector<ResourceRecord> records;
};

enum class LoadError {
  kNone,
  kTruncatedHeader,
  kWrongMagic,
  kInvalidMetadataSize,
  kTruncatedFile,
  kInvalidRecordTable,
  kInvalidName,
  kInvalidObject,
  kInvalidTextureRecord,
};

struct LoadResult {
  PackedResource resource;
  LoadError error = LoadError::kNone;
  explicit operator bool() const { return error == LoadError::kNone; }
};

// Portable reconstruction of BootEdit 0x82052A88. The original reads the
// file through XDK virtual/physical allocators; this boundary accepts the
// complete file bytes while preserving its XPR2 layout and fixups.
LoadResult LoadPackedResource(const std::vector<std::uint8_t>& file);

// BootEdit 0x82052C48 and its two accessors at 0x82052CC8/0x82052CF8.
const ResourceRecord* FindResource(const PackedResource& resource,
                                   std::string_view name);
std::uint32_t ResourceSize(const PackedResource& resource,
                           std::string_view name);
const std::uint8_t* ResourceData(const PackedResource& resource,
                                 std::string_view name,
                                 std::size_t* available = nullptr);

}  // namespace bandwagon::boot_edit::packed_resource
