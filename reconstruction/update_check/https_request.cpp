#include "https_request.hpp"

#include <array>
#include <cctype>

namespace bandwagon::update_check {
namespace {

constexpr std::string_view kRequestMethod = "GET";
constexpr std::string_view kAcceptPrefix = "Accept:";
constexpr std::uint32_t kHttpRedirect = 302;

bool StartsWithCaseInsensitive(std::string_view value,
                               std::string_view prefix) {
  if (value.size() < prefix.size()) return false;
  for (std::size_t index = 0; index < prefix.size(); ++index) {
    const auto left = static_cast<unsigned char>(value[index]);
    const auto right = static_cast<unsigned char>(prefix[index]);
    if (std::tolower(left) != std::tolower(right)) return false;
  }
  return true;
}

bool HasExplicitAcceptHeader(
    const std::vector<std::string>& custom_headers) {
  for (const std::string& header : custom_headers) {
    if (header.size() >= kAcceptPrefix.size() &&
        header.compare(0, kAcceptPrefix.size(), kAcceptPrefix) == 0) {
      return true;
    }
  }
  return false;
}

bool MayFinalizeResponse(const TlsEngineSnapshot& engine) {
  if (engine.current_state != kTlsClosedState) return true;
  // The target accepts this one BR_ERR_IO state when the opaque input cursor
  // equals a five-byte TLS record header. Every other closed state, including
  // a zero engine error, skips response finalization while retaining the
  // function's zero return code.
  return engine.error == kTlsIoError &&
         engine.input_cursor == kTlsRecordHeaderBytes;
}

}  // namespace

std::string BuildHttpRequest(std::string_view method, std::string_view host,
                             std::string_view path,
                             const std::vector<std::string>& custom_headers,
                             bool keep_alive) {
  std::string request;
  request.append(method);
  request.push_back(' ');
  request.append(path);
  request.append(" HTTP/1.1\r\nHost: ");
  request.append(host);
  request.append("\r\nUser-Agent: ");
  request.append(kHttpUserAgent);
  request.append("\r\n");
  request.append(keep_alive ? "Connection: keep-alive\r\n"
                            : "Connection: close\r\n");

  for (const std::string& header : custom_headers) {
    request.append(header);
    request.append("\r\n");
  }
  if (!HasExplicitAcceptHeader(custom_headers)) {
    request.append(kDefaultAcceptHeader);
  }
  request.append("\r\n");
  return request;
}

void HttpsClient::CloseConnection() {
  services_->CloseConnection();
  connection_open_ = false;
  connected_host_.clear();
}

int HttpsClient::SubmitGetRequest(
    std::string_view url, const std::vector<std::string>& custom_headers,
    HttpsResponseBuffer* response, bool keep_alive) {
  if (!services_ || !response) return kResponseWriteFailed;

  CrackedHttpsUrl cracked;
  std::uint32_t platform_error = 0;
  if (!services_->CrackUrl(url, &cracked, &platform_error)) {
    services_->Trace(HttpsTraceEvent::kUrlCrackFailed, {}, platform_error);
    CloseConnection();
    return kUrlCrackFailed;
  }
  if (cracked.path.empty()) cracked.path = "/";

  // The target compares only the host string before reusing an existing
  // connection. It does not compare the newly cracked port.
  if (connection_open_ && connected_host_ != cracked.host) {
    CloseConnection();
  }
  if (!connection_open_) {
    const int open_status =
        services_->OpenConnection(cracked.host, cracked.port);
    if (open_status != kHttpsRequestSuccess) {
      CloseConnection();
      return open_status;
    }
    connection_open_ = true;
    connected_host_ = cracked.host;
  }

  const std::string request =
      BuildHttpRequest(kRequestMethod, cracked.host, cracked.path,
                       custom_headers, keep_alive);
  services_->Trace(HttpsTraceEvent::kRequestText, request, 0);

  const std::uint32_t started = services_->Milliseconds();
  static_cast<void>(services_->TlsWriteAll(request));
  services_->TlsFlush();

  int status = kHttpsRequestSuccess;
  std::array<char, kTlsReadBufferSize> buffer{};
  while (!response->IsComplete()) {
    const std::ptrdiff_t received =
        services_->TlsRead(buffer.data(), buffer.size());
    if (received < 0) break;
    if (static_cast<std::size_t>(received) > buffer.size() ||
        !response->WriteHeaderData(std::string_view(
            buffer.data(), static_cast<std::size_t>(received)))) {
      status = kResponseWriteFailed;
      break;
    }
  }
  const std::uint32_t elapsed = services_->Milliseconds() - started;
  services_->Trace(HttpsTraceEvent::kSocketElapsed, {}, elapsed);

  if (status != kHttpsRequestSuccess) {
    CloseConnection();
    return status;
  }

  if (MayFinalizeResponse(services_->InspectTlsEngine())) {
    response->ParseStatusCode();
    if (response->status_code() == kHttpRedirect) {
      for (const std::string& header : response->headers()) {
        if (!StartsWithCaseInsensitive(header, kLocationHeader)) continue;

        std::string redirect = header.substr(kLocationHeader.size());
        const std::size_t first_non_space = redirect.find_first_not_of(' ');
        if (first_non_space != std::string::npos) {
          redirect.erase(0, first_non_space);
        }
        response->Reset();
        return SubmitGetRequest(redirect, custom_headers, response,
                                keep_alive);
      }
      CloseConnection();
      return kRedirectLocationMissing;
    }
  }

  if (!keep_alive) CloseConnection();
  return kHttpsRequestSuccess;
}

}  // namespace bandwagon::update_check
