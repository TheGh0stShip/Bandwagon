#include "certificate_store.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

using bandwagon::update_check::ByteView;
using bandwagon::update_check::CertificateAdapterServices;
using bandwagon::update_check::CertificateFileResult;
using bandwagon::update_check::CertificateLogEvent;
using bandwagon::update_check::CertificatePublicKeyType;
using bandwagon::update_check::DecodedTrustAnchorView;
using bandwagon::update_check::PemObject;

struct LogRecord {
  CertificateLogEvent event;
  std::string detail;
  std::uint32_t value;
  std::uint32_t auxiliary;
};

class FakeCertificateServices final : public CertificateAdapterServices {
 public:
  CertificateFileResult file_result = CertificateFileResult::kSuccess;
  bool pem_result = true;
  std::uint32_t now = 100;
  std::vector<PemObject> pem_objects;
  std::vector<LogRecord> logs;
  std::vector<std::uint8_t> distinguished_name{0x30, 0x01, 0x02};
  std::vector<std::uint8_t> modulus{0xaa, 0xbb};
  std::vector<std::uint8_t> exponent{0x01, 0x00, 0x01};
  std::vector<std::uint8_t> point{0x04, 0x10, 0x20};

  std::uint32_t Milliseconds() override {
    const std::uint32_t result = now;
    now += 25;
    return result;
  }

  CertificateFileResult ReadCertificateFile(
      std::string_view path, std::vector<std::uint8_t>* bytes,
      std::uint32_t* error_code) override {
    assert(path == "roots.pem");
    *error_code = 0x80070002u;
    *bytes = {1, 2, 3};
    return file_result;
  }

  bool DecodePem(const std::vector<std::uint8_t>& bytes,
                 std::vector<PemObject>* objects) override {
    assert((bytes == std::vector<std::uint8_t>{1, 2, 3}));
    *objects = pem_objects;
    return pem_result;
  }

  bool DecodeX509(ByteView der, DecodedTrustAnchorView* decoded,
                  int* decoder_error) override {
    assert(der.size == 1);
    const std::uint8_t selector = der.data[0];
    if (selector == 4) {
      *decoder_error = 37;
      return false;
    }
    decoded->distinguished_name =
        {distinguished_name.data(), distinguished_name.size()};
    decoded->is_ca = true;
    if (selector == 1) {
      decoded->key_type = CertificatePublicKeyType::kRsa;
      decoded->rsa_modulus = {modulus.data(), modulus.size()};
      decoded->rsa_exponent = {exponent.data(), exponent.size()};
    } else if (selector == 2) {
      decoded->key_type = CertificatePublicKeyType::kEc;
      decoded->ec_curve = 23;
      decoded->ec_point = {point.data(), point.size()};
    } else {
      decoded->key_type = CertificatePublicKeyType::kUnsupported;
    }
    return true;
  }

  void Log(CertificateLogEvent event, std::string_view detail,
           std::uint32_t value, std::uint32_t auxiliary) override {
    logs.push_back({event, std::string(detail), value, auxiliary});
  }
};

}  // namespace

int main() {
  using bandwagon::update_check::CertificateStore;
  using bandwagon::update_check::TrustAnchor;

  FakeCertificateServices services;
  CertificateStore store(&services);

  const std::vector<std::uint8_t> rsa_der{1};
  TrustAnchor rsa;
  assert(store.DecodeCertificate({rsa_der.data(), rsa_der.size()}, &rsa));
  assert(rsa.is_ca);
  assert(rsa.key_type == CertificatePublicKeyType::kRsa);
  assert((rsa.distinguished_name ==
          std::vector<std::uint8_t>{0x30, 0x01, 0x02}));
  assert((rsa.rsa_modulus == std::vector<std::uint8_t>{0xaa, 0xbb}));
  services.modulus[0] = 0;
  assert(rsa.rsa_modulus[0] == 0xaa);  // The adapter owns a deep copy.

  const std::vector<std::uint8_t> unsupported_der{3};
  TrustAnchor unsupported;
  assert(!store.DecodeCertificate(
      {unsupported_der.data(), unsupported_der.size()}, &unsupported));
  assert(services.logs.back().event ==
         CertificateLogEvent::kUnsupportedPublicKey);

  services.logs.clear();
  services.modulus[0] = 0xaa;
  services.pem_objects = {
      {"PRIVATE KEY", {1}},
      {"CERTIFICATE", {1}},
      {"CERTIFICATE", {4}},
      {"X509 CERTIFICATE", {2}},
  };
  assert(store.LoadCertificate("roots.pem"));
  assert(store.anchors().size() == 2);
  assert(store.anchors()[0].key_type == CertificatePublicKeyType::kRsa);
  assert(store.anchors()[1].key_type == CertificatePublicKeyType::kEc);
  assert(store.anchors()[1].ec_curve == 23);
  assert(services.logs.front().event ==
         CertificateLogEvent::kUnsupportedPemObject);
  assert(services.logs[1].event == CertificateLogEvent::kDecodeFailed);
  assert(services.logs.back().event == CertificateLogEvent::kLoaded);
  assert(services.logs.back().value == 2);
  assert(services.logs.back().auxiliary == 25);

  FakeCertificateServices bad_pem;
  bad_pem.pem_result = false;
  CertificateStore bad_pem_store(&bad_pem);
  assert(!bad_pem_store.LoadCertificate("roots.pem"));
  assert(bad_pem.logs.back().event == CertificateLogEvent::kInvalidPem);

  FakeCertificateServices missing;
  missing.file_result = CertificateFileResult::kOpenFailed;
  CertificateStore missing_store(&missing);
  assert(!missing_store.LoadCertificate("roots.pem"));
  assert(missing.logs.back().event == CertificateLogEvent::kOpenFailed);
  assert(missing.logs.back().value == 0x80070002u);

  std::cout << "UpdateCheck certificate-store reconstruction tests passed\n";
}
