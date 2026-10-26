#pragma once

#include "boot_options.hpp"
#include "xell_scan.hpp"

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace bandwagon::bootxell {

inline constexpr std::string_view kBootIniPath =
    "PPROOT:\\OtherOS\\boot.ini";
using FileHandle = std::uintptr_t;
inline constexpr FileHandle kInvalidFileHandle =
    static_cast<FileHandle>(-1);

using IniHandler = int (*)(void*, const char*, const char*, const char*);

class IniStringParser {
 public:
  virtual ~IniStringParser() = default;
  virtual bool ParseString(const char* contents, IniHandler handler,
                           void* user) = 0;
};

class BootXellServices {
 public:
  virtual ~BootXellServices() = default;

  virtual FileHandle OpenRead(std::string_view path) = 0;
  virtual bool QueryFileSize(FileHandle handle, std::uint32_t* size) = 0;
  virtual bool ReadFile(FileHandle handle, std::uint8_t* destination,
                        std::uint32_t size,
                        std::uint32_t* bytes_read) = 0;
  virtual void CloseFile(FileHandle handle) = 0;

  virtual std::uint8_t* AllocateHeap(std::size_t size) = 0;
  virtual void FreeHeap(std::uint8_t* allocation) = 0;
  virtual std::uint8_t* AllocatePhysical(std::uint32_t size,
                                         std::uint32_t alignment,
                                         std::uint32_t flags) = 0;
  virtual void FreePhysical(std::uint8_t* allocation) = 0;
  virtual std::uint32_t PhysicalAddress(const std::uint8_t* allocation) = 0;

  virtual void SubmitRbtr(const RbtrLaunchRequest& request) = 0;
  virtual bool ReadRequestedOptionName(char* destination,
                                       std::size_t capacity) = 0;
};

// Lift of ReadFileToBuffer at 0x91a07550. The returned size includes the
// appended NUL, matching the original file_size + 1 output.
bool ReadFileToBuffer(BootXellServices* services, std::string_view path,
                      std::uint8_t** buffer, std::uint32_t* allocation_size);

// Lift of LoadBootIni at 0x91a06450. The inih r62 parser remains an explicit
// third-party boundary.
bool LoadBootIni(BootXellServices* services, IniStringParser* parser,
                 std::string_view path, BootOptionParserState* state);

// Lift of LaunchBootOption at 0x91a064d8.
bool LaunchBootOption(BootXellServices* services, XellCompression* compression,
                      const BootOption& option);

// Lift of BootXellMain at 0x91a06658.
bool BootXellMain(BootXellServices* services, IniStringParser* parser,
                  XellCompression* compression);

}  // namespace bandwagon::bootxell
