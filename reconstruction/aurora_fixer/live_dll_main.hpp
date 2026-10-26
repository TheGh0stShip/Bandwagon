#pragma once

#include "live_module_import.hpp"

#include <cstdint>

namespace bandwagon::aurora_fixer {

class LiveDllPlatform : public LiveImportPlatform {
 public:
  virtual std::uint32_t GetCurrentModule() = 0;
};

// Complete source-level entry path. The replacement address is passed in so a
// target backend can provide the address of ReturnImmediately in its image.
bool AuroraFixerDllMainLive(std::uint32_t reason,
                            std::uint32_t replacement,
                            LiveDllPlatform* platform);

// Source equivalent of the original one-instruction `blr` routine.
void ReturnImmediately();

}  // namespace bandwagon::aurora_fixer
