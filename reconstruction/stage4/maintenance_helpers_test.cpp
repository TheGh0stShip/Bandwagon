#include "maintenance_helpers.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

namespace {

void Append32(std::vector<std::uint8_t>* bytes, std::uint32_t value) {
  bytes->push_back(static_cast<std::uint8_t>(value >> 24));
  bytes->push_back(static_cast<std::uint8_t>(value >> 16));
  bytes->push_back(static_cast<std::uint8_t>(value >> 8));
  bytes->push_back(static_cast<std::uint8_t>(value));
}

}  // namespace

int main() {
  using bandwagon::stage4::AddressValuePlan;
  using bandwagon::stage4::ParseAddressValueTable;
  using bandwagon::stage4::PlanMappingRange;

  std::vector<std::uint8_t> table;
  Append32(&table, 0x80040000u);
  Append32(&table, 0x12345678u);
  Append32(&table, 0x80050000u);
  Append32(&table, 0x000fffffu);
  Append32(&table, 0xffffffffu);
  table.push_back(0xaa);  // The resident helper stops at the sentinel.

  AddressValuePlan plan;
  assert(ParseAddressValueTable(table.data(), table.size(), &plan));
  assert(plan.bytes_consumed == 20);
  assert(plan.actions.size() == 2);
  assert(plan.actions[0].mapped_address == 0x80040000u);
  assert(plan.actions[0].mapped_value == 0x12345678u);
  assert(plan.actions[0].flag_table_byte_offset ==
         2u * ((0x12345678u >> 6) & 0x3fffu));
  assert(plan.actions[0].flag_value == 0x0800u);
  assert(plan.actions[0].flush_address == 0x80040000u);
  assert(plan.actions[1].flag_table_byte_offset == 0x7ffeu);

  const auto before = plan.actions;
  table.resize(10);
  assert(!ParseAddressValueTable(table.data(), table.size(), &plan));
  assert(plan.actions.size() == before.size());
  assert(!ParseAddressValueTable(nullptr, 0, &plan));

  const auto zero = PlanMappingRange(0);
  assert(zero.first_address == 0x80040000u);
  assert(zero.address_stride == 0x10000u);
  assert(zero.iterations == 1);
  assert(zero.cleared_bit_mask == 7);
  assert(PlanMappingRange(1).iterations == 1);
  assert(PlanMappingRange(4).iterations == 4);
  assert(PlanMappingRange(0x100000002ULL).iterations == 2);
  assert(PlanMappingRange(0x7fffffffu).iterations == 0x7fffffffu);
  assert(PlanMappingRange(0x80000000u).iterations == 1);
  assert(PlanMappingRange(0xffffffffu).iterations == 1);

  std::cout << "Stage4 maintenance-helper reconstruction tests passed\n";
}
