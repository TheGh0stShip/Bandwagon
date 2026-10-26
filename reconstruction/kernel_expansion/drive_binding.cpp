#include "drive_binding.hpp"

#include "drive_prefix.hpp"

namespace bandwagon::kernel_expansion {

int BindDrivePath(DrivePathBindingTable* table, DrivePathBinding* binding,
                  const char16_t* path, std::uint32_t probe_requested,
                  DrivePathProbeServices* probe_services) {
  if (!table || !path) return kInvalidDrivePathStatus;

  const char16_t* parsed_cursor = path;
  const std::int32_t drive_number = ParseDriveNumberPrefix(&parsed_cursor);
  if (drive_number < 0 ||
      static_cast<std::size_t>(drive_number) >= table->slots.size()) {
    return kInvalidDrivePathStatus;
  }

  DrivePathBinding*& slot =
      table->slots[static_cast<std::size_t>(drive_number)];
  if (slot) {
    DrivePathBinding* previous = slot;
    slot = nullptr;
    previous->state = 0;
  }

  if (binding) {
    binding->drive_number = static_cast<std::uint8_t>(drive_number);
    binding->state = 0;
    slot = binding;
  }

  if ((probe_requested & 0xffu) == 0) return 0;
  if (!probe_services) return kInvalidDrivePathStatus;

  const char16_t* probe_cursor = path;
  void* device = nullptr;
  return probe_services->Probe(&probe_cursor, &device, 0);
}

}  // namespace bandwagon::kernel_expansion
