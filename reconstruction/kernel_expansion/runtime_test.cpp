#include "runtime.hpp"

#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

class FakeModuleServices final
    : public bandwagon::kernel_expansion::ModuleServices {
 public:
  int load_result = 0;
  int resolve_result = 0;
  bandwagon::kernel_expansion::ModuleHandle returned_module = 0x1234;
  bandwagon::kernel_expansion::Procedure returned_procedure = 0x5678;
  std::vector<std::string> events;

  int LoadImage(std::string_view path, std::uint32_t flags,
                std::uint32_t minimum_version,
                bandwagon::kernel_expansion::ModuleHandle* module) override {
    assert(path == bandwagon::kernel_expansion::kBootXellPath);
    assert(flags == 8 && minimum_version == 0 && module);
    events.emplace_back("load");
    *module = returned_module;
    return load_result;
  }
  int ResolveProcedure(
      bandwagon::kernel_expansion::ModuleHandle module,
      std::uint32_t ordinal,
      bandwagon::kernel_expansion::Procedure* procedure) override {
    assert(module == returned_module && ordinal == 1 && procedure);
    events.emplace_back("resolve");
    *procedure = returned_procedure;
    return resolve_result;
  }
  void InvokeProcedure(
      bandwagon::kernel_expansion::Procedure procedure) override {
    assert(procedure == returned_procedure);
    events.emplace_back("invoke");
  }
  void UnloadImage(
      bandwagon::kernel_expansion::ModuleHandle module) override {
    assert(module == returned_module);
    events.emplace_back("unload");
  }
};

std::array<std::uint8_t, 0x5c> FixedPointGptHeader() {
  std::array<std::uint8_t, 0x5c> header{};
  constexpr std::array<std::uint8_t, 12> prefix{{
      'E', 'F', 'I', ' ', 'P', 'A', 'R', 'T', 0, 0, 1, 0,
  }};
  std::copy(prefix.begin(), prefix.end(), header.begin());
  header[0x0c] = 0x5c;
  // This little-endian value is the CRC fixed point for this test header. It
  // exercises the binary's unusual self-including checksum comparison.
  header[0x10] = 0x4a;
  header[0x11] = 0x5c;
  header[0x12] = 0x85;
  header[0x13] = 0x8a;
  header[0x50] = 0x80;
  header[0x54] = 0x80;
  return header;
}

}  // namespace

int main() {
  using namespace bandwagon::kernel_expansion;

  FakeModuleServices success;
  LaunchBootXell(&success);
  assert((success.events ==
          std::vector<std::string>{"load", "resolve", "invoke", "unload"}));

  FakeModuleServices resolve_failure;
  resolve_failure.resolve_result = -1;
  LaunchBootXell(&resolve_failure);
  assert((resolve_failure.events ==
          std::vector<std::string>{"load", "resolve", "unload"}));

  FakeModuleServices load_failure;
  load_failure.load_result = -1;
  LaunchBootXell(&load_failure);
  assert((load_failure.events == std::vector<std::string>{"load", "unload"}));

  FakeModuleServices empty_failure;
  empty_failure.load_result = -1;
  empty_failure.returned_module = 0;
  LaunchBootXell(&empty_failure);
  assert((empty_failure.events == std::vector<std::string>{"load"}));
  LaunchBootXell(nullptr);

  auto header = FixedPointGptHeader();
  assert(ComputeRecoveredGptChecksum(header.data(), header.size()) ==
         0x8a855c4a);
  assert(IsRecognizedGptHeader(header.data(), header.size()));

  header[0] = 'X';
  assert(!IsRecognizedGptHeader(header.data(), header.size()));
  header = FixedPointGptHeader();
  header[0x50] = 0x81;
  assert(!IsRecognizedGptHeader(header.data(), header.size()));
  header = FixedPointGptHeader();
  header[0x54] = 0x40;
  assert(!IsRecognizedGptHeader(header.data(), header.size()));
  header = FixedPointGptHeader();
  header[0x0c] = 0x5b;
  assert(!IsRecognizedGptHeader(header.data(), header.size()));
  assert(!IsRecognizedGptHeader(nullptr, 0));

  assert(IsAsciiAlphaNumeric('0'));
  assert(IsAsciiAlphaNumeric('9'));
  assert(IsAsciiAlphaNumeric('A'));
  assert(IsAsciiAlphaNumeric('Z'));
  assert(IsAsciiAlphaNumeric('a'));
  assert(IsAsciiAlphaNumeric('z'));
  assert(!IsAsciiAlphaNumeric('/'));
  assert(!IsAsciiAlphaNumeric(':'));
  assert(!IsAsciiAlphaNumeric('@'));
  assert(!IsAsciiAlphaNumeric('['));
  assert(!IsAsciiAlphaNumeric('`'));
  assert(!IsAsciiAlphaNumeric('{'));
  assert(!IsAsciiAlphaNumeric(0x80));

  std::cout << "Kernel Expansion runtime reconstruction tests passed\n";
}
