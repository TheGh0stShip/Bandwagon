#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

#include "resident_probe.hpp"

namespace bandwagon::stage3 {

class Platform : public HypervisorInvoker {
 public:
  ~Platform() override = default;
  virtual std::uint64_t AllocatePhysicalMemory(
      std::uint32_t flags, std::uint32_t size, std::uint32_t protection,
      std::uint32_t minimum_address, std::uint32_t maximum_address,
      std::uint32_t alignment) = 0;
  virtual void ZeroMemory(std::uint64_t address, std::size_t size) = 0;
  virtual void InstallResidentProbe(std::uint64_t address) = 0;
  virtual std::uint32_t GetPhysicalAddress(std::uint64_t address) = 0;
  virtual void SynchronizeCode() = 0;
  virtual void FreePhysicalMemory(std::uint32_t flags,
                                  std::uint64_t address) = 0;
  virtual void RunBadUpdateExploit(std::uint32_t argument) = 0;
  virtual bool ReadFileIntoPhysicalMemory(std::string_view path,
                                          std::uint64_t* allocation,
                                          std::uint32_t* size) = 0;
  virtual void DisplayFatalError(std::uint32_t code) = 0;
  virtual void SendSmcMessage(
      const std::array<std::uint8_t, 16>& message) = 0;
  virtual void LaunchTitle(std::string_view path) = 0;
};

struct RuntimeResult {
  bool completed = false;
  std::uint32_t fatal_code = 0;
  bool used_resident_dispatcher = false;
  bool installed_stage4 = false;
};

// Source-level implementation of GameSaveExploitMain plus its Peer-specific
// resident probe orchestration. InstallResidentProbe is the sole target-code
// boundary: a console backend supplies the recovered 0x38-byte PowerPC probe.
RuntimeResult RunStage3(Platform* platform);

}  // namespace bandwagon::stage3
