#include "runtime.hpp"

#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <map>
#include <tuple>
#include <utility>
#include <vector>

namespace {

void Append32(std::vector<std::uint8_t>* bytes, std::uint32_t value) {
  bytes->push_back(static_cast<std::uint8_t>(value >> 24));
  bytes->push_back(static_cast<std::uint8_t>(value >> 16));
  bytes->push_back(static_cast<std::uint8_t>(value >> 8));
  bytes->push_back(static_cast<std::uint8_t>(value));
}

class MockPlatform final : public bandwagon::stage4::Platform {
 public:
  void WriteSmcMmioWord(std::uint64_t address,
                        std::uint32_t value) override {
    smc_writes.emplace_back(address, value);
  }
  void SmcIoBarrier() override { ++smc_barriers; }
  void RelocateCacheLines(const std::uint8_t* source, std::uint64_t destination,
                          std::uint32_t count) override {
    relocate_source = source;
    relocate_destination = destination;
    relocate_count = count;
  }
  std::uint32_t ReadMappedWord(std::uint64_t address) override {
    return words[address];
  }
  void WriteMappedWord(std::uint64_t address, std::uint32_t value) override {
    words[address] = value;
    word_writes.emplace_back(address, value);
  }
  std::uint64_t GetFlagTableBase() override {
    ++flag_table_lookups;
    return flag_table_base;
  }
  void WriteMappedHalfword(std::uint64_t address,
                           std::uint16_t value) override {
    flags.emplace_back(address, value);
  }
  void FlushAddress(std::uint64_t address) override {
    flushes.push_back(address);
  }
  void WriteBytes(std::uint64_t address, const std::uint8_t* data,
                  std::size_t size) override {
    byte_writes.emplace_back(address,
                             std::vector<std::uint8_t>(data, data + size));
  }

  std::vector<std::pair<std::uint64_t, std::uint32_t>> smc_writes;
  std::uint32_t smc_barriers = 0;
  const std::uint8_t* relocate_source = nullptr;
  std::uint64_t relocate_destination = 0;
  std::uint32_t relocate_count = 0;
  std::map<std::uint64_t, std::uint32_t> words;
  std::vector<std::pair<std::uint64_t, std::uint32_t>> word_writes;
  std::uint64_t flag_table_base = 0x80000000u;
  std::uint32_t flag_table_lookups = 0;
  std::vector<std::pair<std::uint64_t, std::uint16_t>> flags;
  std::vector<std::uint64_t> flushes;
  std::vector<std::pair<std::uint64_t, std::vector<std::uint8_t>>> byte_writes;
};

}  // namespace

int main() {
  using bandwagon::stage4::RunStage4;
  using bandwagon::stage4::RuntimeInputs;

  std::array<std::uint8_t, 0x10000> clean{};
  std::vector<std::uint8_t> patches;
  Append32(&patches, 0x00015fd0);
  Append32(&patches, 2);
  patches.push_back(0xaa);
  patches.push_back(0xbb);
  Append32(&patches, 0xffffffff);
  MockPlatform mode0_platform;
  RuntimeInputs mode0;
  mode0.clean_hv = clean.data();
  mode0.clean_hv_size = clean.size();
  mode0.patch_stream = patches.data();
  mode0.patch_stream_size = patches.size();
  mode0.input_r0 = 0x1122334455667788ULL;
  const auto mode0_result = RunStage4(mode0, &mode0_platform);
  assert(mode0_result.succeeded && mode0_result.return_value == 0x41414141u);
  assert(mode0_platform.smc_writes.size() == 6);
  assert(mode0_platform.smc_barriers == 6);
  using SmcWrite = std::pair<std::uint64_t, std::uint32_t>;
  assert(mode0_platform.smc_writes[0] ==
         SmcWrite(0x80000200ea001084ULL, 0x04000000u));
  assert(mode0_platform.smc_writes[1] ==
         SmcWrite(0x80000200ea001080ULL, 0x99ffff44u));
  assert(mode0_platform.smc_writes[3] ==
         SmcWrite(0x80000200ea001080ULL, 0x11223344u));
  assert(mode0_platform.smc_writes[4] ==
         SmcWrite(0x80000200ea001080ULL, 0x55667788u));
  assert(mode0_platform.smc_writes[5] ==
         SmcWrite(0x80000200ea001084ULL, 0u));
  assert(mode0_platform.relocate_source == clean.data());
  assert(mode0_platform.relocate_destination == 0x8000010600030000ULL);
  assert(mode0_platform.relocate_count == 0x200);
  assert(mode0_platform.byte_writes.size() == 1);
  assert(mode0_platform.byte_writes[0].first == 0x200015fd0ULL);

  MockPlatform embedded_platform;
  RuntimeInputs embedded;
  embedded.clean_hv = clean.data();
  embedded.clean_hv_size = clean.size();
  assert(RunStage4(embedded, &embedded_platform).succeeded);
  assert(embedded_platform.byte_writes.size() == 27);
  assert(embedded_platform.byte_writes.front().first == 0x12d8);
  assert(embedded_platform.byte_writes.back().first == 0x600031fb8ULL);
  assert(embedded_platform.byte_writes.back().second.size() == 0x220);

  std::vector<std::uint8_t> table;
  Append32(&table, 0x80050000);
  Append32(&table, 0x12345678);
  Append32(&table, 0xffffffff);
  MockPlatform mode1_platform;
  mode1_platform.words[0x80040000] = 0xffffffff;
  RuntimeInputs mode1;
  mode1.mode_register = 1;
  mode1.address_value_table = table.data();
  mode1.address_value_table_size = table.size();
  const auto mode1_result = RunStage4(mode1, &mode1_platform);
  assert(mode1_result.succeeded);
  const std::pair<std::uint64_t, std::uint32_t> initial_write{0x80040000,
                                                              0xffffffdf};
  const std::pair<std::uint64_t, std::uint32_t> table_write{0x80050000,
                                                            0x12345678};
  assert(mode1_platform.word_writes[0] == initial_write);
  assert(mode1_platform.word_writes[1] == table_write);
  assert(mode1_platform.flags.size() == 1 &&
         mode1_platform.flag_table_lookups == 1 &&
         mode1_platform.flags[0].first ==
             mode1_platform.flag_table_base +
                 2u * ((0x12345678u >> 6) & 0x3fffu) &&
         mode1_platform.flags[0].second == 0x0800);
  assert(mode1_platform.flushes == std::vector<std::uint64_t>{0x80050000});

  MockPlatform mode2_platform;
  mode2_platform.words[0x80040000] = 0xf;
  mode2_platform.words[0x80050000] = 0xa;
  RuntimeInputs mode2;
  mode2.mode_register = 2;
  mode2.r6 = 2;
  assert(RunStage4(mode2, &mode2_platform).succeeded);
  assert(mode2_platform.words[0x80040000] == 8);
  assert(mode2_platform.words[0x80050000] == 8);
  assert(mode2_platform.flushes.size() == 2);

  RuntimeInputs unsupported;
  unsupported.mode_register = 3;
  assert(RunStage4(unsupported, &mode2_platform).succeeded);
  assert(!RunStage4(mode0, nullptr).succeeded);

  std::cout << "Stage4 runtime reconstruction tests passed\n";
}
