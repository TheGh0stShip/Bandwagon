#include "maintenance_helpers.hpp"

#include <utility>

namespace bandwagon::stage4 {
namespace {

std::uint32_t LoadBigEndian32(const std::uint8_t* input) {
  return (static_cast<std::uint32_t>(input[0]) << 24) |
         (static_cast<std::uint32_t>(input[1]) << 16) |
         (static_cast<std::uint32_t>(input[2]) << 8) | input[3];
}

}  // namespace

bool ParseAddressValueTable(const std::uint8_t* data, std::size_t size,
                            AddressValuePlan* output) {
  if (!data || !output) return false;
  AddressValuePlan parsed;
  std::size_t cursor = 0;
  while (true) {
    if (size - cursor < 4) return false;
    const std::uint32_t address = LoadBigEndian32(data + cursor);
    if (address == 0xffffffffu) {
      parsed.bytes_consumed = cursor + 4;
      *output = std::move(parsed);
      return true;
    }
    if (size - cursor < 8) return false;
    const std::uint32_t value = LoadBigEndian32(data + cursor + 4);
    parsed.actions.push_back(AddressValueAction{
        address, value,
        static_cast<std::uint16_t>(2u * ((value >> 6) & 0x3fffu)), 0x0800u,
        address});
    cursor += 8;
  }
}

MappingRangePlan PlanMappingRange(std::uint64_t count_register) {
  MappingRangePlan plan;
  const std::int32_t count =
      static_cast<std::int32_t>(static_cast<std::uint32_t>(count_register));
  if (count > 1) plan.iterations = static_cast<std::uint32_t>(count);
  return plan;
}

}  // namespace bandwagon::stage4
