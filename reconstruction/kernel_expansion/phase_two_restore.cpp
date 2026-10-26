#include "phase_two_restore.hpp"

#include <array>
#include <cstdio>

namespace bandwagon::kernel_expansion {

char* FindCharacterOnLine(char* text, char target) {
  if (!text) return nullptr;
  for (;;) {
    if (*text == target) return text;
    if (*text == '\0' || *text == '\n') return nullptr;
    ++text;
  }
}

bool CompletePhaseTwoFileRestore(FileServices* files,
                                 PhaseTwoFileServices* phase_two) {
  if (!files || !phase_two) return false;

  std::uint8_t* allocation = nullptr;
  std::uint32_t allocation_size = 0;
  if (!ReadFileToAllocatedBuffer(files, kPhaseTwoMarkerPath, &allocation,
                                 &allocation_size)) {
    return true;
  }

  bool complete = false;
  if (allocation) {
    char* const begin = reinterpret_cast<char*>(allocation);
    char* const end = begin + allocation_size;
    char* cursor = begin;
    while (cursor < end) {
      char* newline = FindCharacterOnLine(cursor, '\n');
      if (!newline) break;

      *newline = '\0';
      if (phase_two->DeleteFile(cursor) != 0) goto cleanup;

      std::array<char, 0x104> backup{};
      std::snprintf(backup.data(), backup.size(), "%s_", cursor);
      if (!phase_two->MoveFile(backup.data(), cursor)) goto cleanup;
      cursor = newline + 1;
    }
  }

  if (phase_two->DeleteFile(kPhaseTwoMarkerPath) == 0) complete = true;

cleanup:
  if (allocation) files->Free(allocation);
  return complete;
}

}  // namespace bandwagon::kernel_expansion
