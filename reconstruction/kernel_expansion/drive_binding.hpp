#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace bandwagon::kernel_expansion {

inline constexpr std::size_t kDrivePathBindingCount = 10;
inline constexpr int kInvalidDrivePathStatus = 11;

// Only the two bytes directly accessed by the recovered binding routine are
// named. The surrounding owner object remains under type recovery.
struct DrivePathBinding {
  std::uint8_t state = 0;
  std::uint8_t drive_number = 0;
};

struct DrivePathBindingTable {
  std::array<DrivePathBinding*, kDrivePathBindingCount> slots{};
};

class DrivePathProbeServices {
 public:
  virtual ~DrivePathProbeServices() = default;

  virtual int Probe(const char16_t** path_cursor, void** device,
                    std::uint32_t flags) = 0;
};

// Lift of the per-drive binding routine at 0x801f3e68. The optional probe sees
// the original path, not the cursor advanced by prefix parsing.
int BindDrivePath(DrivePathBindingTable* table, DrivePathBinding* binding,
                  const char16_t* path, std::uint32_t probe_requested,
                  DrivePathProbeServices* probe_services);

}  // namespace bandwagon::kernel_expansion
