#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace bandwagon::stage4 {

struct PatchRecord {
  std::size_t header_offset;
  std::uint32_t raw_target;
  std::uint64_t effective_target;
  std::vector<std::uint8_t> payload;
};

std::uint64_t ExpandHypervisorAddress(std::uint32_t raw_address);

// Decodes records of big-endian target, big-endian size, and exact payload,
// terminated by 0xffffffff. The sentinel must occupy the final four bytes.
bool ParsePatchStream(const std::uint8_t* data, std::size_t size,
                      std::vector<PatchRecord>* output);

}  // namespace bandwagon::stage4
