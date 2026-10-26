# Bundled 9199 boot animation provenance

`PeerPressure/Themes/BootAnim/9199/bootanim.xex` is distributed inside the Peer
Pressure archive, but its own authenticated executable metadata does not
identify it as Peer-authored code:

- original image name: `bootanim.dll`;
- linker timestamp: 2010-02-18 02:21:38 UTC;
- execution version/base version: `0x2023ef00`;
- Xbox kernel and C runtime version: `2.0.9199.0`;
- D3D/XAudio/XAPI versions: `2.0.2099.x`;
- no Peer Pressure strings or 2026 build metadata; and
- image SHA-256:
  `d978e0fa36eb14b08bbd163338a009906435f9f4e92db11654897737d4545fed`.

This is therefore classified as a bundled Xbox dashboard 9199 component, not
closed Peer Pressure source. The public project records its provenance and
hash but does not translate or republish the Microsoft binary as source. That
boundary does not exclude Peer-authored theme selection/install logic in other
executables, which remains in reverse-engineering scope.

No byte-identity claim against an independently sourced stock 9199 image is
made here because such an image is not present in the research inputs. The
authorship conclusion rests on internal build/version evidence; an independent
hash match can strengthen it later without changing the current copyright
boundary.
