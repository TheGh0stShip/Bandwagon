#include "file_disposition.hpp"

namespace bandwagon::kernel_expansion {

std::uint32_t MapCreateDispositionToOpenFlags(
    std::uint32_t disposition, FileDispositionDiagnostics* diagnostics) {
  switch (static_cast<FileCreateDisposition>(disposition)) {
    case FileCreateDisposition::kSupersede:
    case FileCreateDisposition::kOverwriteIf:
      return kSupersedeOrOverwriteIfFlag;
    case FileCreateDisposition::kOpen:
    case FileCreateDisposition::kOverwrite:
      return 0;
    case FileCreateDisposition::kCreate:
      return kCreateFlag;
    case FileCreateDisposition::kOpenIf:
      return kOpenIfFlag;
  }

  if (diagnostics) diagnostics->DebugBreak();
  return 0;
}

}  // namespace bandwagon::kernel_expansion
