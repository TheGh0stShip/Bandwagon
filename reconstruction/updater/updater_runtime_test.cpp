#include "updater_runtime.hpp"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace {

using bandwagon::updater::ArchiveChunkSink;
using bandwagon::updater::ArchiveEntryInfo;
using bandwagon::updater::ArchiveReader;
using bandwagon::updater::DirectoryCreateResult;
using bandwagon::updater::Sha1Digest;
using bandwagon::updater::UpdatePlatform;

Sha1Digest FakeSha1(const std::uint8_t* data, std::size_t size) {
  Sha1Digest digest{};
  for (std::size_t i = 0; i < size; ++i) {
    digest[i % digest.size()] = static_cast<std::uint8_t>(
        digest[i % digest.size()] * 33u + data[i] + i);
  }
  return digest;
}

std::uint32_t FakeCrc32(std::uint32_t seed, const std::uint8_t* data,
                        std::size_t size) {
  std::uint32_t value = seed;
  for (std::size_t i = 0; i < size; ++i) value = value * 33u + data[i];
  return value;
}

struct FakeEntry {
  std::string path;
  std::vector<std::uint8_t> bytes;
};

class FakeArchive final : public ArchiveReader {
 public:
  explicit FakeArchive(std::vector<FakeEntry> entries)
      : entries_(std::move(entries)) {}

  bool OpenMemory(const std::uint8_t* data, std::size_t size) override {
    opened_ = data && size >= 4 && data[0] == 'P' && data[1] == 'K' &&
              data[2] == 3 && data[3] == 4;
    if (!opened_) error_ = "bad ZIP input";
    return opened_;
  }

  std::size_t EntryCount() const override {
    return opened_ ? entries_.size() : 0;
  }

  bool Locate(std::string_view path, std::uint32_t* index) const override {
    if (!opened_ || !index) return false;
    for (std::size_t i = 0; i < entries_.size(); ++i) {
      if (entries_[i].path == path) {
        *index = static_cast<std::uint32_t>(i);
        return true;
      }
    }
    error_ = "member not found";
    return false;
  }

  bool Stat(std::uint32_t index, ArchiveEntryInfo* entry) const override {
    if (!opened_ || !entry || index >= entries_.size()) {
      error_ = "bad member index";
      return false;
    }
    *entry = {};
    entry->index = index;
    entry->path = entries_[index].path;
    entry->uncompressed_size = entries_[index].bytes.size();
    return true;
  }

  bool ExtractToBuffer(std::uint32_t index, std::uint8_t* output,
                       std::size_t size) override {
    if (!opened_ || index >= entries_.size() ||
        size != entries_[index].bytes.size() || (!output && size != 0)) {
      error_ = "bad extraction buffer";
      return false;
    }
    std::copy(entries_[index].bytes.begin(), entries_[index].bytes.end(),
              output);
    return true;
  }

  bool ExtractToCallback(std::uint32_t index,
                         const ArchiveChunkSink& sink) override {
    if (!opened_ || index >= entries_.size() || !sink) return false;
    const auto& bytes = entries_[index].bytes;
    for (std::size_t at = 0; at < bytes.size(); at += 3) {
      const std::size_t count = std::min<std::size_t>(3, bytes.size() - at);
      if (sink(at, bytes.data() + at, count) != count) return false;
    }
    return true;
  }

  std::string LastError() const override { return error_; }

 private:
  std::vector<FakeEntry> entries_;
  bool opened_ = false;
  mutable std::string error_;
};

class FakePlatform final : public UpdatePlatform {
 public:
  bool ReadWholeFile(std::string_view path,
                     std::vector<std::uint8_t>* output) override {
    const auto found = files.find(std::string(path));
    if (!output || found == files.end()) return false;
    *output = found->second;
    return true;
  }

  bool FileExists(std::string_view path) const override {
    return files.find(std::string(path)) != files.end();
  }

  DirectoryCreateResult CreateDirectory(std::string_view path) override {
    const bool inserted = directories.insert(std::string(path)).second;
    return inserted ? DirectoryCreateResult::kCreated
                    : DirectoryCreateResult::kAlreadyExists;
  }

  bool BeginFile(std::string_view path) override {
    files[std::string(path)].clear();
    return true;
  }

  bool WriteFileAt(std::string_view path, std::uint64_t offset,
                   const std::uint8_t* data, std::size_t size,
                   std::size_t* written) override {
    auto& file = files[std::string(path)];
    const std::size_t at = static_cast<std::size_t>(offset);
    if (at > file.size()) file.resize(at, 0);
    if (at + size > file.size()) file.resize(at + size);
    std::copy_n(data, size, file.begin() + at);
    if (written) *written = size;
    return true;
  }

  bool WriteWholeFile(std::string_view path, const std::uint8_t* data,
                      std::size_t size) override {
    files[std::string(path)] = std::vector<std::uint8_t>(data, data + size);
    return true;
  }

  bool WritePhysicalDisk(std::uint64_t offset, const std::uint8_t* data,
                         std::size_t size) override {
    disk_offset = offset;
    disk.assign(data, data + size);
    return true;
  }

