#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace bandwagon::update_check {

struct ByteView {
  const std::uint8_t* data = nullptr;
  std::size_t size = 0;
};

enum class CertificatePublicKeyType : std::uint8_t {
  kRsa = 1,
  kEc = 2,
  kUnsupported = 0xff,
};

// Borrowed view returned by the stock BearSSL X.509 decoder boundary.
struct DecodedTrustAnchorView {
  ByteView distinguished_name;
  bool is_ca = false;
  CertificatePublicKeyType key_type = CertificatePublicKeyType::kUnsupported;
  ByteView rsa_modulus;
  ByteView rsa_exponent;
  std::uint32_t ec_curve = 0;
  ByteView ec_point;
};

// Owned C++ form of the BearSSL br_x509_trust_anchor consumed by XHttps.
struct TrustAnchor {
  std::vector<std::uint8_t> distinguished_name;
  bool is_ca = false;
  CertificatePublicKeyType key_type = CertificatePublicKeyType::kUnsupported;
  std::vector<std::uint8_t> rsa_modulus;
  std::vector<std::uint8_t> rsa_exponent;
  std::uint32_t ec_curve = 0;
  std::vector<std::uint8_t> ec_point;
};

struct PemObject {
  std::string name;
  std::vector<std::uint8_t> der;
};

enum class CertificateFileResult {
  kSuccess,
  kOpenFailed,
  kAllocationFailed,
  kReadFailed,
};

enum class CertificateLogEvent {
  kOpenFailed,
  kAllocationFailed,
  kReadFailed,
  kInvalidPem,
  kUnsupportedPemObject,
  kDecodeFailed,
  kUnsupportedPublicKey,
  kLoaded,
};

// BearSSL PEM/X.509 parsing and Xbox file I/O remain explicit dependencies;
// this interface prevents the product adapter from relifting either body.
class CertificateAdapterServices {
 public:
  virtual ~CertificateAdapterServices() = default;

  virtual std::uint32_t Milliseconds() = 0;
  virtual CertificateFileResult ReadCertificateFile(
      std::string_view path, std::vector<std::uint8_t>* bytes,
      std::uint32_t* error_code) = 0;
  virtual bool DecodePem(const std::vector<std::uint8_t>& bytes,
                         std::vector<PemObject>* objects) = 0;
  virtual bool DecodeX509(ByteView der, DecodedTrustAnchorView* decoded,
                          int* decoder_error) = 0;
  virtual void Log(CertificateLogEvent event, std::string_view detail,
                   std::uint32_t value, std::uint32_t auxiliary) = 0;
};

class CertificateStore {
 public:
  explicit CertificateStore(CertificateAdapterServices* services);

  // Readable models of the three product/mixed adapter entries at
  // 0x91a0fcb8, 0x91a0ff00, and 0x91a0ffd8 respectively.
  bool DecodeCertificate(ByteView der, TrustAnchor* output);
  void AppendTrustAnchor(TrustAnchor anchor);
  bool LoadCertificate(std::string_view path);

  const std::vector<TrustAnchor>& anchors() const { return anchors_; }

 private:
  CertificateAdapterServices* services_;
  std::vector<TrustAnchor> anchors_;
};

}  // namespace bandwagon::update_check
