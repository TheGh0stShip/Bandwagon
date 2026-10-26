#pragma once

#include "config.hpp"
#include "config_persistence.hpp"

#include <cstdint>

namespace bandwagon::kernel_expansion {

inline constexpr std::uint32_t kConfigStatusSuccess = 0x00000000u;
inline constexpr std::uint32_t kConfigStatusUnsuccessful = 0xc0000001u;
inline constexpr std::uint32_t kConfigStatusNotImplemented = 0xc0000002u;
inline constexpr std::uint32_t kConfigStatusInvalidParameter = 0xc000000du;
inline constexpr std::uint32_t kConfigStatusBufferTooSmall = 0xc0000023u;
inline constexpr std::uint32_t kConfigStatusNotFound = 0xc0000225u;

// Lift of the exported hash-only getter at 0x801f1178. String queries return
// the schema capacity, including terminator space; successful scalar reads do
// not rewrite the caller's supplied size. The target trusts a non-null size
// pointer whenever an output is supplied. A supported matching entry would
// dereference a null size pointer, while this host lift safely rejects it.
std::uint32_t GetPeerPressureConfigValueByHash(
    PeerPressureConfig* config, std::uint32_t key_hash, void* output,
    std::uint32_t* in_out_size, ConfigLockServices* locks);

// Lift of the exported hash-only setter at 0x801f1328. A successful mutation
// is persisted in mode one while the exclusive lock remains held. Persistence
// failure does not roll the in-memory mutation back. String input buffers must
// cover the full fixed schema capacity because the target copies that many
// bytes regardless of the string's logical length.
std::uint32_t SetPeerPressureConfigValueByHash(
    PeerPressureConfig* config, std::uint32_t key_hash, const void* value,
    std::uint32_t value_size, ConfigFileWorkflow* workflow,
    ConfigLockServices* locks);

}  // namespace bandwagon::kernel_expansion
