#include "https_request.hpp"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace {

using bandwagon::update_check::CrackedHttpsUrl;
using bandwagon::update_check::HttpsRequestServices;
using bandwagon::update_check::HttpsTraceEvent;
using bandwagon::update_check::TlsEngineSnapshot;

struct UrlPlan {
  std::string expected_url;
  bool succeeds = true;
  CrackedHttpsUrl result;
  std::uint32_t platform_error = 0;
};

struct TraceRecord {
  HttpsTraceEvent event;
  std::string text;
  std::uint32_t value;
};

class MockServices final : public HttpsRequestServices {
 public:
  bool CrackUrl(std::string_view url, CrackedHttpsUrl* result,
                std::uint32_t* platform_error) override {
    assert(url_index < urls.size());
    const UrlPlan& plan = urls[url_index++];
    assert(url == plan.expected_url);
    *result = plan.result;
    *platform_error = plan.platform_error;
    return plan.succeeds;
  }

  int OpenConnection(std::string_view host, std::uint16_t port) override {
    opened.emplace_back(std::string(host), port);
    return open_status;
  }

  void CloseConnection() override { ++close_calls; }

  int TlsWriteAll(std::string_view bytes) override {
    requests.emplace_back(bytes);
    return write_result;
  }

  void TlsFlush() override { ++flush_calls; }

  std::ptrdiff_t TlsRead(char* destination,
                         std::size_t capacity) override {
    if (read_index == reads.size()) return -1;
    const std::string& next = reads[read_index++];
    assert(next.size() <= capacity);
    std::memcpy(destination, next.data(), next.size());
    return static_cast<std::ptrdiff_t>(next.size());
  }

  TlsEngineSnapshot InspectTlsEngine() const override { return engine; }

  std::uint32_t Milliseconds() override {
    assert(milliseconds_index < milliseconds.size());
    return milliseconds[milliseconds_index++];
  }

  void Trace(HttpsTraceEvent event, std::string_view text,
             std::uint32_t value) override {
    traces.push_back({event, std::string(text), value});
  }

  std::vector<UrlPlan> urls;
  std::size_t url_index = 0;
  int open_status = 0;
  std::vector<std::pair<std::string, std::uint16_t>> opened;
  int close_calls = 0;
  int write_result = 0;
  std::vector<std::string> requests;
  int flush_calls = 0;
  std::vector<std::string> reads;
  std::size_t read_index = 0;
  TlsEngineSnapshot engine{};
  std::vector<std::uint32_t> milliseconds;
  std::size_t milliseconds_index = 0;
  std::vector<TraceRecord> traces;
};

UrlPlan Plan(std::string url, std::string host, std::string path,
             std::uint16_t port = 443) {
  return {std::move(url), true,
          {std::move(host), std::move(path), port}, 0};
}

}  // namespace

