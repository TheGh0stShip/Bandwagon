#include "config_access.hpp"

#include "config_schema.hpp"

#include <cstring>

namespace bandwagon::kernel_expansion {
namespace {

const StructuredConfigEntry* FindConfigEntryByCachedHash(
    std::uint32_t key_hash) {
  for (const StructuredConfigEntry& entry : kPeerPressureConfigSchema) {
    if (entry.key_hash == key_hash) return &entry;
  }
  return nullptr;
}

}  // namespace

std::uint32_t GetPeerPressureConfigValueByHash(
    PeerPressureConfig* config, std::uint32_t key_hash, void* output,
    std::uint32_t* in_out_size, ConfigLockServices* locks) {
  // The target rejects only the both-null output case and would fault when an
  // output is supplied without a size pointer. Keep the host boundary safe.
  if (!config || !in_out_size || !locks)
    return kConfigStatusInvalidParameter;

  locks->EnterCriticalRegion();
  locks->AcquireShared();

  std::uint32_t status = kConfigStatusNotFound;
  const StructuredConfigEntry* entry =
      FindConfigEntryByCachedHash(key_hash);
  if (entry) {
    const std::uint8_t* source = config->Data() + entry->destination_offset;
    switch (entry->type) {
      case StructuredConfigValueType::kString: {
        const std::uint32_t required = entry->options;
        if (!output || *in_out_size < required) {
          *in_out_size = required;
          status = kConfigStatusBufferTooSmall;
          break;
        }
        std::strncpy(static_cast<char*>(output),
                     reinterpret_cast<const char*>(source), required);
        *in_out_size = required;
        status = kConfigStatusSuccess;
        break;
      }
      case StructuredConfigValueType::kInteger: {
        constexpr std::uint32_t required = sizeof(std::uint32_t);
        if (!output || *in_out_size < required) {
          *in_out_size = required;
          status = kConfigStatusBufferTooSmall;
          break;
        }
        std::memcpy(output, source, required);
        status = kConfigStatusSuccess;
        break;
      }
      case StructuredConfigValueType::kBoolean: {
        constexpr std::uint32_t required = sizeof(std::uint8_t);
        if (!output || *in_out_size < required) {
          *in_out_size = required;
          status = kConfigStatusBufferTooSmall;
          break;
        }
        std::memcpy(output, source, required);
        status = kConfigStatusSuccess;
        break;
      }
      default:
        status = kConfigStatusNotImplemented;
        break;
    }
  }

  locks->Release();
  locks->LeaveCriticalRegion();
  return status;
}

std::uint32_t SetPeerPressureConfigValueByHash(
    PeerPressureConfig* config, std::uint32_t key_hash, const void* value,
    std::uint32_t value_size, ConfigFileWorkflow* workflow,
    ConfigLockServices* locks) {
  if (!config || !value || !workflow || !locks)
    return kConfigStatusInvalidParameter;

  locks->EnterCriticalRegion();
  locks->AcquireExclusive();

  std::uint32_t status = kConfigStatusNotFound;
  bool mutated = false;
  const StructuredConfigEntry* entry =
      FindConfigEntryByCachedHash(key_hash);
  if (entry) {
    std::uint8_t* destination = config->Data() + entry->destination_offset;
    switch (entry->type) {
      case StructuredConfigValueType::kString: {
        const std::uint32_t capacity = entry->options;
        if (value_size < capacity) {
          status = kConfigStatusBufferTooSmall;
          break;
        }
        // The target uses the schema capacity rather than value_size as the
        // strncpy bound, then always terminates the final destination byte.
        std::strncpy(reinterpret_cast<char*>(destination),
                     static_cast<const char*>(value), capacity);
        destination[capacity - 1] = 0;
        mutated = true;
        break;
      }
      case StructuredConfigValueType::kInteger:
        if (value_size < sizeof(std::uint32_t)) {
          status = kConfigStatusBufferTooSmall;
          break;
        }
        std::memcpy(destination, value, sizeof(std::uint32_t));
        mutated = true;
        break;
      case StructuredConfigValueType::kBoolean:
        if (value_size < sizeof(std::uint8_t)) {
          status = kConfigStatusBufferTooSmall;
          break;
        }
        std::memcpy(destination, value, sizeof(std::uint8_t));
        mutated = true;
        break;
      default:
        status = kConfigStatusNotImplemented;
        break;
    }
  }

  if (mutated) {
    status = ProcessPeerPressureConfigFile(1, config, workflow)
                 ? kConfigStatusSuccess
                 : kConfigStatusUnsuccessful;
  }

  locks->Release();
  locks->LeaveCriticalRegion();
  return status;
}

}  // namespace bandwagon::kernel_expansion
