#pragma once

#include "release_version.hpp"

#include <cstdint>
#include <string_view>

namespace bandwagon::update_check {

inline constexpr std::string_view kTrustedRootsPath =
    "PPROOT:\\System\\IncludedRootsPEM.txt";
inline constexpr std::string_view kLatestReleaseUrl =
    "https://api.github.com/repos/grimdoomer/"
    "Xbox360PeerPressure/releases/latest";
inline constexpr std::uint32_t kHttpStatusOk = 200;
inline constexpr std::uint32_t kRecoveredGetMethod = 2;

using ServiceHandle = std::uintptr_t;

class LatestReleaseServices {
 public:
  virtual ~LatestReleaseServices() = default;

  virtual int ReadInstalledVersion(std::uint32_t* packed_version,
                                   std::uint32_t* auxiliary) = 0;
  virtual bool InitializeNetworkSecurity() = 0;
  virtual ServiceHandle CreateTrustStore() = 0;
  virtual bool LoadTrustStore(ServiceHandle trust_store,
                              std::string_view path) = 0;
  virtual ServiceHandle CreateResponse() = 0;
  virtual ServiceHandle CreateHttpClient(ServiceHandle trust_store) = 0;
  virtual int Request(ServiceHandle client, std::string_view url,
                      std::uint32_t method, ServiceHandle response) = 0;
  virtual std::uint32_t ResponseStatus(ServiceHandle response) = 0;
  virtual bool DecodeReleaseDocument(ServiceHandle response,
                                     ReleaseDocument* document) = 0;

  virtual void DestroyHttpClient(ServiceHandle client) = 0;
  virtual void DestroyResponse(ServiceHandle response) = 0;
  virtual void DestroyTrustStore(ServiceHandle trust_store) = 0;

  // The binary invokes three platform cleanup imports, each with argument 2,
  // in this order on every exit path. Their exact ordinal names are not yet
  // proven, so the portable boundary retains neutral phase names.
  virtual void ShutdownPhaseOne(std::uint32_t argument) = 0;
  virtual void ShutdownPhaseTwo(std::uint32_t argument) = 0;
  virtual void ShutdownPhaseThree(std::uint32_t argument) = 0;
};

// Readable lift of CheckLatestReleaseVersion at 0x91a0d860.
bool CheckLatestReleaseVersion(LatestReleaseServices* services,
                               std::uint32_t* packed_version);

}  // namespace bandwagon::update_check
