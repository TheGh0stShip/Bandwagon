#include "runtime.hpp"

#include <algorithm>

#include "entry_flow.hpp"
#include "maintenance_helpers.hpp"
#include "patch_stream.hpp"

namespace bandwagon::stage4 {
namespace {

std::uint32_t LoadBigEndian32(const std::uint8_t* input) {
  return (static_cast<std::uint32_t>(input[0]) << 24) |
         (static_cast<std::uint32_t>(input[1]) << 16) |
         (static_cast<std::uint32_t>(input[2]) << 8) | input[3];
}

void StoreBigEndian64(std::uint64_t value, std::uint8_t* output) {
  for (unsigned index = 0; index < 8; ++index) {
    output[index] = static_cast<std::uint8_t>(value >> (56 - 8 * index));
  }
}

RuntimeResult Success() { return {true, kStage4ReturnMarker}; }

}  // namespace

RuntimeResult RunStage4(const RuntimeInputs& inputs, Platform* platform) {
  if (!platform) return {};
  const std::uint32_t mode = static_cast<std::uint32_t>(inputs.mode_register);
  if (mode > 2) return Success();

  if (mode == 0) {
    if (!inputs.clean_hv || inputs.clean_hv_size != 0x10000) return {};
    std::vector<PatchRecord> patches;
    if (!ParsePatchStream(inputs.patch_stream, inputs.patch_stream_size,
                          &patches)) {
      return {};
    }

    std::array<std::uint8_t, 16> message{};
    StoreBigEndian64(inputs.input_r0, message.data());
    StoreBigEndian64(inputs.input_r0, message.data() + 8);
    message[0] = 0x99;
    message[1] = 0xff;
    message[2] = 0xff;
    platform->SendSmcMessage(message);
    platform->RelocateCacheLines(inputs.clean_hv, 0x8000010600030000ULL,
                                 0x200);
    for (const auto& patch : patches) {
      platform->WriteBytes(patch.effective_target, patch.payload.data(),
                           patch.payload.size());
    }
    return Success();
  }

  if (mode == 1) {
    if (!inputs.address_value_table || inputs.address_value_table_size < 4)
      return {};
    AddressValuePlan values;
    if (!ParseAddressValueTable(inputs.address_value_table,
                                inputs.address_value_table_size, &values)) {
      return {};
    }
    const std::uint64_t initial_address =
        static_cast<std::uint64_t>(
            LoadBigEndian32(inputs.address_value_table)) -
        0x10000ULL;
    const std::uint32_t initial = platform->ReadMappedWord(initial_address);
    platform->WriteMappedWord(initial_address, initial & ~0x20u);
    for (const auto& action : values.actions) {
      platform->WriteMappedWord(action.mapped_address, action.mapped_value);
      platform->WriteFlagHalfword(action.flag_table_byte_offset,
                                  action.flag_value);
      platform->FlushAddress(action.flush_address);
    }
    return Success();
  }

  const MappingRangePlan range = PlanMappingRange(inputs.r6);
  for (std::uint32_t index = 0; index < range.iterations; ++index) {
    const std::uint32_t address =
        range.first_address + index * range.address_stride;
    const std::uint32_t value = platform->ReadMappedWord(address);
    platform->WriteMappedWord(address, value & ~range.cleared_bit_mask);
    platform->FlushAddress(address);
  }
  return Success();
}

}  // namespace bandwagon::stage4
