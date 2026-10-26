#include "release_version.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>

int main() {
  using bandwagon::update_check::ParseReleaseVersion;
  using bandwagon::update_check::ParseReleaseTag;
  using bandwagon::update_check::ReleaseDocument;

  std::uint32_t version = 0;
  assert(ParseReleaseTag("v1.0.477", &version));
  assert(version == 0x010001ddu);

  // sscanf permits signs, decimal whitespace, and trailing input. The PPC
  // rlwimi sequence retains 8, 8, and 16 bits respectively.
  assert(ParseReleaseTag("r257.258.65537-trailing", &version));
  assert(version == 0x01020001u);
  assert(ParseReleaseTag("x-1.-2.-3", &version));
  assert(version == 0xfffefffdu);

  version = 0x12345678u;
  assert(!ParseReleaseTag("1.2.3", &version));
  assert(version == 0x12345678u);
  assert(!ParseReleaseTag("v1.2", &version));
  assert(!ParseReleaseTag("v1.2.3", nullptr));

  ReleaseDocument document{true, std::string("v2.3.4")};
  assert(ParseReleaseVersion(document, &version));
  assert(version == 0x02030004u);
  document.parse_succeeded = false;
  assert(!ParseReleaseVersion(document, &version));
  document = {true, std::nullopt};
  assert(!ParseReleaseVersion(document, &version));

  std::cout << "UpdateCheck release-version reconstruction tests passed\n";
}
