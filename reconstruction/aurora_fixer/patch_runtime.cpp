#include "patch_runtime.hpp"

#include "absolute_branch.hpp"

namespace bandwagon::aurora_fixer {

void PatchAbsoluteBranch(std::uint32_t address, std::uint32_t destination,
                         bool link, std::array<std::uint8_t, 16>* saved_bytes,
                         PatchRuntimePlatform* platform) {
  if (!platform) return;
  if (saved_bytes)
    platform->ReadMemory(address, saved_bytes->data(), saved_bytes->size());
  const auto branch = EncodeAbsoluteBranch(destination, link);
  platform->WriteMemory(address, branch.data(), branch.size());
  platform->MmGetPhysicalAddress(address);
  platform->KeFlushCacheRange(address, 16);
  platform->HvxFlushDcacheRange(address, 16);
}

}  // namespace bandwagon::aurora_fixer
