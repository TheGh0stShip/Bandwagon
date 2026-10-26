#include "http_response.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <utility>

namespace bandwagon::update_check {
namespace {

bool CaseInsensitiveStartsWith(std::string_view value,
                               std::string_view prefix) {
  if (value.size() < prefix.size()) return false;
  for (std::size_t i = 0; i < prefix.size(); ++i) {
    const auto left = static_cast<unsigned char>(value[i]);
    const auto right = static_cast<unsigned char>(prefix[i]);
    if (std::tolower(left) != std::tolower(right)) return false;
  }
  return true;
}

}  // namespace

std::vector<std::string> AllocateHeaderStorage(std::size_t count) {
  if (count > std::numeric_limits<std::size_t>::max() / sizeof(std::string)) {
    throw std::length_error("vector<T> too long");
  }
  std::vector<std::string> headers;
  headers.reserve(count);
  return headers;
}

void DestroyHeaderRange(std::vector<std::string>* headers,
                        std::size_t first, std::size_t last) {
  if (!headers || first > last || last > headers->size()) return;
  headers->erase(headers->begin() + static_cast<std::ptrdiff_t>(first),
                 headers->begin() + static_cast<std::ptrdiff_t>(last));
}

void EraseHeaderRange(std::vector<std::string>* headers,
                      std::size_t first, std::size_t last) {
  DestroyHeaderRange(headers, first, last);
}

void DestroyHeaderVector(std::vector<std::string>* headers) {
  if (!headers) return;
  std::vector<std::string>().swap(*headers);
}

void ReserveHeaders(std::vector<std::string>* headers, std::size_t count) {
  if (!headers) return;
  headers->reserve(count);
}

void AppendHeader(std::vector<std::string>* headers, std::string header) {
  if (!headers) return;
  headers->push_back(std::move(header));
}

HttpsResponseBuffer::HttpsResponseBuffer()
    : headers_(AllocateHeaderStorage(0)) {}

HttpsResponseBuffer::~HttpsResponseBuffer() = default;

bool HttpsResponseBuffer::WriteContentData(std::string_view bytes) {
  if (bytes.empty()) return true;

  if (content_storage_.empty()) {
    if (advertised_content_length_ == 0) return false;
    content_storage_.resize(advertised_content_length_);
  }

  if (bytes.size() > advertised_content_length_ -
                         std::min<std::size_t>(received_content_size_,
                                               advertised_content_length_)) {
    return false;
  }

  std::memcpy(content_storage_.data() + received_content_size_, bytes.data(),
              bytes.size());
  received_content_size_ += bytes.size();
  return true;
}

void HttpsResponseBuffer::ParseHeaderLine(std::string line) {
  AppendHeader(&headers_, line);
  constexpr std::string_view kContentLength = "content-length:";
  if (CaseInsensitiveStartsWith(line, kContentLength)) {
    advertised_content_length_ = static_cast<std::uint32_t>(
        std::atoi(line.c_str() + kContentLength.size()));
  }
}

bool HttpsResponseBuffer::WriteHeaderData(std::string_view bytes) {
  if (headers_complete_) return WriteContentData(bytes);

  std::size_t input_offset = 0;
  while (input_offset < bytes.size() && !headers_complete_) {
    const std::size_t available =
        kMaximumHttpHeaderBytes - header_buffer_size_;
    const std::size_t copied =
        std::min(available, bytes.size() - input_offset);
    if (copied == 0) return false;

    std::memcpy(header_buffer_.data() + header_buffer_size_,
                bytes.data() + input_offset, copied);
    header_buffer_size_ += copied;
    input_offset += copied;

    std::size_t line_start = 0;
    std::size_t cursor = 0;
    while (cursor + 1 < header_buffer_size_) {
      if (header_buffer_[cursor] != '\r' ||
          header_buffer_[cursor + 1] != '\n') {
        ++cursor;
        continue;
      }

      const std::size_t line_size = cursor - line_start;
      if (line_size == 0) {
        headers_complete_ = true;
        const std::size_t content_start = cursor + 2;
        if (content_start < header_buffer_size_) {
          const std::string_view buffered_content(
              header_buffer_.data() + content_start,
              header_buffer_size_ - content_start);
          // The target ignores this first virtual write's result and returns
          // the subsequent write result (or true when no input remains).
          static_cast<void>(WriteContentData(buffered_content));
        }
        header_buffer_size_ = 0;
        break;
      }

      ParseHeaderLine(
          std::string(header_buffer_.data() + line_start, line_size));
      line_start = cursor + 2;
      cursor = line_start;
    }

    if (!headers_complete_ && line_start != 0) {
      const std::size_t remaining = header_buffer_size_ - line_start;
      std::memmove(header_buffer_.data(), header_buffer_.data() + line_start,
                   remaining);
      header_buffer_size_ = remaining;
    }
  }

  if (headers_complete_ && input_offset < bytes.size()) {
    return WriteContentData(bytes.substr(input_offset));
  }
  return true;
}

void HttpsResponseBuffer::ParseStatusCode() {
  status_code_ = 0;
  for (const std::string& header : headers_) {
    if (header.compare(0, 5, "HTTP/") != 0) continue;
    const std::size_t separator = header.find(' ');
    if (separator != std::string::npos) {
      status_code_ = static_cast<std::uint32_t>(
          std::atoi(header.c_str() + separator + 1));
    }
    return;
  }
}

void HttpsResponseBuffer::Reset() {
  content_storage_.clear();
  advertised_content_length_ = 0;
  received_content_size_ = 0;
  header_buffer_size_ = 0;
  headers_complete_ = false;
  status_code_ = 0;
  EraseHeaderRange(&headers_, 0, headers_.size());
}

bool HttpsResponseBuffer::IsComplete() const {
  return headers_complete_ &&
         received_content_size_ == advertised_content_length_;
}

std::string_view HttpsResponseBuffer::content() const {
  if (received_content_size_ == 0) return {};
  return std::string_view(
      reinterpret_cast<const char*>(content_storage_.data()),
      received_content_size_);
}

}  // namespace bandwagon::update_check