int main() {
  using namespace bandwagon::update_check;

  const std::string request =
      BuildHttpRequest("GET", "api.github.com", "/release", {}, false);
  assert(kUrlCrackFailureDiagnostic.find("XHttpCrackUrl failed") !=
         std::string_view::npos);
  assert(kSocketElapsedDiagnostic.find("socket time") !=
         std::string_view::npos);
  assert(kRequestTextDiagnostic == "%s\n");
  assert(request ==
         "GET /release HTTP/1.1\r\n"
         "Host: api.github.com\r\n"
         "User-Agent: Xbox360XHttps/1.0\r\n"
         "Connection: close\r\n"
         "Accept: */*\r\n\r\n");

  const std::vector<std::string> explicit_accept = {
      "X-Test: yes", "Accept: application/json"};
  const std::string with_accept = BuildHttpRequest(
      "POST", "example.test", "/submit", explicit_accept, true);
  assert(with_accept ==
         "POST /submit HTTP/1.1\r\n"
         "Host: example.test\r\n"
         "User-Agent: Xbox360XHttps/1.0\r\n"
         "Connection: keep-alive\r\n"
         "X-Test: yes\r\n"
         "Accept: application/json\r\n\r\n");

  const std::string lower_accept = BuildHttpRequest(
      "GET", "example.test", "/", {"accept: text/plain"}, true);
  assert(lower_accept.find("accept: text/plain\r\nAccept: */*\r\n") !=
         std::string::npos);

  MockServices success;
  success.urls = {Plan("https://one.test", "one.test", "")};
  success.reads = {
      "HTTP/1.1 200 OK\r\nContent-Length: 2\r\n\r\nOK"};
  success.milliseconds = {100, 125};
  success.write_result = -1;  // The target deliberately ignores this result.
  HttpsClient success_client(&success);
  HttpsResponseBuffer success_response;
  assert(success_client.SubmitGetRequest("https://one.test", {},
                                         &success_response, true) == 0);
  assert(success.opened.size() == 1);
  assert(success.opened[0] ==
         std::make_pair(std::string("one.test"), std::uint16_t{443}));
  assert(success.requests.size() == 1);
  assert(success.requests[0].find("GET / HTTP/1.1\r\n") == 0);
  assert(success.flush_calls == 1 && success.close_calls == 0);
  assert(success_client.connection_open());
  assert(success_client.connected_host() == "one.test");
  assert(success_response.status_code() == 200);
  assert(success_response.content() == "OK");
  assert(success.traces.size() == 2);
  assert(success.traces[0].event == HttpsTraceEvent::kRequestText);
  assert(success.traces[1].event == HttpsTraceEvent::kSocketElapsed);
  assert(success.traces[1].value == 25);

  // The target reuses a connection solely by exact host equality, even when
  // the newly cracked port differs.
  success.urls.push_back(
      Plan("https://one.test:444/again", "one.test", "/again", 444));
  success.reads.push_back(
      "HTTP/1.1 204 Empty\r\nContent-Length: 0\r\n\r\n");
  success.milliseconds.push_back(200);
  success.milliseconds.push_back(207);
  HttpsResponseBuffer reused_response;
  assert(success_client.SubmitGetRequest("https://one.test:444/again", {},
                                         &reused_response, true) == 0);
  assert(success.opened.size() == 1);
  assert(reused_response.status_code() == 204);

  MockServices redirect;
  redirect.urls = {
      Plan("https://first.test/old", "first.test", "/old"),
      Plan("https://second.test/new", "second.test", "/new")};
  redirect.reads = {
      "HTTP/1.1 302 Found\r\n"
      "LoCaTiOn:   https://second.test/new\r\n"
      "Content-Length: 0\r\n\r\n",
      "HTTP/1.1 200 OK\r\nContent-Length: 3\r\n\r\nyes"};
  redirect.milliseconds = {10, 12, 20, 25};
  HttpsClient redirect_client(&redirect);
  HttpsResponseBuffer redirect_response;
  assert(redirect_client.SubmitGetRequest(
             "https://first.test/old", {"Accept: application/json"},
             &redirect_response, true) == 0);
  assert(redirect.opened.size() == 2);
  assert(redirect.close_calls == 1);
  assert(redirect.requests.size() == 2);
  assert(redirect.requests[1].find("GET /new HTTP/1.1\r\n") == 0);
  assert(redirect_response.status_code() == 200);
  assert(redirect_response.content() == "yes");

  MockServices no_location;
  no_location.urls = {Plan("https://lost.test", "lost.test", "/")};
  no_location.reads = {
      "HTTP/1.1 302 Found\r\nContent-Length: 0\r\n\r\n"};
  no_location.milliseconds = {0, 1};
  HttpsClient no_location_client(&no_location);
  HttpsResponseBuffer no_location_response;
  assert(no_location_client.SubmitGetRequest(
             "https://lost.test", {}, &no_location_response, true) ==
         kRedirectLocationMissing);
  assert(no_location.close_calls == 1);
  assert(!no_location_client.connection_open());

  MockServices rejected_content;
  rejected_content.urls = {Plan("https://large.test", "large.test", "/")};
  rejected_content.reads = {
      "HTTP/1.1 200 OK\r\nContent-Length: 1\r\n\r\n", "XX"};
  rejected_content.milliseconds = {30, 35};
  HttpsClient rejected_client(&rejected_content);
  HttpsResponseBuffer rejected_response;
  assert(rejected_client.SubmitGetRequest(
             "https://large.test", {}, &rejected_response, true) ==
         kResponseWriteFailed);
  assert(rejected_content.close_calls == 1);

  MockServices crack_failure;
  crack_failure.urls = {
      {"bad-url", false, {}, 0x80070057u}};
  HttpsClient crack_failure_client(&crack_failure);
  HttpsResponseBuffer crack_failure_response;
  assert(crack_failure_client.SubmitGetRequest(
             "bad-url", {}, &crack_failure_response, true) ==
         kUrlCrackFailed);
  assert(crack_failure.traces.size() == 1);
  assert(crack_failure.traces[0].event ==
         HttpsTraceEvent::kUrlCrackFailed);
  assert(crack_failure.traces[0].value == 0x80070057u);
  assert(crack_failure.close_calls == 1);

  MockServices open_failure;
  open_failure.urls = {Plan("https://offline.test", "offline.test", "/")};
  open_failure.open_status = 3;
  HttpsClient open_failure_client(&open_failure);
  HttpsResponseBuffer open_failure_response;
  assert(open_failure_client.SubmitGetRequest(
             "https://offline.test", {}, &open_failure_response, true) == 3);
  assert(open_failure.close_calls == 1);
  assert(open_failure.requests.empty());

  // A closed BearSSL engine skips status parsing but still returns zero unless
  // the BR_ERR_IO/input-cursor==5 exception applies. This includes a closed
  // engine whose error field is zero.
  MockServices closed_error;
  closed_error.urls = {Plan("https://tls.test", "tls.test", "/")};
  closed_error.milliseconds = {40, 45};
  closed_error.engine = {kTlsClosedState, 12, 0};
  HttpsClient closed_error_client(&closed_error);
  HttpsResponseBuffer closed_error_response;
  assert(closed_error_response.WriteHeaderData(
      "HTTP/1.1 206 Partial\r\nContent-Length: 1\r\n\r\n"));
  assert(closed_error_client.SubmitGetRequest(
             "https://tls.test", {}, &closed_error_response, true) == 0);
  assert(closed_error_response.status_code() == 0);

  MockServices closed_without_error;
  closed_without_error.urls = {
      Plan("https://closed.test", "closed.test", "/")};
  closed_without_error.milliseconds = {46, 49};
  closed_without_error.engine = {kTlsClosedState, 0, 0};
  HttpsClient closed_without_error_client(&closed_without_error);
  HttpsResponseBuffer closed_without_error_response;
  assert(closed_without_error_response.WriteHeaderData(
      "HTTP/1.1 205 Reset Content\r\nContent-Length: 0\r\n\r\n"));
  assert(closed_without_error_client.SubmitGetRequest(
             "https://closed.test", {}, &closed_without_error_response,
             true) == 0);
  assert(closed_without_error_response.status_code() == 0);

  MockServices tolerated_io;
  tolerated_io.urls = {Plan("https://tls.test", "tls.test", "/")};
  tolerated_io.milliseconds = {50, 55};
  tolerated_io.engine =
      {kTlsClosedState, kTlsIoError, kTlsRecordHeaderBytes};
  HttpsClient tolerated_io_client(&tolerated_io);
  HttpsResponseBuffer tolerated_io_response;
  assert(tolerated_io_response.WriteHeaderData(
      "HTTP/1.1 206 Partial\r\nContent-Length: 1\r\n\r\n"));
  assert(tolerated_io_client.SubmitGetRequest(
             "https://tls.test", {}, &tolerated_io_response, false) == 0);
  assert(tolerated_io_response.status_code() == 206);
  assert(tolerated_io.close_calls == 1);

  std::cout << "UpdateCheck HTTPS-request reconstruction tests passed\n";
}
