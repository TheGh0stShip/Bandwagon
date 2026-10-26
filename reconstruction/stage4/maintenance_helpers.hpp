#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace bandwagon::stage4 {

struct AddressValueAction {
  std::uint32_t mapped_address;
  std::uint32_t mapped_value;
  std::uint16_t flag_table_byte_offset;
  std::uint16_t flag_value;
  std::uint32_t flush_address;
};

struct AddressValuePlan {
  std::size_t bytes_consumed = 0;
  std::vector<AddressValueAction> actions;
};

// Models the table consumed by the helper at Stage4 file offset 0x174. Each
// action represents the mapped-word store, the 0x0800 halfword-table store,
// and the subsequent address flush. Bytes following the sentinel are ignored,
// matching the resident helper. Output is replaced only on success.
bool ParseAddressValueTable(const std::uint8_t* data, std::size_t size,
                            AddressValuePlan* output);

struct MappingRangePlan {
  std::uint32_t first_address = 0x80040000u;
  std::uint32_t address_stride = 0x00010000u;
  std::uint32_t iterations = 1;
  std::uint32_t cleared_bit_mask = 0x00000007u;
};

// Models the signed, post-tested loop at Stage4 file offset 0x210. Counts
// whose low word is zero or negative still produce exactly one iteration.
MappingRangePlan PlanMappingRange(std::uint64_t count_register);

}  // namespace bandwagon::stage4
