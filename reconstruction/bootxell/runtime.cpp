#include "runtime.hpp"

#include <array>
#include <limits>

namespace bandwagon::bootxell {

bool ReadFileToBuffer(BootXellServices* services, std::string_view path,
                      std::uint8_t** buffer, std::uint32_t* allocation_size) {
  if (!services || !buffer || !allocation_size) return false;
  *buffer = nullptr;
  *allocation_size = 0;
  const FileHandle handle = services->OpenRead(path);
  if (handle == kInvalidFileHandle) return false;

  bool success = false;
  std::uint8_t* allocation = nullptr;
  std::uint32_t file_size = 0;
  if (services->QueryFileSize(handle, &file_size) &&
      file_size != std::numeric_limits<std::uint32_t>::max()) {
    const std::uint32_t requested_size = file_size + 1u;
    if (requested_size != 0) {
      allocation = services->AllocateHeap(requested_size);
      std::uint32_t bytes_read = 0;
      if (allocation &&
          services->ReadFile(handle, allocation, file_size, &bytes_read) &&
          bytes_read != 0) {
        allocation[file_size] = 0;
        *buffer = allocation;
        *allocation_size = requested_size;
        allocation = nullptr;
        success = true;
      }
    }
  }
  if (allocation) services->FreeHeap(allocation);
  services->CloseFile(handle);
  return success;
}

bool LoadBootIni(BootXellServices* services, IniStringParser* parser,
                 std::string_view path, BootOptionParserState* state) {
  if (!services || !parser || !state) return false;
  std::uint8_t* buffer = nullptr;
  std::uint32_t allocation_size = 0;
  if (!ReadFileToBuffer(services, path, &buffer, &allocation_size))
    return false;
  (void)allocation_size;
  const bool parsed = parser->ParseString(
      reinterpret_cast<const char*>(buffer), ParseBootIniOption, state);
  services->FreeHeap(buffer);
  FinalizeBootIniOptions(*state);
  return parsed;
}

bool LaunchBootOption(BootXellServices* services, XellCompression* compression,
                      const BootOption& option) {
  if (!services) return false;
  const std::string path = BuildOtherOsPath(option);
  const FileHandle handle = services->OpenRead(path);
  if (handle == kInvalidFileHandle) return false;

  std::uint8_t* allocation = nullptr;
  bool success = false;
  std::uint32_t file_size = 0;
  if (services->QueryFileSize(handle, &file_size)) {
    const std::uint32_t aligned_size = (file_size + 127u) & ~127u;
    allocation = services->AllocatePhysical(aligned_size, 128, 0x204);
    std::uint32_t bytes_read = 0;
    if (allocation &&
        services->ReadFile(handle, allocation, file_size, &bytes_read)) {
      bool scan_succeeded = false;
      if (HasXellScanMarker(allocation, file_size) &&
          !option.bypass_xell_scanning) {
        scan_succeeded =
            SanitizeXellImage(allocation, file_size, compression);
      }
      if (IsLaunchImageAllowed(option, allocation, file_size,
                               scan_succeeded)) {
        const RbtrLaunchRequest request = BuildRbtrLaunchRequest(
            option, services->PhysicalAddress(allocation), file_size);
        services->SubmitRbtr(request);
        success = true;
      }
    }
  }
  if (allocation) services->FreePhysical(allocation);
  services->CloseFile(handle);
  return success;
}

bool BootXellMain(BootXellServices* services, IniStringParser* parser,
                  XellCompression* compression) {
  if (!services || !parser) return false;
  std::array<char, 65> requested{};
  if (!services->ReadRequestedOptionName(requested.data(), 64)) return false;

  BootOptionParserState state;
  if (!LoadBootIni(services, parser, kBootIniPath, &state)) return false;
  const BootOption* option = FindBootOption(state.options, requested.data());
  return option && LaunchBootOption(services, compression, *option);
}

}  // namespace bandwagon::bootxell
