#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace bandwagon::update_check {

// Exact constants used by XHttps::HttpsClient::OpenConnection at 0x91a0cd28.
// The imported XAM ordinals resolve to NetDll_socket (0x03),
// NetDll_closesocket (0x04), NetDll_setsockopt (0x07), NetDll_connect
// (0x0c), NetDll_XNetDnsLookup (0x43), and NetDll_XNetDnsRelease (0x44).
inline constexpr std::uint32_t kNetworkCallerSystem = 2;
inline constexpr std::int32_t kInvalidSocket = -1;
inline constexpr std::uint32_t kInvalidIpv4Address = 0xffffffffu;
inline constexpr std::int32_t kIpv4AddressFamily = 2;
inline constexpr std::int32_t kStreamSocketType = 1;
inline constexpr std::int32_t kTcpProtocol = 6;
inline constexpr std::int32_t kSocketOptionLevel = 0xffff;
inline constexpr std::int32_t kSocketOptionName = 0x5801;
inline constexpr std::int32_t kDnsIoPending = 10036;

inline constexpr std::string_view kSocketFailureDiagnostic =
    "XHttps::HttpsClient::OpenConnection: socket failed 0x%08x\n";
inline constexpr std::string_view kSocketOptionFailureDiagnostic =
    "XHttps::HttpsClient::OpenConnection: setsockopt failed 0x%08x\n";
inline constexpr std::string_view kDnsLookupFailureDiagnostic =
    "XHttps::HttpsClient::OpenConnection: XNetDnsLookup failed 0x%08x\n";
inline constexpr std::string_view kConnectFailureDiagnostic =
    "XHttps::HttpsClient::OpenConnection: connect failed 0x%08x\n";
inline constexpr std::string_view kHostConnectFailureDiagnostic =
    "XHttps::HttpsClient::OpenConnection: failed to connect to '%s'\n";
inline constexpr std::string_view kDnsElapsedDiagnostic =
    "XHttps::HttpsClient::OpenConnection: DNS time %d ms\n";

using NetworkEventHandle = std::uintptr_t;

struct DnsLookupResult {
  std::int32_t status = 0;
  std::vector<std::uint32_t> ipv4_addresses;
};

struct HttpsClientConnectionState {
  std::int32_t socket = kInvalidSocket;
  std::string host;
  bool tls_client_constructed = false;
  bool tls_socket_bound = false;
};

enum class OpenConnectionResult : std::int32_t {
  kSuccess = 0,
  kSocketSetupFailed = 1,
  kDnsLookupFailed = 2,
  kConnectFailed = 3,
};

// Portable boundary around the XAM socket calls, the Microsoft platform
// helpers at 0x91a0d9f8-0x91a0dc58, DbgPrint, and the TLS engine construction
// calls. The five helper containers are excluded only after exact normalized
// matches to a clean Microsoft 2.0.17559 image, not merely by API role.
class ConnectionServices {
 public:
  virtual ~ConnectionServices() = default;

  virtual std::uint32_t TickCountMilliseconds() = 0;
  virtual std::int32_t OpenSocket(std::uint32_t caller,
                                  std::int32_t address_family,
                                  std::int32_t socket_type,
                                  std::int32_t protocol) = 0;
  virtual std::uint32_t LastSocketError() = 0;
  virtual std::int32_t SetSocketOption(std::uint32_t caller,
                                       std::int32_t socket,
                                       std::int32_t level,
                                       std::int32_t option,
                                       std::uint32_t value) = 0;
  virtual std::uint32_t ParseIpv4Address(std::string_view host) = 0;

  virtual NetworkEventHandle CreateDnsEvent() = 0;
  virtual std::int32_t StartDnsLookup(std::uint32_t caller,
                                      std::string_view host,
                                      NetworkEventHandle event,
                                      DnsLookupResult** result) = 0;
  virtual void WaitForDnsEvent(NetworkEventHandle event) = 0;

  virtual std::int32_t Connect(std::uint32_t caller, std::int32_t socket,
                               std::uint32_t ipv4_address,
                               std::uint16_t port) = 0;
  virtual void CloseSocket(std::uint32_t caller, std::int32_t socket) = 0;
  virtual void CloseDnsEvent(NetworkEventHandle event) = 0;
  virtual void ReleaseDnsResult(std::uint32_t caller,
                                DnsLookupResult* result) = 0;

  virtual void ConstructTlsClient(std::string_view host) = 0;
  virtual void BindTlsSocket(std::int32_t socket) = 0;

  virtual void LogInteger(std::string_view format, std::uint32_t value) = 0;
  virtual void LogHost(std::string_view format, std::string_view host) = 0;
};

// Readable behavioral reconstruction of the complete 712-byte inventory entry
// at 0x91a0cd28 (SHA-256
// 4109955b6cbe16cb5f78744a6796ea7735c195730a0506098ec4beb648b341aa).
OpenConnectionResult OpenHttpsConnection(HttpsClientConnectionState* state,
                                         std::string_view host,
                                         std::uint16_t port,
                                         ConnectionServices* services);

}  // namespace bandwagon::update_check
