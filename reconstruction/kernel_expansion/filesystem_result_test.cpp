#include "filesystem_result.hpp"

#include <cassert>
#include <cstring>
#include <iostream>

namespace {

class FakeDiagnostics final
    : public bandwagon::kernel_expansion::FileSystemResultDiagnostics {
 public:
  void PrintUnsupportedResult(const char* format,
                              std::int32_t result) override {
    ++prints;
    last_format = format;
    last_result = result;
  }
  void DebugBreak() override { ++breaks; }

  int prints = 0;
  int breaks = 0;
  const char* last_format = nullptr;
  std::int32_t last_result = 0;
};

}  // namespace

int main() {
  using namespace bandwagon::kernel_expansion;

  FakeDiagnostics diagnostics;
  struct Mapping {
    std::int32_t input;
    std::int32_t status;
    std::uint32_t secondary;
  };
  constexpr Mapping mappings[] = {
      {kFatResultNoFile, kStatusObjectNameNotFound, kSecondaryNameFailure},
      {kFatResultNoPath, kStatusObjectPathNotFound, kSecondaryNameFailure},
      {kFatResultInvalidName, kStatusObjectNameInvalid, kSecondaryNameFailure},
      {kFatResultExist, kStatusObjectNameCollision, kSecondaryNameCollision},
  };
  for (const Mapping& mapping : mappings) {
    std::uint32_t secondary = 0xffffffffu;
    assert(MapFatFsResultToNtStatus(mapping.input, &secondary, &diagnostics) ==
           mapping.status);
    assert(secondary == mapping.secondary);
  }
  assert(diagnostics.prints == 0);
  assert(diagnostics.breaks == 0);

  assert(MapFatFsResultToNtStatus(kFatResultNoFile, nullptr, &diagnostics) ==
         kStatusObjectNameNotFound);

  std::uint32_t secondary = 0xffffffffu;
  assert(MapFatFsResultToNtStatus(7, &secondary, &diagnostics) ==
         kStatusUnsuccessful);
  assert(secondary == 0);
  assert(diagnostics.prints == 1);
  assert(diagnostics.breaks == 1);
  assert(diagnostics.last_result == 7);
  assert(std::strcmp(diagnostics.last_format, kUnsupportedFatResultFormat) ==
         0);

  assert(MapFatFsResultToNtStatus(-1, nullptr, nullptr) ==
         kStatusUnsuccessful);

  std::cout << "Kernel Expansion filesystem result tests passed\n";
}
