#include "latest_release.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

class FakeServices final
    : public bandwagon::update_check::LatestReleaseServices {
 public:
  int version_result = 0;
  std::uint32_t installed_version = 0x01000100;
  bool network_result = true;
  bool roots_result = true;
  int request_result = 0;
  std::uint32_t status = 200;
  bool decode_result = true;
  bandwagon::update_check::ReleaseDocument document{
      true, std::string("v1.0.477")};
  bool allocate_roots = true;
  bool allocate_response = true;
  bool allocate_client = true;
  std::vector<std::string> events;

  int ReadInstalledVersion(std::uint32_t* version,
                           std::uint32_t* auxiliary) override {
    events.emplace_back("read-version");
    *version = installed_version;
    *auxiliary = 0;
    return version_result;
  }
  bool InitializeNetworkSecurity() override {
    events.emplace_back("initialize");
    return network_result;
  }
  bandwagon::update_check::ServiceHandle CreateTrustStore() override {
    events.emplace_back("create-roots");
    return allocate_roots ? 1 : 0;
  }
  bool LoadTrustStore(bandwagon::update_check::ServiceHandle handle,
                      std::string_view path) override {
    assert(handle == 1 && path == bandwagon::update_check::kTrustedRootsPath);
    events.emplace_back("load-roots");
    return roots_result;
  }
  bandwagon::update_check::ServiceHandle CreateResponse() override {
    events.emplace_back("create-response");
    return allocate_response ? 2 : 0;
  }
  bandwagon::update_check::ServiceHandle CreateHttpClient(
      bandwagon::update_check::ServiceHandle roots) override {
    assert(roots == 1);
    events.emplace_back("create-client");
    return allocate_client ? 3 : 0;
  }
  int Request(bandwagon::update_check::ServiceHandle client,
              std::string_view url, std::uint32_t method,
              bandwagon::update_check::ServiceHandle response) override {
    assert(client == 3 && response == 2);
    assert(url == bandwagon::update_check::kLatestReleaseUrl);
    assert(method == bandwagon::update_check::kRecoveredGetMethod);
    events.emplace_back("request");
    return request_result;
  }
  std::uint32_t ResponseStatus(
      bandwagon::update_check::ServiceHandle response) override {
    assert(response == 2);
    events.emplace_back("status");
    return status;
  }
  bool DecodeReleaseDocument(
      bandwagon::update_check::ServiceHandle response,
      bandwagon::update_check::ReleaseDocument* output) override {
    assert(response == 2 && output);
    events.emplace_back("decode");
    *output = document;
    return decode_result;
  }
  void DestroyHttpClient(
      bandwagon::update_check::ServiceHandle client) override {
    assert(client == 3);
    events.emplace_back("destroy-client");
  }
  void DestroyResponse(
      bandwagon::update_check::ServiceHandle response) override {
    assert(response == 2);
    events.emplace_back("destroy-response");
  }
  void DestroyTrustStore(
      bandwagon::update_check::ServiceHandle roots) override {
    assert(roots == 1);
    events.emplace_back("destroy-roots");
  }
  void ShutdownPhaseOne(std::uint32_t argument) override {
    assert(argument == 2);
    events.emplace_back("shutdown-1");
  }
  void ShutdownPhaseTwo(std::uint32_t argument) override {
    assert(argument == 2);
    events.emplace_back("shutdown-2");
  }
  void ShutdownPhaseThree(std::uint32_t argument) override {
    assert(argument == 2);
    events.emplace_back("shutdown-3");
  }
};

}  // namespace

int main() {
  using bandwagon::update_check::CheckLatestReleaseVersion;

  FakeServices success;
  std::uint32_t version = 0xffffffffu;
  assert(CheckLatestReleaseVersion(&success, &version));
  assert(version == 0x010001ddu);
  const std::vector<std::string> expected{
      "read-version",     "initialize",       "create-roots",
      "load-roots",       "create-response",  "create-client",
      "request",          "status",           "decode",
      "destroy-client",   "destroy-response", "destroy-roots",
      "shutdown-1",       "shutdown-2",       "shutdown-3"};
  assert(success.events == expected);

  FakeServices current;
  current.installed_version = 0x010001ddu;
  version = 9;
  assert(!CheckLatestReleaseVersion(&current, &version));
  assert(version == 0);
  assert(current.events == expected);

  FakeServices bad_status;
  bad_status.status = 404;
  assert(!CheckLatestReleaseVersion(&bad_status, &version));
  assert((bad_status.events == std::vector<std::string>{
      "read-version", "initialize", "create-roots", "load-roots",
      "create-response", "create-client", "request", "status",
      "destroy-client", "destroy-response", "destroy-roots",
      "shutdown-1", "shutdown-2", "shutdown-3"}));

  FakeServices load_failure;
  load_failure.roots_result = false;
  assert(!CheckLatestReleaseVersion(&load_failure, &version));
  assert((load_failure.events == std::vector<std::string>{
      "read-version", "initialize", "create-roots", "load-roots",
      "destroy-roots", "shutdown-1", "shutdown-2", "shutdown-3"}));

  FakeServices early_failure;
  early_failure.version_result = 1;
  assert(!CheckLatestReleaseVersion(&early_failure, &version));
  assert((early_failure.events == std::vector<std::string>{
      "read-version", "shutdown-1", "shutdown-2", "shutdown-3"}));

  assert(!CheckLatestReleaseVersion(nullptr, &version));
  assert(!CheckLatestReleaseVersion(&success, nullptr));

  std::cout << "UpdateCheck latest-release reconstruction tests passed\n";
}
