#include "namespace_links.hpp"

#include <cassert>
#include <iostream>
#include <string_view>

namespace {

class FakeLinks final
    : public bandwagon::kernel_expansion::SymbolicLinkServices {
 public:
  int delete_status = 0;
  int create_status = 0;
  bool deleted = false;
  bool created = false;

  int Delete(std::string_view link_name) override {
    assert(link_name == "\\??\\PAYLOAD:");
    deleted = true;
    return delete_status;
  }

  int Create(std::string_view link_name,
             std::string_view target_name) override {
    assert(link_name == "\\??\\PAYLOAD:");
    assert(target_name == "\\System??\\PAYLOAD:");
    created = true;
    return create_status;
  }
};

}  // namespace

int main() {
  using namespace bandwagon::kernel_expansion;

  FakeLinks success;
  assert(DeleteObjectManagerSymbolicLink(&success, "\\??\\PAYLOAD:"));
  assert(success.deleted);
  assert(CreateObjectManagerSymbolicLink(
      &success, "\\??\\PAYLOAD:", "\\System??\\PAYLOAD:"));
  assert(success.created);

  FakeLinks failure;
  failure.delete_status = -1;
  failure.create_status = -2;
  assert(!DeleteObjectManagerSymbolicLink(&failure, "\\??\\PAYLOAD:"));
  assert(!CreateObjectManagerSymbolicLink(
      &failure, "\\??\\PAYLOAD:", "\\System??\\PAYLOAD:"));
  assert(failure.deleted && failure.created);

  class PeerRootLinks final : public SymbolicLinkServices {
   public:
    int status = 0;
    int Delete(std::string_view) override { return -1; }
    int Create(std::string_view link_name,
               std::string_view target_name) override {
      assert(link_name == kPeerPressureRootLink);
      assert(target_name == kPeerPressureRootTarget);
      return status;
    }
  } peer_root;
  assert(CreatePeerPressureRootSymbolicLink(&peer_root));
  peer_root.status = -1;
  assert(!CreatePeerPressureRootSymbolicLink(&peer_root));

  assert(!DeleteObjectManagerSymbolicLink(nullptr, "\\??\\PAYLOAD:"));
  assert(!CreateObjectManagerSymbolicLink(
      nullptr, "\\??\\PAYLOAD:", "\\System??\\PAYLOAD:"));
  assert(!CreatePeerPressureRootSymbolicLink(nullptr));

  std::cout << "Kernel Expansion namespace-link tests passed\n";
}
