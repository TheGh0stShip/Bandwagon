#pragma once

#include "runtime.hpp"

#include <string_view>

namespace bandwagon::kernel_expansion {

inline constexpr std::string_view kPhaseTwoMarkerPath =
    "PPROOT:\\update.phase2";

class PhaseTwoFileServices {
 public:
  virtual ~PhaseTwoFileServices() = default;
  virtual int DeleteFile(std::string_view path) = 0;
  virtual bool MoveFile(std::string_view source,
                        std::string_view destination) = 0;
};

// Lift of 0x801fa688. Absence of the marker is already-complete success. When
// present, every newline-terminated path is deleted and restored from a path
// with '_' appended; the marker is deleted only after all entries succeed.
bool CompletePhaseTwoFileRestore(FileServices* files,
                                 PhaseTwoFileServices* phase_two);

}  // namespace bandwagon::kernel_expansion
