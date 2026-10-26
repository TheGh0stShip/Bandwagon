#include "network_lifecycle.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

using bandwagon::update_check::ConnectionServices;
using bandwagon::update_check::DnsLookupResult;
using bandwagon::update_check::NetworkEventHandle;

class FakeConnectionServices final : public ConnectionServices {
 public:
  std::uint32_t start_ticks = 100;
  std::uint32_t end_ticks = 137;
  std::int32_t opened_socket = 41;
  std::uint32_t last_socket_error = 0x2736;
  std::int32_t option_result = 0;
  std::uint32_t parsed_address = 0x01020304;
  NetworkEventHandle event = 0x55;
  std::int32_t lookup_result = 0;
  DnsLookupResult dns;
  std::int32_t status_after_wait = 0;
  std::int32_t direct_connect_result = 0;
  std::vector<std::int32_t> dns_connect_results;
  std::size_t tick_calls = 0;
  std::size_t connect_calls = 0;
  std::vector<std::string> events;
  std::vector<std::pair<std::string, std::uint32_t>> integer_logs;
  std::vector<std::pair<std::string, std::string>> host_logs;

  std::uint32_t TickCountMilliseconds() override {
    events.emplace_back("ticks");
    return tick_calls++ == 0 ? start_ticks : end_ticks;
  }

  std::int32_t OpenSocket(std::uint32_t caller,
                          std::int32_t address_family,
                          std::int32_t socket_type,
                          std::int32_t protocol) override {
    assert(caller == bandwagon::update_check::kNetworkCallerSystem);
    assert(address_family == bandwagon::update_check::kIpv4AddressFamily);
    assert(socket_type == bandwagon::update_check::kStreamSocketType);
    assert(protocol == bandwagon::update_check::kTcpProtocol);
    events.emplace_back("socket");
    return opened_socket;
  }

  std::uint32_t LastSocketError() override {
    events.emplace_back("last-error");
    return last_socket_error;
  }

  std::int32_t SetSocketOption(std::uint32_t caller,
                               std::int32_t socket,
                               std::int32_t level,
                               std::int32_t option,
                               std::uint32_t value) override {
    assert(caller == bandwagon::update_check::kNetworkCallerSystem);
    assert(socket == opened_socket);
    assert(level == bandwagon::update_check::kSocketOptionLevel);
    assert(option == bandwagon::update_check::kSocketOptionName);
    assert(value == 1);
    events.emplace_back("setsockopt");
    return option_result;
  }

  std::uint32_t ParseIpv4Address(std::string_view host) override {
    assert(host == "updates.example");
    events.emplace_back("inet-addr");
    return parsed_address;
  }

  NetworkEventHandle CreateDnsEvent() override {
    events.emplace_back("create-event");
    return event;
  }

  std::int32_t StartDnsLookup(std::uint32_t caller, std::string_view host,
                              NetworkEventHandle supplied_event,
                              DnsLookupResult** result) override {
    assert(caller == bandwagon::update_check::kNetworkCallerSystem);
    assert(host == "updates.example");
    assert(supplied_event == event);
    events.emplace_back("dns-lookup");
    if (lookup_result == 0) *result = &dns;
    return lookup_result;
  }

  void WaitForDnsEvent(NetworkEventHandle supplied_event) override {
    assert(supplied_event == event);
    events.emplace_back("wait-event");
    dns.status = status_after_wait;
  }

  std::int32_t Connect(std::uint32_t caller, std::int32_t socket,
                       std::uint32_t ipv4_address,
                       std::uint16_t port) override {
    assert(caller == bandwagon::update_check::kNetworkCallerSystem);
    assert(socket == opened_socket);
    assert(port == 443);
    events.emplace_back("connect:" + std::to_string(ipv4_address));
    if (parsed_address != bandwagon::update_check::kInvalidIpv4Address) {
      assert(ipv4_address == parsed_address);
      ++connect_calls;
      return direct_connect_result;
    }
    assert(connect_calls < dns.ipv4_addresses.size());
    assert(ipv4_address == dns.ipv4_addresses[connect_calls]);
    const std::int32_t result = dns_connect_results.at(connect_calls);
    ++connect_calls;
    return result;
  }

  void CloseSocket(std::uint32_t caller, std::int32_t socket) override {
    assert(caller == bandwagon::update_check::kNetworkCallerSystem);
    assert(socket == opened_socket);
    events.emplace_back("close-socket");
  }

  void CloseDnsEvent(NetworkEventHandle supplied_event) override {
    assert(supplied_event == event);
    events.emplace_back("close-event");
  }

