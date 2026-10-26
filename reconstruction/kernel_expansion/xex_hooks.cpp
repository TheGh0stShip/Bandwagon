#include "xex_hooks.hpp"

#include <array>
#include <cstring>
#include <limits>

namespace bandwagon::kernel_expansion {
namespace {

constexpr std::array<std::uint8_t, 8> kXexDataName{{
    '.', 'x', 'e', 'd', 'a', 't', 'a', 0,
}};

std::uint16_t ReadLittleEndian16(const std::uint8_t* value) {
  return static_cast<std::uint16_t>(value[0]) |
         static_cast<std::uint16_t>(value[1] << 8);
}

std::uint32_t ReadLittleEndian32(const std::uint8_t* value) {
  return static_cast<std::uint32_t>(value[0]) |
         (static_cast<std::uint32_t>(value[1]) << 8) |
         (static_cast<std::uint32_t>(value[2]) << 16) |
         (static_cast<std::uint32_t>(value[3]) << 24);
}

std::uint32_t ReadBigEndian32(const std::uint8_t* value) {
  return (static_cast<std::uint32_t>(value[0]) << 24) |
         (static_cast<std::uint32_t>(value[1]) << 16) |
         (static_cast<std::uint32_t>(value[2]) << 8) |
         static_cast<std::uint32_t>(value[3]);
}

void StoreBigEndian32(std::uint8_t* output, std::uint32_t value) {
  output[0] = static_cast<std::uint8_t>(value >> 24);
  output[1] = static_cast<std::uint8_t>(value >> 16);
  output[2] = static_cast<std::uint8_t>(value >> 8);
  output[3] = static_cast<std::uint8_t>(value);
}

bool Contains(std::size_t image_size, std::size_t offset, std::size_t size) {
  return offset <= image_size && size <= image_size - offset;
}

}  // namespace

bool InstallKernelXexDataHandlers(std::uint8_t* mapped_image,
                                  std::size_t image_size,
                                  XexHookServices* services,
                                  std::uint32_t first_handler,
                                  std::uint32_t second_handler) {
  if (!mapped_image || !services) return false;

  std::size_t nt_offset = 0;
  if (!services->FindNtHeader(mapped_image, image_size, &nt_offset) ||
      !Contains(image_size, nt_offset, 0x18)) {
    return false;
  }

  const std::uint16_t section_count =
      ReadLittleEndian16(mapped_image + nt_offset + 6);
  const std::uint16_t optional_header_size =
      ReadLittleEndian16(mapped_image + nt_offset + 0x14);
  const std::size_t section_table = nt_offset + 0x18 + optional_header_size;
  if (section_table < nt_offset) return false;

  constexpr std::size_t kSectionHeaderSize = 0x28;
  for (std::uint16_t index = 0; index < section_count; ++index) {
    if (index > (std::numeric_limits<std::size_t>::max() - section_table) /
                    kSectionHeaderSize) {
      return false;
    }
    const std::size_t section =
        section_table + static_cast<std::size_t>(index) * kSectionHeaderSize;
    if (!Contains(image_size, section, kSectionHeaderSize)) return false;
    if (std::memcmp(mapped_image + section, kXexDataName.data(),
                    kXexDataName.size()) != 0) {
      continue;
    }

    const std::uint32_t virtual_address =
        ReadLittleEndian32(mapped_image + section + 0x0c);
    const std::uint32_t section_size =
        ReadLittleEndian32(mapped_image + section + 0x10);
    const std::size_t xex_data = virtual_address;
    if (!Contains(image_size, xex_data, 0x2c) ||
        section_size == std::numeric_limits<std::uint32_t>::max()) {
      return false;
    }

    const std::uint32_t relocation_units =
        ReadBigEndian32(mapped_image + xex_data + 0x20);
    const std::uint32_t active_count =
        ReadBigEndian32(mapped_image + xex_data + 0x24);
    const std::uint32_t tail_count =
        ReadBigEndian32(mapped_image + xex_data + 0x28);
    if (active_count + tail_count != 0x3a4 || tail_count > 0x30c)
      return false;

    const std::uint64_t first_slot =
        static_cast<std::uint64_t>(xex_data) + 0x2c +
        static_cast<std::uint64_t>(active_count) * 4;
    const std::uint64_t second_slot =
        static_cast<std::uint64_t>(xex_data) + 0x2c +
        static_cast<std::uint64_t>(0x30c - tail_count) * 4;
    if (first_slot > std::numeric_limits<std::size_t>::max() ||
        second_slot > std::numeric_limits<std::size_t>::max() ||
        !Contains(image_size, static_cast<std::size_t>(first_slot), 4) ||
        !Contains(image_size, static_cast<std::size_t>(second_slot), 4)) {
      return false;
    }

    const std::size_t flush_size = static_cast<std::size_t>(section_size) + 4;
    if (!Contains(image_size, xex_data, flush_size)) return false;

    const std::uint32_t relocation = relocation_units << 16;
    StoreBigEndian32(mapped_image + first_slot, first_handler - relocation);
    StoreBigEndian32(mapped_image + xex_data + 0x24, active_count + 1);
    StoreBigEndian32(mapped_image + second_slot, second_handler - relocation);

    services->FlushCpuCache(xex_data, flush_size);
    const std::uint64_t physical = services->PhysicalAddress(xex_data);
    services->FlushHypervisorDataCache(physical, flush_size);
    return true;
  }
  return false;
}

}  // namespace bandwagon::kernel_expansion
