#include "network_lifecycle.hpp"

namespace bandwagon::update_check {
namespace {

void FinishDnsAttempt(ConnectionServices* services,
                      NetworkEventHandle event,
                      DnsLookupResult* dns_result) {
  // The recovered function closes its event before releasing the XNDNS block.
  if (event != 0) services->CloseDnsEvent(event);
  if (dns_result != nullptr) {
    services->ReleaseDnsResult(kNetworkCallerSystem, dns_result);
  }
}

void CloseFailedConnection(HttpsClientConnectionState* state,
                           ConnectionServices* services,
                           NetworkEventHandle event,
                           DnsLookupResult* dns_result) {
  if (state->socket != kInvalidSocket) {
    services->CloseSocket(kNetworkCallerSystem, state->socket);
    state->socket = kInvalidSocket;
  }
  FinishDnsAttempt(services, event, dns_result);
}

}  // namespace

OpenConnectionResult OpenHttpsConnection(HttpsClientConnectionState* state,
                                         std::string_view host,
                                         std::uint16_t port,
                                         ConnectionServices* services) {
  if (!state || !services) return OpenConnectionResult::kSocketSetupFailed;

  DnsLookupResult* dns_result = nullptr;
  NetworkEventHandle dns_event = 0;
  const std::uint32_t start_ticks = services->TickCountMilliseconds();

  state->socket =
      services->OpenSocket(kNetworkCallerSystem, kIpv4AddressFamily,
                           kStreamSocketType, kTcpProtocol);
  if (state->socket == kInvalidSocket) {
    services->LogInteger(kSocketFailureDiagnostic,
                         services->LastSocketError());
    CloseFailedConnection(state, services, dns_event, dns_result);
    return OpenConnectionResult::kSocketSetupFailed;
  }

  const std::int32_t option_result = services->SetSocketOption(
      kNetworkCallerSystem, state->socket, kSocketOptionLevel,
      kSocketOptionName, 1);
  if (option_result != 0) {
    services->LogInteger(kSocketOptionFailureDiagnostic,
                         static_cast<std::uint32_t>(option_result));
    // The target returns directly on this branch. It deliberately leaves the
    // newly opened socket in the client object for its normal destructor.
    return OpenConnectionResult::kSocketSetupFailed;
  }

  std::uint32_t ipv4_address = services->ParseIpv4Address(host);
  if (ipv4_address == kInvalidIpv4Address) {
    dns_event = services->CreateDnsEvent();
    const std::int32_t lookup_result = services->StartDnsLookup(
        kNetworkCallerSystem, host, dns_event, &dns_result);
    if (lookup_result != 0) {
      services->LogInteger(kDnsLookupFailureDiagnostic,
                           static_cast<std::uint32_t>(lookup_result));
      CloseFailedConnection(state, services, dns_event, dns_result);
      return OpenConnectionResult::kDnsLookupFailed;
    }

    // A zero API result contractually supplies XNDNS. The null check is the
    // portable model's only hardening beyond the target's direct dereference.
    if (dns_result == nullptr) {
      services->LogInteger(kDnsLookupFailureDiagnostic, 0);
      CloseFailedConnection(state, services, dns_event, dns_result);
      return OpenConnectionResult::kDnsLookupFailed;
    }

    if (dns_result->status == kDnsIoPending) {
      services->WaitForDnsEvent(dns_event);
    }
    if (dns_result->status != 0) {
      // The machine code passes a literal zero to this diagnostic after an
      // asynchronous completion error; preserve that observable quirk.
      services->LogInteger(kDnsLookupFailureDiagnostic, 0);
      CloseFailedConnection(state, services, dns_event, dns_result);
      return OpenConnectionResult::kDnsLookupFailed;
    }

    bool connected = false;
    for (const std::uint32_t candidate : dns_result->ipv4_addresses) {
      if (services->Connect(kNetworkCallerSystem, state->socket, candidate,
                            port) == 0) {
        ipv4_address = candidate;
        connected = true;
        break;
      }
    }
    if (!connected) {
      services->LogHost(kHostConnectFailureDiagnostic, host);
      CloseFailedConnection(state, services, dns_event, dns_result);
      return OpenConnectionResult::kConnectFailed;
    }
  } else {
    const std::int32_t connect_result = services->Connect(
        kNetworkCallerSystem, state->socket, ipv4_address, port);
    if (connect_result != 0) {
      services->LogInteger(kConnectFailureDiagnostic,
                           static_cast<std::uint32_t>(connect_result));
      CloseFailedConnection(state, services, dns_event, dns_result);
      return OpenConnectionResult::kConnectFailed;
    }
  }

  const std::uint32_t elapsed =
      services->TickCountMilliseconds() - start_ticks;
  services->LogInteger(kDnsElapsedDiagnostic, elapsed);
  services->ConstructTlsClient(host);
  state->tls_client_constructed = true;
  services->BindTlsSocket(state->socket);
  state->tls_socket_bound = true;
  state->host.assign(host.begin(), host.end());

  FinishDnsAttempt(services, dns_event, dns_result);
  return OpenConnectionResult::kSuccess;
}

}  // namespace bandwagon::update_check
