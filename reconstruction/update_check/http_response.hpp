#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace bandwagon::update_check {

inline constexpr std::size_t kMaximumHttpHeaderBytes = 0x1000;

// Named wrappers for the std::vector<std::string> operations emitted beside
// the original XHttpsResponseBuffer translation unit. They express source
// intent without copying the target MSVC container implementation.
std::vector<std::string> AllocateHeaderStorage(std::size_t count);
void DestroyHeaderRange(std::vector<std::string>* headers,
                        std::size_t first, std::size_t last);
void EraseHeaderRange(std::vector<std::string>* headers,
                      std::size_t first, std::size_t last);
void DestroyHeaderVector(std::vector<std::string>* headers);
void ReserveHeaders(std::vector<std::string>* headers, std::size_t count);
void AppendHeader(std::vector<std::string>* headers, std::string header);

class HttpsResponseBuffer {
 public:
  HttpsResponseBuffer();
  ~HttpsResponseBuffer();

  bool WriteContentData(std::string_view bytes);
  bool WriteHeaderData(std::string_view bytes);
  void ParseStatusCode();
  void Reset();
  bool IsComplete() const;

  std::uint32_t status_code() const { return status_code_; }
  std::uint32_t advertised_content_length() const {
    return advertised_content_length_;
  }
  std::size_t received_content_size() const { return received_content_size_; }
  bool headers_complete() const { return headers_complete_; }
  const std::vector<std::string>& headers() const { return headers_; }
  std::string_view content() const;

 private:
  void ParseHeaderLine(std::string line);

  std::array<char, kMaximumHttpHeaderBytes> header_buffer_{};
  std::size_t header_buffer_size_ = 0;
  bool headers_complete_ = false;
  std::uint32_t status_code_ = 0;
  std::uint32_t advertised_content_length_ = 0;
  std::size_t received_content_size_ = 0;
  std::vector<std::uint8_t> content_storage_;
  std::vector<std::string> headers_;
};

}  // namespace bandwagon::update_check
