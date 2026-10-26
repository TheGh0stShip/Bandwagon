#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace bandwagon::updater {

constexpr std::size_t kBootstrapHeaderSize = 16;
constexpr std::size_t kBootstrapSectorSize = 512;
constexpr std::size_t kMaximumStage2Size = 0x7c000;
constexpr std::uint64_t kBootstrapDiskOffset = 0x4000;

using Crc32 = std::uint32_t (*)(std::uint32_t seed, const std::uint8_t* data,
                               std::size_t size);

// Reconstructs the record prepared by InstallHddBootstrap before NtWriteFile.
// Throws std::invalid_argument for an empty/oversized payload or null CRC.
// The closed binary leaves final-sector padding uninitialized. This reviewable
// replacement intentionally clears it to avoid persisting stale heap bytes.
std::vector<std::uint8_t> BuildHddBootstrapRecord(
    const std::uint8_t* stage2, std::size_t stage2_size,
    std::uint32_t softmod_context_tag, Crc32 crc32);

}  // namespace bandwagon::updater
