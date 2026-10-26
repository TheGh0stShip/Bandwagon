#include "certificate_store.hpp"

#include <utility>

namespace bandwagon::update_check {
namespace {

std::vector<std::uint8_t> CopyBytes(ByteView bytes) {
  if (bytes.size == 0) return {};
  if (!bytes.data) return {};
  return std::vector<std::uint8_t>(bytes.data, bytes.data + bytes.size);
}

CertificateLogEvent FileFailureEvent(CertificateFileResult result) {
  switch (result) {
    case CertificateFileResult::kOpenFailed:
      return CertificateLogEvent::kOpenFailed;
    case CertificateFileResult::kAllocationFailed:
      return CertificateLogEvent::kAllocationFailed;
    case CertificateFileResult::kReadFailed:
      return CertificateLogEvent::kReadFailed;
    case CertificateFileResult::kSuccess:
      break;
  }
  return CertificateLogEvent::kReadFailed;
}

}  // namespace

CertificateStore::CertificateStore(CertificateAdapterServices* services)
    : services_(services) {}

bool CertificateStore::DecodeCertificate(ByteView der, TrustAnchor* output) {
  if (!services_ || !output || (der.size != 0 && !der.data)) return false;

  DecodedTrustAnchorView decoded;
  int decoder_error = 0;
  if (!services_->DecodeX509(der, &decoded, &decoder_error)) {
    services_->Log(CertificateLogEvent::kDecodeFailed, {},
                   static_cast<std::uint32_t>(decoder_error), 0);
    return false;
  }

  TrustAnchor candidate;
  candidate.distinguished_name = CopyBytes(decoded.distinguished_name);
  candidate.is_ca = decoded.is_ca;
  candidate.key_type = decoded.key_type;

  switch (decoded.key_type) {
    case CertificatePublicKeyType::kRsa:
      candidate.rsa_modulus = CopyBytes(decoded.rsa_modulus);
      candidate.rsa_exponent = CopyBytes(decoded.rsa_exponent);
      break;
    case CertificatePublicKeyType::kEc:
      candidate.ec_curve = decoded.ec_curve;
      candidate.ec_point = CopyBytes(decoded.ec_point);
      break;
    case CertificatePublicKeyType::kUnsupported:
      services_->Log(CertificateLogEvent::kUnsupportedPublicKey, {},
                     static_cast<std::uint32_t>(decoded.key_type), 0);
      return false;
    default:
      services_->Log(CertificateLogEvent::kUnsupportedPublicKey, {},
                     static_cast<std::uint32_t>(decoded.key_type), 0);
      return false;
  }

  *output = std::move(candidate);
  return true;
}

void CertificateStore::AppendTrustAnchor(TrustAnchor anchor) {
  anchors_.push_back(std::move(anchor));
}

bool CertificateStore::LoadCertificate(std::string_view path) {
  if (!services_) return false;

  const std::uint32_t started = services_->Milliseconds();
  std::vector<std::uint8_t> file_bytes;
  std::uint32_t error_code = 0;
  const CertificateFileResult read_result =
      services_->ReadCertificateFile(path, &file_bytes, &error_code);
  if (read_result != CertificateFileResult::kSuccess) {
    services_->Log(FileFailureEvent(read_result), path, error_code, 0);
    return false;
  }

  std::vector<PemObject> objects;
  if (!services_->DecodePem(file_bytes, &objects)) {
    services_->Log(CertificateLogEvent::kInvalidPem, path, 0, 0);
    return false;
  }

  std::uint32_t loaded = 0;
  for (const PemObject& object : objects) {
    if (object.name != "CERTIFICATE" && object.name != "X509 CERTIFICATE") {
      services_->Log(CertificateLogEvent::kUnsupportedPemObject, object.name,
                     0, 0);
      continue;
    }

    TrustAnchor anchor;
    const ByteView der{object.der.data(), object.der.size()};
    if (DecodeCertificate(der, &anchor)) {
      AppendTrustAnchor(std::move(anchor));
      ++loaded;
    }
  }

  const std::uint32_t elapsed = services_->Milliseconds() - started;
  services_->Log(CertificateLogEvent::kLoaded, path, loaded, elapsed);
  return true;
}

}  // namespace bandwagon::update_check
