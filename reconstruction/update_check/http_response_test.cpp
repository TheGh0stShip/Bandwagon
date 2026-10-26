#include "http_response.hpp"

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

int main() {
  using bandwagon::update_check::AllocateHeaderStorage;
  using bandwagon::update_check::AppendHeader;
  using bandwagon::update_check::DestroyHeaderRange;
  using bandwagon::update_check::DestroyHeaderVector;
  using bandwagon::update_check::EraseHeaderRange;
  using bandwagon::update_check::HttpsResponseBuffer;
  using bandwagon::update_check::ReserveHeaders;

  HttpsResponseBuffer response;
  assert(!response.IsComplete());
  assert(response.WriteHeaderData(
      "HTTP/1.1 200 OK\r\nContent-Leng"));
  assert(response.WriteHeaderData(
      "th: 5\r\nX-Test: yes\r\n\r\nhe"));
  response.ParseStatusCode();
  assert(response.headers_complete());
  assert(response.status_code() == 200);
  assert(response.advertised_content_length() == 5);
  assert(response.content() == "he");
  assert(!response.IsComplete());
  assert(response.WriteHeaderData("llo"));
  assert(response.content() == "hello");
  assert(response.IsComplete());
  assert(!response.WriteContentData("!"));

  HttpsResponseBuffer mixed_case;
  assert(mixed_case.WriteHeaderData(
      "HTTP/1.0 404 Missing\r\ncOnTeNt-LeNgTh: 0\r\n\r\n"));
  mixed_case.ParseStatusCode();
  assert(mixed_case.status_code() == 404);
  assert(mixed_case.IsComplete());

  response.Reset();
  assert(response.headers().empty());
  assert(response.content().empty());
  assert(response.status_code() == 0);
  assert(!response.headers_complete());
  assert(!response.WriteContentData("body-before-length"));
  assert(response.WriteContentData({}));

  std::vector<std::string> headers = AllocateHeaderStorage(4);
  ReserveHeaders(&headers, 8);
  AppendHeader(&headers, "one");
  AppendHeader(&headers, "two");
  AppendHeader(&headers, "three");
  EraseHeaderRange(&headers, 1, 2);
  assert((headers == std::vector<std::string>{"one", "three"}));
  DestroyHeaderRange(&headers, 0, 1);
  assert((headers == std::vector<std::string>{"three"}));
  DestroyHeaderVector(&headers);
  assert(headers.empty());

  std::cout << "UpdateCheck HTTP-response reconstruction tests passed\n";
}
