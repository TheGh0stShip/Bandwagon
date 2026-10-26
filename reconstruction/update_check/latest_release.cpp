#include "latest_release.hpp"

namespace bandwagon::update_check {

bool CheckLatestReleaseVersion(LatestReleaseServices* services,
                               std::uint32_t* packed_version) {
  if (!services || !packed_version) return false;
  *packed_version = 0;

  std::uint32_t installed_version = 0;
  std::uint32_t latest_version = 0;
  std::uint32_t auxiliary = 0;
  ServiceHandle trust_store = 0;
  ServiceHandle response = 0;
  ServiceHandle client = 0;
  bool success = false;

  if (services->ReadInstalledVersion(&installed_version, &auxiliary) == 0 &&
      services->InitializeNetworkSecurity()) {
    trust_store = services->CreateTrustStore();
    if (trust_store != 0 &&
        services->LoadTrustStore(trust_store, kTrustedRootsPath)) {
      response = services->CreateResponse();
      if (response != 0) {
        client = services->CreateHttpClient(trust_store);
        if (client != 0 &&
            services->Request(client, kLatestReleaseUrl,
                              kRecoveredGetMethod, response) == 0 &&
            services->ResponseStatus(response) == kHttpStatusOk) {
          ReleaseDocument document;
          if (services->DecodeReleaseDocument(response, &document) &&
              ParseReleaseVersion(document, &latest_version) &&
              latest_version > installed_version) {
            *packed_version = latest_version;
            success = true;
          }
        }
      }
    }
  }

  if (client != 0) services->DestroyHttpClient(client);
  if (response != 0) services->DestroyResponse(response);
  if (trust_store != 0) services->DestroyTrustStore(trust_store);
  services->ShutdownPhaseOne(2);
  services->ShutdownPhaseTwo(2);
  services->ShutdownPhaseThree(2);
  return success;
}

}  // namespace bandwagon::update_check
