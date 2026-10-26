#pragma once

#include "http_response.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace bandwagon::update_check {

inline constexpr std::string_view kHttpUserAgent = "Xbox360XHttps/1.0";
inline constexpr std::string_view kDefaultAcceptHeader = "Accept: */*\r\n";
inline constexpr std::string_view kLocationHeader = "location:";
inline constexpr std::string_view kUrlCrackFailureDiagnostic =
    "XHttps::HttpsClient::SubmitGETRequest: XHttpCrackUrl failed 0x%08x\n";
inline constexpr std::string_view kSocketElapsedDiagnostic =
    "XHttps::HttpsClient::SubmitGETRequest: socket time %d ms\n";
inline constexpr std::string_view kRequestTextDiagnostic = "%s\n";
inline constexpr std::size_t kTlsReadBufferSize = 0x200;

inline constexpr int kHttpsRequestSuccess = 0;
inline constexpr int kUrlCrackFailed = 4;
inline constexpr int kRedirectLocationMissing = 6;
inline constexpr int kResponseWriteFailed = 7;

inline constexpr std::uint32_t kTlsClosedState = 1;
inline constexpr std::int32_t kTlsIoError = 31;
inline constexpr std::uint32_t kTlsRecordHeaderBytes = 5;

enum class HttpsTraceEvent {
  kUrlCrackFailed,
  kRequestText,
  kSocketElapsed,
};

struct CrackedHttpsUrl {
  std::string host;
  std::string path;
  std::uint16_t port = 0;
};

// The target reads these three BearSSL engine values after the receive loop.
// input_cursor is the engine's opaque 32-bit field at offset 0x20; the name
// deliberately avoids claiming a more specific ABI meaning than the evidence.
struct TlsEngineSnapshot {
  std::uint32_t current_state = 0;
  std::int32_t error = 0;
  std::uint32_t input_cursor = 0;
};

class HttpsRequestServices {
 public:
  virtual ~HttpsRequestServices() = default;

  // Models NetDll_XHttpCrackUrl with flags=2 and its immediate last-error
  // capture. A false result leaves platform_error available for diagnostics.
  virtual bool CrackUrl(std::string_view url, CrackedHttpsUrl* result,
                        std::uint32_t* platform_error) = 0;

  // OpenConnection is the separately recovered 0x91a0cd28 boundary. Its
  // numeric status is returned unchanged by SubmitGetRequest.
  virtual int OpenConnection(std::string_view host, std::uint16_t port) = 0;
  virtual void CloseConnection() = 0;

  virtual int TlsWriteAll(std::string_view bytes) = 0;
  virtual void TlsFlush() = 0;
  virtual std::ptrdiff_t TlsRead(char* destination,
                                 std::size_t capacity) = 0;
  virtual TlsEngineSnapshot InspectTlsEngine() const = 0;
  virtual std::uint32_t Milliseconds() = 0;
  virtual void Trace(HttpsTraceEvent event, std::string_view text,
                     std::uint32_t value) = 0;
};

// Readable reconstruction of the exact request construction at 0x91a0d1e8.
// Custom headers are appended verbatim with CRLF. The target's seven-byte
// Accept: probe is case-sensitive; this model intentionally preserves it.
std::string BuildHttpRequest(std::string_view method, std::string_view host,
                             std::string_view path,
                             const std::vector<std::string>& custom_headers,
                             bool keep_alive);

class HttpsClient {
 public:
  explicit HttpsClient(HttpsRequestServices* services) : services_(services) {}

  // Readable reconstruction of XHttps::HttpsClient::SubmitGETRequest at
  // 0x91a0d328. Redirect recursion is intentionally unbounded, as in the
  // target; callers control the URL source and transport service.
  int SubmitGetRequest(std::string_view url,
                       const std::vector<std::string>& custom_headers,
                       HttpsResponseBuffer* response, bool keep_alive);

  bool connection_open() const { return connection_open_; }
  std::string_view connected_host() const { return connected_host_; }

 private:
  void CloseConnection();

  HttpsRequestServices* services_ = nullptr;
  bool connection_open_ = false;
  std::string connected_host_;
};

}  // namespace bandwagon::update_check
