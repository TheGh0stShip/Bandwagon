#include "filesystem_result.hpp"

namespace bandwagon::kernel_expansion {

std::int32_t MapFatFsResultToNtStatus(
    std::int32_t result, std::uint32_t* secondary_code,
    FileSystemResultDiagnostics* diagnostics) {
  std::int32_t status = kStatusUnsuccessful;
  std::uint32_t secondary = 0;

  switch (result) {
    case kFatResultNoFile:
      status = kStatusObjectNameNotFound;
      secondary = kSecondaryNameFailure;
      break;
    case kFatResultNoPath:
      status = kStatusObjectPathNotFound;
      secondary = kSecondaryNameFailure;
      break;
    case kFatResultInvalidName:
      status = kStatusObjectNameInvalid;
      secondary = kSecondaryNameFailure;
      break;
    case kFatResultExist:
      status = kStatusObjectNameCollision;
      secondary = kSecondaryNameCollision;
      break;
    default:
      if (diagnostics) {
        diagnostics->PrintUnsupportedResult(kUnsupportedFatResultFormat,
                                            result);
        diagnostics->DebugBreak();
      }
      break;
  }

  if (secondary_code) *secondary_code = secondary;
  return status;
}

}  // namespace bandwagon::kernel_expansion