  std::map<std::string, std::vector<std::uint8_t>> files;
  std::set<std::string> directories;
  std::uint64_t disk_offset = 0;
  std::vector<std::uint8_t> disk;
};

std::vector<std::uint8_t> Bytes(std::string_view text) {
  return std::vector<std::uint8_t>(text.begin(), text.end());
}

}  // namespace

int main() {
  using namespace bandwagon::updater;

  SoftmodManifest manifest;
  manifest.softmod_major = 1;
  manifest.softmod_minor = 0;
  manifest.softmod_revision = 477;
  manifest.entries = {
      {{}, 0, "Hdd\\PeerPressureExploit-Stage2.bin"},
      {{}, kManifestPreserveExisting, "PeerPressure\\OtherOS\\boot.ini"},
      {{}, 0, "PeerPressure\\System\\new.bin"},
      {{}, kManifestPhaseTwo, "PeerPressure\\System\\HudResources.xzp"},
  };
  const std::vector<std::uint8_t> manifest_bytes =
      BuildSoftmodManifest(manifest, FakeSha1);
  const std::vector<std::uint8_t> stage2(37, 0x5a);
  const std::vector<std::uint8_t> zip = {'P', 'K', 3, 4, 0x42};
  const std::vector<std::uint8_t> wrapped = BuildUpdateArchiveWrapper(
      {1, 0, 477}, zip.data(), zip.size(), FakeSha1);

  FakeArchive archive({
      {"softmod.manifest", manifest_bytes},
      {"Hdd/PeerPressureExploit-Stage2.bin", stage2},
      {"PeerPressure/OtherOS/boot.ini", Bytes("replacement")},
      {"PeerPressure/System/new.bin", Bytes("new")},
      {"PeerPressure/System/HudResources.xzp", Bytes("late")},
  });
  FakePlatform platform;
  platform.files["update.bin"] = wrapped;
  platform.files["PPROOT:\\PeerPressure\\OtherOS\\boot.ini"] = Bytes("keep");

  std::vector<std::string> diagnostics;
  std::vector<std::string> progress_messages;
  UpdateCallbacks callbacks;
  callbacks.diagnostic = [&diagnostics](std::string_view message) {
    diagnostics.emplace_back(message);
  };
  callbacks.progress = [&progress_messages](float, std::string_view message) {
    progress_messages.emplace_back(message);
  };

  assert(RunUpdate("update.bin", platform, archive, FakeSha1, FakeCrc32,
                   callbacks));
  assert(diagnostics.empty());
  assert(platform.files["PPROOT:\\PeerPressure\\OtherOS\\boot.ini"] ==
         Bytes("keep"));
  assert(platform.files["PPROOT:\\PeerPressure\\System\\new.bin"] ==
         Bytes("new"));
  assert(platform.files[
             "PPROOT:\\PeerPressure\\System\\HudResources.xzp_"] ==
         Bytes("late"));
  assert(platform.files["PPROOT:\\update.phase2"] ==
         Bytes("PPROOT:\\PeerPressure\\System\\HudResources.xzp\n"));
  assert(platform.disk_offset == kBootstrapDiskOffset);
  assert(platform.disk.size() == kBootstrapSectorSize);
  assert(std::equal(platform.disk.begin(), platform.disk.begin() + 4,
                    Bytes("XBPP").begin()));
  assert(platform.disk[4] == 1 && platform.disk[5] == 0 &&
         platform.disk[6] == 1 && platform.disk[7] == 0xdd);
  assert(progress_messages.front() == "Loading update data...");
  assert(progress_messages.back() == "Finalizing...");

  std::vector<std::uint8_t> direct(stage2.size());
  std::size_t direct_size = 0;
  assert(ZipArchiveExtractToBuffer(
      archive, kStage2ArchivePath, direct.data(), direct.size(), &direct_size));
  assert(direct_size == stage2.size() && direct == stage2);
  assert(!ZipArchiveExtractToBuffer(archive, kStage2ArchivePath,
                                    direct.data(), direct.size() - 1,
                                    &direct_size));
  std::vector<std::uint8_t> heap_copy;
  assert(ZipArchiveExtractToHeap(archive, "softmod.manifest", &heap_copy));
  assert(heap_copy == manifest_bytes);

  assert(CompareStringsWithCaseOption("PeerPressure/System", "peerpressure/",
                                      true));
  assert(!CompareStringsWithCaseOption("short", "shorter", true));
  assert(!CompareStringsWithCaseOption("Peer", "peer", false));
  assert(UpdaterDllLifecycleHook(0));
  assert(&GetSoftmodDataContext() == &GetSoftmodDataContext());
  assert(FileExists(platform,
                    "PPROOT:\\PeerPressure\\System\\new.bin"));

  std::vector<std::string> staged;
  AppendPhaseTwoPath(&staged, "one");
  assert((staged == std::vector<std::string>{"one"}));

  std::string formatted;
  assert(FormatDiagnosticAndDispatch(
             [&formatted](std::string_view value) { formatted = value; },
             "failure %d", 7) == 9);
  assert(formatted == "failure 7");

  std::cout << "Updater runtime reconstruction tests passed\n";
}
