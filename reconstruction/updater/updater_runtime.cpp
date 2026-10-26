#include "updater_runtime.hpp"

#include "parent_directories.hpp"
#include "phase_two_list.hpp"

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <limits>
#include <stdexcept>
#include <utility>

namespace bandwagon::updater {
namespace {

constexpr std::size_t kDiagnosticCapacity = 1024;

void Report(const DiagnosticSink& sink, std::string_view message) {
  if (sink) sink(message);
}

bool EqualAscii(std::string_view left, std::string_view right) {
  return left.size() == right.size() &&
         CompareStringsWithCaseOption(left, right, true);
}

unsigned char FoldAscii(unsigned char value) {
  if (value >= 'A' && value <= 'Z') {
    return static_cast<unsigned char>(value + ('a' - 'A'));
  }
  return value;
}

std::string ArchiveError(ArchiveReader& archive, std::string_view operation,
                         std::string_view path) {
  std::string message(operation);
  message.append(" '");
  message.append(path);
  message.append("': ");
  message.append(archive.LastError());
  return message;
}

}  // namespace

int FormatDiagnosticAndDispatch(const DiagnosticSink& sink,
                                const char* format, ...) {
  if (!format) return -1;
  char buffer[kDiagnosticCapacity]{};
  va_list arguments;
  va_start(arguments, format);
  const int result = std::vsnprintf(buffer, sizeof(buffer), format, arguments);
  va_end(arguments);
  if (sink) sink(buffer);
  return result;
}

bool UpdaterDllLifecycleHook(std::uint32_t reason) {
  (void)reason;
  return true;
}

bool CompareStringsWithCaseOption(std::string_view candidate,
                                  std::string_view prefix,
                                  bool ignore_ascii_case) {
  if (candidate.size() < prefix.size()) return false;
  for (std::size_t i = 0; i < prefix.size(); ++i) {
    unsigned char left = static_cast<unsigned char>(candidate[i]);
    unsigned char right = static_cast<unsigned char>(prefix[i]);
    if (ignore_ascii_case) {
      left = FoldAscii(left);
      right = FoldAscii(right);
    }
    if (left != right) return false;
  }
  return true;
}

std::string NormalizeArchivePath(std::string_view path) {
  std::string normalized(path);
  std::replace(normalized.begin(), normalized.end(), '/', '\\');
  return normalized;
}

bool ZipArchiveExtractToBuffer(ArchiveReader& archive, std::string_view path,
                               std::uint8_t* output, std::size_t capacity,
                               std::size_t* extracted_size,
                               const DiagnosticSink& diagnostic) {
  if (extracted_size) *extracted_size = 0;
  if (!output && capacity != 0) return false;
  std::uint32_t index = 0;
  if (!archive.Locate(path, &index)) {
    Report(diagnostic, ArchiveError(archive, "failed to find", path));
    return false;
  }
  ArchiveEntryInfo entry;
  if (!archive.Stat(index, &entry)) {
    Report(diagnostic, ArchiveError(archive, "failed to stat", path));
    return false;
  }
  if (entry.uncompressed_size > capacity ||
      entry.uncompressed_size > std::numeric_limits<std::size_t>::max()) {
    Report(diagnostic, "archive member does not fit the destination buffer");
    return false;
  }
  const std::size_t size = static_cast<std::size_t>(entry.uncompressed_size);
  if (!archive.ExtractToBuffer(index, output, size)) {
    Report(diagnostic, ArchiveError(archive, "failed to extract", path));
    return false;
  }
  if (extracted_size) *extracted_size = size;
  return true;
}

bool ZipArchiveExtractToHeap(ArchiveReader& archive, std::string_view path,
                             std::vector<std::uint8_t>* output,
                             const DiagnosticSink& diagnostic) {
  if (!output) return false;
  std::uint32_t index = 0;
  if (!archive.Locate(path, &index)) {
    Report(diagnostic, ArchiveError(archive, "failed to find", path));
    return false;
  }
  ArchiveEntryInfo entry;
  if (!archive.Stat(index, &entry) ||
      entry.uncompressed_size > std::numeric_limits<std::size_t>::max()) {
    Report(diagnostic, ArchiveError(archive, "failed to stat", path));
    return false;
  }
  std::vector<std::uint8_t> extracted(
      static_cast<std::size_t>(entry.uncompressed_size));
  if (!archive.ExtractToBuffer(index, extracted.data(), extracted.size())) {
    Report(diagnostic, ArchiveError(archive, "failed to extract", path));
    return false;
  }
  *output = std::move(extracted);
  return true;
}

std::size_t WriteFileCallback(UpdatePlatform& platform, std::string_view path,
                              std::uint64_t offset, const std::uint8_t* data,
                              std::size_t size,
                              const DiagnosticSink& diagnostic) {
  std::size_t written = 0;
  if (!platform.WriteFileAt(path, offset, data, size, &written)) {
    Report(diagnostic, "WriteFileCallback failed to write archive data");
    return 0;
  }
  return written;
}

SoftmodData::SoftmodData() = default;

SoftmodData::~SoftmodData() { Reset(); }

void SoftmodData::Reset() {
  wrapped_archive_ = {};
  manifest_ = {};
  archive_ = nullptr;
  manifest_loaded_ = false;
}

bool SoftmodData::LoadArchive(const std::uint8_t* data, std::size_t size,
                              Sha1 sha1, ArchiveReader& archive,
                              const DiagnosticSink& diagnostic) {
  Reset();
  try {
    wrapped_archive_ = ParseUpdateArchiveWrapper(data, size, sha1);
    if (!archive.OpenMemory(wrapped_archive_.zip_data.data(),
                            wrapped_archive_.zip_data.size())) {
      Report(diagnostic, "SoftmodData::LoadArchive failed to open ZIP: " +
                             archive.LastError());
      Reset();
      return false;
    }
    archive_ = &archive;
    if (!LoadManifest(std::move(sha1), diagnostic)) {
      Reset();
      return false;
    }
    return true;
  } catch (const std::exception& error) {
    Report(diagnostic, error.what());
    Reset();
    return false;
  }
}

bool SoftmodData::LoadManifest(Sha1 sha1,
                               const DiagnosticSink& diagnostic) {
  if (!archive_) return false;
  std::vector<std::uint8_t> bytes;
  if (!ZipArchiveExtractToHeap(*archive_, "softmod.manifest", &bytes,
                               diagnostic)) {
    return false;
  }
  try {
    manifest_ = ParseSoftmodManifest(bytes.data(), bytes.size(), std::move(sha1));
    manifest_loaded_ = true;
    return true;
  } catch (const std::exception& error) {
    Report(diagnostic, error.what());
    return false;
  }
}

void SoftmodData::DecorateWithManifest(ArchiveEntryInfo* entry) const {
  if (!entry || !manifest_loaded_) return;
  const std::string normalized = NormalizeArchivePath(entry->path);
  const auto found = std::find_if(
      manifest_.entries.begin(), manifest_.entries.end(),
      [&normalized](const SoftmodManifestEntry& record) {
        return EqualAscii(normalized, record.path);
      });
  if (found == manifest_.entries.end()) return;
  entry->expected_sha1 = found->sha1;
  entry->manifest_flags = found->flags;
  entry->has_manifest_record = true;
}

bool SoftmodData::FindFile(std::string_view path, ArchiveEntryInfo* entry,
                           const DiagnosticSink& diagnostic) const {
  if (!archive_ || !entry) return false;
  std::uint32_t index = 0;
  if (!archive_->Locate(path, &index)) {
    Report(diagnostic, ArchiveError(*archive_, "failed to find", path));
    return false;
  }
  ArchiveEntryInfo result;
  if (!archive_->Stat(index, &result)) {
    Report(diagnostic, ArchiveError(*archive_, "failed to stat", path));
    return false;
  }
  result.index = index;
  DecorateWithManifest(&result);
  *entry = std::move(result);
  return true;
}

bool SoftmodData::CollectFilesByPrefix(
    std::string_view prefix, std::vector<ArchiveEntryInfo>* entries,
    const DiagnosticSink& diagnostic) const {
  if (!archive_ || !entries) return false;
  for (std::size_t i = 0; i < archive_->EntryCount(); ++i) {
    if (i > std::numeric_limits<std::uint32_t>::max()) return false;
    ArchiveEntryInfo entry;
    if (!archive_->Stat(static_cast<std::uint32_t>(i), &entry)) {
      Report(diagnostic, "failed to enumerate ZIP entry: " +
                             archive_->LastError());
      return false;
    }
    entry.index = static_cast<std::uint32_t>(i);
    if (!prefix.empty() &&
        !CompareStringsWithCaseOption(entry.path, prefix, true)) {
      continue;
    }
    DecorateWithManifest(&entry);
    entries->push_back(std::move(entry));
  }
  return true;
}

bool SoftmodData::ExtractFileToBuffer(
    const ArchiveEntryInfo& entry, std::vector<std::uint8_t>* output,
    const DiagnosticSink& diagnostic) const {
  if (!archive_ || !output ||
      entry.uncompressed_size > std::numeric_limits<std::size_t>::max()) {
    return false;
  }
  std::vector<std::uint8_t> extracted(
      static_cast<std::size_t>(entry.uncompressed_size));
  if (!archive_->ExtractToBuffer(entry.index, extracted.data(),
                                 extracted.size())) {
    Report(diagnostic, ArchiveError(*archive_, "failed to extract", entry.path));
    return false;
  }
  *output = std::move(extracted);
  return true;
}

bool SoftmodData::ExtractFileToPath(
    const ArchiveEntryInfo& entry, std::string_view destination,
    UpdatePlatform& platform, const DiagnosticSink& diagnostic) const {
  if (!archive_ || !EnsureParentDirectories(platform, destination, diagnostic) ||
      !platform.BeginFile(destination)) {
    Report(diagnostic, "failed to prepare archive destination");
    return false;
  }
  const bool extracted = archive_->ExtractToCallback(
      entry.index,
      [&platform, destination, &diagnostic](std::uint64_t offset,
                                            const std::uint8_t* data,
                                            std::size_t size) {
        return WriteFileCallback(platform, destination, offset, data, size,
                                 diagnostic);
      });
  if (!extracted) {
    Report(diagnostic, ArchiveError(*archive_, "failed to extract", entry.path));
  }
  return extracted;
}

std::uint32_t SoftmodData::SoftmodVersionTag() const {
  return (static_cast<std::uint32_t>(manifest_.softmod_major) << 24) |
         (static_cast<std::uint32_t>(manifest_.softmod_minor) << 16) |
         manifest_.softmod_revision;
}

SoftmodData& GetSoftmodDataContext() {
  static SoftmodData context;
  return context;
}

bool FileExists(const UpdatePlatform& platform, std::string_view path) {
  return platform.FileExists(path);
}

bool EnsureParentDirectories(UpdatePlatform& platform, std::string_view path,
                             const DiagnosticSink& diagnostic) {
  for (const std::string& directory : EnumerateParentDirectories(path)) {
    const DirectoryCreateResult result = platform.CreateDirectory(directory);
    if (result != DirectoryCreateResult::kCreated &&
        result != DirectoryCreateResult::kAlreadyExists) {
      Report(diagnostic, "failed to create directory path '" + directory + "'");
      return false;
    }
  }
  return true;
}

void AppendPhaseTwoPath(std::vector<std::string>* paths,
                        std::string_view path) {
  if (paths) paths->emplace_back(path);
}

bool WritePhaseTwoFileList(UpdatePlatform& platform,
                           const std::vector<std::string>& paths,
                           const DiagnosticSink& diagnostic) {
  if (paths.empty()) return true;
  std::string serialized;
  if (!SerializePhaseTwoFileList(paths, &serialized)) {
    Report(diagnostic, "phase-two path exceeds the safe line length");
    return false;
  }
  if (!platform.WriteWholeFile(
          kPhaseTwoListPath,
          reinterpret_cast<const std::uint8_t*>(serialized.data()),
          serialized.size())) {
    Report(diagnostic, "failed to write PPROOT:\\update.phase2");
    return false;
  }
  return true;
}

bool InstallHddBootstrap(SoftmodData& data, UpdatePlatform& platform,
                         Crc32 crc32, const DiagnosticSink& diagnostic) {
  ArchiveEntryInfo stage2;
  if (!data.FindFile(kStage2ArchivePath, &stage2, diagnostic) ||
      stage2.uncompressed_size == 0) {
    Report(diagnostic, "PeerPressureExploit-Stage2.bin has invalid file size");
    return false;
  }
  if (stage2.uncompressed_size > kMaximumStage2Size ||
      stage2.uncompressed_size > std::numeric_limits<std::size_t>::max()) {
    Report(diagnostic, "PeerPressureExploit-Stage2.bin is too large");
    return false;
  }
  std::vector<std::uint8_t> payload(
      static_cast<std::size_t>(stage2.uncompressed_size));
  if (!data.ExtractFileToBuffer(stage2, &payload, diagnostic) ||
      payload.size() != static_cast<std::size_t>(stage2.uncompressed_size)) {
    Report(diagnostic, "Stage2 extraction size mismatch");
    return false;
  }
  try {
    const std::vector<std::uint8_t> record = BuildHddBootstrapRecord(
        payload.data(), payload.size(), data.SoftmodVersionTag(), crc32);
    if (!platform.WritePhysicalDisk(kBootstrapDiskOffset, record.data(),
                                    record.size())) {
      Report(diagnostic, "failed to write modified Stage2 payload to HDD");
      return false;
    }
  } catch (const std::exception& error) {
    Report(diagnostic, error.what());
    return false;
  }
  return true;
}

bool RunUpdate(std::string_view archive_path, UpdatePlatform& platform,
               ArchiveReader& archive, Sha1 sha1, Crc32 crc32,
               const UpdateCallbacks& callbacks) {
  if (callbacks.progress) callbacks.progress(-1.0f, "Loading update data...");
  std::vector<std::uint8_t> archive_bytes;
  if (!platform.ReadWholeFile(archive_path, &archive_bytes)) {
    Report(callbacks.diagnostic, "failed to read update archive");
    return false;
  }

  SoftmodData& data = GetSoftmodDataContext();
  if (!data.LoadArchive(archive_bytes.data(), archive_bytes.size(), sha1,
                        archive, callbacks.diagnostic)) {
    return false;
  }

  std::vector<ArchiveEntryInfo> files;
  if (!data.CollectFilesByPrefix(kArchiveTreePrefix, &files,
                                 callbacks.diagnostic)) {
    return false;
  }
  if (callbacks.progress) callbacks.progress(-1.0f, "Updating HDD payload...");
  if (!InstallHddBootstrap(data, platform, crc32, callbacks.diagnostic)) {
    return false;
  }

  std::vector<std::string> phase_two_paths;
  for (std::size_t i = 0; i < files.size(); ++i) {
    const ArchiveEntryInfo& entry = files[i];
    std::string destination(kDestinationRoot);
    destination.append(NormalizeArchivePath(entry.path));
    if (FileExists(platform, destination) &&
        (entry.manifest_flags & kManifestPreserveExisting) != 0) {
      continue;
    }
    if ((entry.manifest_flags & kManifestPhaseTwo) != 0) {
      AppendPhaseTwoPath(&phase_two_paths, destination);
      destination.push_back('_');
    }

    if (callbacks.progress) {
      const float progress = files.empty()
                                 ? 0.0f
                                 : static_cast<float>(i) /
                                       static_cast<float>(files.size());
      callbacks.progress(progress, "Extracting: " + entry.path);
    }
    if (!EnsureParentDirectories(platform, destination,
                                 callbacks.diagnostic) ||
        !data.ExtractFileToPath(entry, destination, platform,
                                callbacks.diagnostic)) {
      return false;
    }
  }

  if (callbacks.progress) callbacks.progress(-1.0f, "Finalizing...");
  return WritePhaseTwoFileList(platform, phase_two_paths,
                               callbacks.diagnostic);
}

}  // namespace bandwagon::updater
