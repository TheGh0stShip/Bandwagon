#pragma once

#include "archive_wrapper.hpp"
#include "bootstrap_record.hpp"
#include "softmod_manifest.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace bandwagon::updater {

constexpr std::uint32_t kManifestPreserveExisting = 1u << 0;
constexpr std::uint32_t kManifestPhaseTwo = 1u << 1;
constexpr std::string_view kArchiveTreePrefix = "PeerPressure/";
constexpr std::string_view kDestinationRoot = "PPROOT:\\";
constexpr std::string_view kPhaseTwoListPath = "PPROOT:\\update.phase2";
constexpr std::string_view kStage2ArchivePath =
    "Hdd/PeerPressureExploit-Stage2.bin";

using DiagnosticSink = std::function<void(std::string_view)>;
using ProgressSink = std::function<void(float, std::string_view)>;
using ArchiveChunkSink =
    std::function<std::size_t(std::uint64_t, const std::uint8_t*, std::size_t)>;

struct ArchiveEntryInfo {
  std::uint32_t index = 0;
  std::uint64_t uncompressed_size = 0;
  std::string path;
  Sha1Digest expected_sha1{};
  std::uint32_t manifest_flags = 0;
  bool has_manifest_record = false;
};

// Portable seam around the pinned miniz reader. The Xbox build can implement
// this interface directly with mz_zip_reader_* without duplicating miniz.
class ArchiveReader {
 public:
  virtual ~ArchiveReader() = default;
  virtual bool OpenMemory(const std::uint8_t* data, std::size_t size) = 0;
  virtual std::size_t EntryCount() const = 0;
  virtual bool Locate(std::string_view path, std::uint32_t* index) const = 0;
  virtual bool Stat(std::uint32_t index, ArchiveEntryInfo* entry) const = 0;
  virtual bool ExtractToBuffer(std::uint32_t index, std::uint8_t* output,
                               std::size_t size) = 0;
  virtual bool ExtractToCallback(std::uint32_t index,
                                 const ArchiveChunkSink& sink) = 0;
  virtual std::string LastError() const = 0;
};

enum class DirectoryCreateResult {
  kCreated,
  kAlreadyExists,
  kError,
};

// All state-changing Xbox operations are injected. Host tests therefore cover
// update policy without opening a real disk or touching the host filesystem.
class UpdatePlatform {
 public:
  virtual ~UpdatePlatform() = default;
  virtual bool ReadWholeFile(std::string_view path,
                             std::vector<std::uint8_t>* output) = 0;
  virtual bool FileExists(std::string_view path) const = 0;
  virtual DirectoryCreateResult CreateDirectory(std::string_view path) = 0;
  virtual bool BeginFile(std::string_view path) = 0;
  virtual bool WriteFileAt(std::string_view path, std::uint64_t offset,
                           const std::uint8_t* data, std::size_t size,
                           std::size_t* written) = 0;
  virtual bool WriteWholeFile(std::string_view path, const std::uint8_t* data,
                              std::size_t size) = 0;
  virtual bool WritePhysicalDisk(std::uint64_t offset,
                                 const std::uint8_t* data,
                                 std::size_t size) = 0;
};

int FormatDiagnosticAndDispatch(const DiagnosticSink& sink,
                                const char* format, ...);
bool UpdaterDllLifecycleHook(std::uint32_t reason);

bool CompareStringsWithCaseOption(std::string_view candidate,
                                  std::string_view prefix,
                                  bool ignore_ascii_case);
std::string NormalizeArchivePath(std::string_view path);

bool ZipArchiveExtractToBuffer(ArchiveReader& archive, std::string_view path,
                               std::uint8_t* output, std::size_t capacity,
                               std::size_t* extracted_size,
                               const DiagnosticSink& diagnostic = {});
bool ZipArchiveExtractToHeap(ArchiveReader& archive, std::string_view path,
                             std::vector<std::uint8_t>* output,
                             const DiagnosticSink& diagnostic = {});
std::size_t WriteFileCallback(UpdatePlatform& platform, std::string_view path,
                              std::uint64_t offset, const std::uint8_t* data,
                              std::size_t size,
                              const DiagnosticSink& diagnostic = {});

class SoftmodData {
 public:
  SoftmodData();
  ~SoftmodData();
  SoftmodData(const SoftmodData&) = delete;
  SoftmodData& operator=(const SoftmodData&) = delete;

  void Reset();
  bool LoadArchive(const std::uint8_t* data, std::size_t size, Sha1 sha1,
                   ArchiveReader& archive,
                   const DiagnosticSink& diagnostic = {});
  bool LoadManifest(Sha1 sha1, const DiagnosticSink& diagnostic = {});
  bool FindFile(std::string_view path, ArchiveEntryInfo* entry,
                const DiagnosticSink& diagnostic = {}) const;
  bool CollectFilesByPrefix(std::string_view prefix,
                            std::vector<ArchiveEntryInfo>* entries,
                            const DiagnosticSink& diagnostic = {}) const;
  bool ExtractFileToBuffer(const ArchiveEntryInfo& entry,
                           std::vector<std::uint8_t>* output,
                           const DiagnosticSink& diagnostic = {}) const;
  bool ExtractFileToPath(const ArchiveEntryInfo& entry,
                         std::string_view destination, UpdatePlatform& platform,
                         const DiagnosticSink& diagnostic = {}) const;

  std::uint32_t SoftmodVersionTag() const;
  const SoftmodManifest& manifest() const { return manifest_; }

 private:
  void DecorateWithManifest(ArchiveEntryInfo* entry) const;

  WrappedUpdateArchive wrapped_archive_;
  SoftmodManifest manifest_;
  ArchiveReader* archive_ = nullptr;
  bool manifest_loaded_ = false;
};

SoftmodData& GetSoftmodDataContext();

bool FileExists(const UpdatePlatform& platform, std::string_view path);
bool EnsureParentDirectories(UpdatePlatform& platform, std::string_view path,
                             const DiagnosticSink& diagnostic = {});
void AppendPhaseTwoPath(std::vector<std::string>* paths,
                        std::string_view path);
bool WritePhaseTwoFileList(UpdatePlatform& platform,
                           const std::vector<std::string>& paths,
                           const DiagnosticSink& diagnostic = {});
bool InstallHddBootstrap(SoftmodData& data, UpdatePlatform& platform,
                         Crc32 crc32,
                         const DiagnosticSink& diagnostic = {});

struct UpdateCallbacks {
  DiagnosticSink diagnostic;
  ProgressSink progress;
};

bool RunUpdate(std::string_view archive_path, UpdatePlatform& platform,
               ArchiveReader& archive, Sha1 sha1, Crc32 crc32,
               const UpdateCallbacks& callbacks = {});

}  // namespace bandwagon::updater