  void ReleaseDnsResult(std::uint32_t caller,
                        DnsLookupResult* result) override {
    assert(caller == bandwagon::update_check::kNetworkCallerSystem);
    assert(result == &dns);
    events.emplace_back("release-dns");
  }

  void ConstructTlsClient(std::string_view host) override {
    assert(host == "updates.example");
    events.emplace_back("construct-tls");
  }

  void BindTlsSocket(std::int32_t socket) override {
    assert(socket == opened_socket);
    events.emplace_back("bind-tls");
  }

  void LogInteger(std::string_view format, std::uint32_t value) override {
    events.emplace_back("log-integer");
    integer_logs.emplace_back(std::string(format), value);
  }

  void LogHost(std::string_view format, std::string_view host) override {
    events.emplace_back("log-host");
    host_logs.emplace_back(std::string(format), std::string(host));
  }
};

}  // namespace

int main() {
  using namespace bandwagon::update_check;

  HttpsClientConnectionState state;
  FakeConnectionServices numeric;
  assert(OpenHttpsConnection(&state, "updates.example", 443, &numeric) ==
         OpenConnectionResult::kSuccess);
  assert(state.socket == 41);
  assert(state.host == "updates.example");
  assert(state.tls_client_constructed && state.tls_socket_bound);
  assert((numeric.events == std::vector<std::string>{
      "ticks", "socket", "setsockopt", "inet-addr",
      "connect:16909060", "ticks", "log-integer", "construct-tls",
      "bind-tls"}));
  assert(numeric.integer_logs.size() == 1);
  assert(numeric.integer_logs[0].first == kDnsElapsedDiagnostic);
  assert(numeric.integer_logs[0].second == 37);

  HttpsClientConnectionState socket_failure_state;
  FakeConnectionServices socket_failure;
  socket_failure.opened_socket = kInvalidSocket;
  assert(OpenHttpsConnection(&socket_failure_state, "updates.example", 443,
                             &socket_failure) ==
         OpenConnectionResult::kSocketSetupFailed);
  assert(socket_failure_state.socket == kInvalidSocket);
  assert((socket_failure.events == std::vector<std::string>{
      "ticks", "socket", "last-error", "log-integer"}));
  assert(socket_failure.integer_logs[0].first == kSocketFailureDiagnostic);
  assert(socket_failure.integer_logs[0].second == 0x2736);

  HttpsClientConnectionState option_failure_state;
  FakeConnectionServices option_failure;
  option_failure.option_result = 0x2734;
  assert(OpenHttpsConnection(&option_failure_state, "updates.example", 443,
                             &option_failure) ==
         OpenConnectionResult::kSocketSetupFailed);
  assert(option_failure_state.socket == 41);
  assert((option_failure.events == std::vector<std::string>{
      "ticks", "socket", "setsockopt", "log-integer"}));
  assert(option_failure.integer_logs[0].first ==
         kSocketOptionFailureDiagnostic);
  assert(option_failure.integer_logs[0].second == 0x2734);

  HttpsClientConnectionState dns_success_state;
  FakeConnectionServices dns_success;
  dns_success.parsed_address = kInvalidIpv4Address;
  dns_success.dns.status = kDnsIoPending;
  dns_success.dns.ipv4_addresses = {0x0a000001, 0x0a000002};
  dns_success.dns_connect_results = {-1, 0};
  assert(OpenHttpsConnection(&dns_success_state, "updates.example", 443,
                             &dns_success) ==
         OpenConnectionResult::kSuccess);
  assert(dns_success_state.socket == 41);
  assert((dns_success.events == std::vector<std::string>{
      "ticks", "socket", "setsockopt", "inet-addr", "create-event",
      "dns-lookup", "wait-event", "connect:167772161",
      "connect:167772162", "ticks", "log-integer", "construct-tls",
      "bind-tls", "close-event", "release-dns"}));

  HttpsClientConnectionState lookup_failure_state;
  FakeConnectionServices lookup_failure;
  lookup_failure.parsed_address = kInvalidIpv4Address;
  lookup_failure.lookup_result = 0x2734;
  assert(OpenHttpsConnection(&lookup_failure_state, "updates.example", 443,
                             &lookup_failure) ==
         OpenConnectionResult::kDnsLookupFailed);
  assert(lookup_failure_state.socket == kInvalidSocket);
  assert((lookup_failure.events == std::vector<std::string>{
      "ticks", "socket", "setsockopt", "inet-addr", "create-event",
      "dns-lookup", "log-integer", "close-socket", "close-event"}));
  assert(lookup_failure.integer_logs[0].first == kDnsLookupFailureDiagnostic);
  assert(lookup_failure.integer_logs[0].second == 0x2734);

  HttpsClientConnectionState completion_failure_state;
  FakeConnectionServices completion_failure;
  completion_failure.parsed_address = kInvalidIpv4Address;
  completion_failure.dns.status = 5;
  assert(OpenHttpsConnection(&completion_failure_state, "updates.example",
                             443, &completion_failure) ==
         OpenConnectionResult::kDnsLookupFailed);
  assert(completion_failure.integer_logs[0].first ==
         kDnsLookupFailureDiagnostic);
  assert(completion_failure.integer_logs[0].second == 0);
  assert((completion_failure.events == std::vector<std::string>{
      "ticks", "socket", "setsockopt", "inet-addr", "create-event",
      "dns-lookup", "log-integer", "close-socket", "close-event",
      "release-dns"}));

  HttpsClientConnectionState pending_failure_state;
  FakeConnectionServices pending_failure;
  pending_failure.parsed_address = kInvalidIpv4Address;
  pending_failure.dns.status = kDnsIoPending;
  pending_failure.status_after_wait = 5;
  assert(OpenHttpsConnection(&pending_failure_state, "updates.example", 443,
                             &pending_failure) ==
         OpenConnectionResult::kDnsLookupFailed);
  assert(pending_failure.integer_logs[0].second == 0);
  assert((pending_failure.events == std::vector<std::string>{
      "ticks", "socket", "setsockopt", "inet-addr", "create-event",
      "dns-lookup", "wait-event", "log-integer", "close-socket",
      "close-event", "release-dns"}));

  HttpsClientConnectionState empty_dns_state;
  FakeConnectionServices empty_dns;
  empty_dns.parsed_address = kInvalidIpv4Address;
  assert(OpenHttpsConnection(&empty_dns_state, "updates.example", 443,
                             &empty_dns) ==
         OpenConnectionResult::kConnectFailed);
  assert(empty_dns.host_logs[0].first == kHostConnectFailureDiagnostic);
  assert(empty_dns.host_logs[0].second == "updates.example");
  assert((empty_dns.events == std::vector<std::string>{
      "ticks", "socket", "setsockopt", "inet-addr", "create-event",
      "dns-lookup", "log-host", "close-socket", "close-event",
      "release-dns"}));

  HttpsClientConnectionState exhausted_dns_state;
  FakeConnectionServices exhausted_dns;
  exhausted_dns.parsed_address = kInvalidIpv4Address;
  exhausted_dns.dns.ipv4_addresses = {0x0a000001, 0x0a000002};
  exhausted_dns.dns_connect_results = {-1, -2};
  assert(OpenHttpsConnection(&exhausted_dns_state, "updates.example", 443,
                             &exhausted_dns) ==
         OpenConnectionResult::kConnectFailed);
  assert(exhausted_dns.connect_calls == 2);
  assert(exhausted_dns.host_logs[0].first ==
         kHostConnectFailureDiagnostic);
  assert((exhausted_dns.events == std::vector<std::string>{
      "ticks", "socket", "setsockopt", "inet-addr", "create-event",
      "dns-lookup", "connect:167772161", "connect:167772162", "log-host",
      "close-socket", "close-event", "release-dns"}));

  HttpsClientConnectionState direct_failure_state;
  FakeConnectionServices direct_failure;
  direct_failure.direct_connect_result = -7;
  assert(OpenHttpsConnection(&direct_failure_state, "updates.example", 443,
                             &direct_failure) ==
         OpenConnectionResult::kConnectFailed);
  assert(direct_failure_state.socket == kInvalidSocket);
  assert(direct_failure.integer_logs[0].first == kConnectFailureDiagnostic);
  assert(direct_failure.integer_logs[0].second ==
         static_cast<std::uint32_t>(-7));
  assert((direct_failure.events == std::vector<std::string>{
      "ticks", "socket", "setsockopt", "inet-addr",
      "connect:16909060", "log-integer", "close-socket"}));

  assert(OpenHttpsConnection(nullptr, "updates.example", 443,
                             &numeric) ==
         OpenConnectionResult::kSocketSetupFailed);
  assert(OpenHttpsConnection(&state, "updates.example", 443,
                             nullptr) ==
         OpenConnectionResult::kSocketSetupFailed);

  std::cout << "UpdateCheck network-lifecycle reconstruction tests passed\n";
}
