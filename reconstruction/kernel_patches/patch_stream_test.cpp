#include "patch_stream.hpp"

#include <cassert>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

int main(int argc, char** argv) {
  using namespace bandwagon::kernel_patches;
  assert(argc == 2);
  std::ifstream input(argv[1], std::ios::binary);
  assert(input);
  const std::vector<std::uint8_t> expected{
      std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
  const auto built = BuildKernelPatchStream();
  assert(built == expected);
  assert(built.size() == 372);
  assert(KernelPatchRecords().size() == 23);
  assert(KernelPatchRecords().front().target == 0x8007b920);
  assert(KernelPatchRecords().back().target == 0x800d8748);
  assert(PatchEffectName(PatchEffect::kReturnZero) == "return zero");

  std::uint32_t value = 0;
  assert(ReturnOneAndOptionallyStoreOne(&value) == 1 && value == 1);
  assert(ReturnOneAndOptionallyStoreOne(nullptr) == 1);

  std::cout << "Kernel patch typed reconstruction tests passed\n";
}
