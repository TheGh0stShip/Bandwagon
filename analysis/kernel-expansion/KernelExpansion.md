# Kernel expansion reconstruction

`PeerPressure/System/KernelExpansion.bin` is a raw, big-endian Xbox 360 PPC
image mapped at `0x801f0000`. Its 83,792 bytes rebuild exactly from
`analysis/reconstruction/KernelExpansion.reconstructed.S` with SHA-256
`e0bea139ce9f48d0f0bd94f65fae0fd26316985f0a1b6625809893f29f028ad8`.

The base is proved by both halves of the image: signed `lis`/`addi` references
resolve strings beginning at runtime `0x801ff4b0`, while positive low-half
references resolve globals above `0x80200000`. The code and kernel-thunk region
is `0x00000-0x0f4af`; embedded strings, tables, mutable globals, and other data
occupy `0x0f4b0-0x1474f`.

## Recovered static structure

- 372 entry points from standard PPC prologues, direct `bl` targets, thirteen
  independently bounded local entries, and tail-only import thunks.
- 1,085 direct call edges.
- 141 four-instruction absolute retail-kernel/XAM thunks; 127 names are
  corroborated against the pinned public BadUpdate 17559 symbol maps.
- 46 direct references from 24 functions to embedded ASCII strings.
- 83 high-confidence behavioral labels are emitted directly into the exact
  assembly, including the config parser, file loader, XEX-section lookup,
  side-loading, boot, update, and storage-recognition paths.
- Full initialized code and data are preserved without embedding or executing
  the original binary.

The machine-readable evidence is in `KernelExpansion-functions.json`,
`KernelExpansion-callgraph.json`, `KernelExpansion-string-xrefs.json`, and the
byte-verified six-record `KernelExpansion-config-schema.json`.
`KernelExpansion-strings.txt` is the deterministic address inventory for every
four-byte-or-longer printable run in the data region; it is intentionally kept
separate from confirmed xrefs so random printable table bytes are not promoted
to strings without evidence.
Regenerate the source and function inventory with
`python tools/reconstruct_kernel_expansion.py`; the common reconstruction
verifier proves whole-file identity.

## Attribution boundary

Code being present in the closed `KernelExpansion.bin` proves component
provenance, not that every routine was uniquely authored for Peer Pressure or
by one individual. Source work is now provenance-first: known public,
Microsoft/XDK, compiler-runtime, and third-party code is mapped to its upstream
source rather than redundantly rewritten.

`KernelExpansion-source-attribution.json` begins that machine-readable split.
It already excludes the 316-byte MSVC/Xenon register save/restore islands and
all 2,256 bytes of the 141 compiler/linker-generated Microsoft/CRT import
thunks from the Peer-specific implementation target. The embedded filesystem
core is now fingerprinted as the official, unpatched FatFs R0.16 release and
its source is pinned under `third_party/fatfs-r016`. Forty-seven configured
functions / 18,728 bytes map directly to that upstream callgraph. A separate
216-byte helper is derived from the R0.16 `f_getfree` scan but changes its ABI
and removes most filesystem branches, so it remains in mixed/custom adapter
scope rather than inflating the strict upstream count. Xbox disk callbacks and
integration glue likewise remain separate targets. The remaining ranges are
not classified as Peer-specific until evidence supports that conclusion.

`KernelExpansion-fatfs-r016-map.json` records every strict FatFs function with
its exact binary range, hash, and upstream symbol.
`tools/verify_kernel_expansion_fatfs_map.py` checks those 47 mappings against
both the immutable image and the vendored official source.

`KernelExpansion-function-ownership.json` exhaustively partitions all 372
inventory entries and all 62,632 instruction bytes: 157 entries / 2,572 bytes
are proven platform or toolchain, 47 / 18,728 are strict FatFs, 96 / 25,308 are
product adapter or policy, and 72 / 16,024 remain unknown. Its verifier rejects
gaps, overlaps, split functions, range/hash drift, or aggregate-count drift.

Nine app-local helpers / 1,312 bytes were moved from the platform bucket into
the product/local-adapter boundary. They implement hook patching, Xbox
object-manager links, SATA UDMA policy, file loading, and bootstrap cleanup;
calling platform APIs does not establish platform-source provenance. All nine
already have tested readable source, so this correction does not increase the
remaining backlog: 43 known product functions / 17,948 bytes still need lifts.

The fingerprint is the unpatched base release, not current patched FatFs.
ChaN's official patch page records later R0.16 correctness and security fixes.
That version fact does not by itself prove reachability or exploitability
through Peer Pressure's Xbox adapter, but a maintainable replacement should
update or patch FatFs rather than preserve vulnerable behavior merely for byte
identity. The original bytes remain only the evidence oracle.

## Initial semantic recovery

High-confidence behaviors visible from path and format strings include loading
`Config.ini`, applying `kernel_patches.bin`, selecting BootXell and custom boot
animation paths, Aurora side-loading, parsing `DllConfig` records, locating
`UpdateData.zip`, launching `SettingsApp.xex`, recognizing FAT32/GPT media,
completing the phase-two file restore, and starting `UpdateCheck.xex`. These
labels are behavioral
descriptions, not claims that original private symbol names were recovered.

The reconstruction closes the byte/source gap for the final manifest-listed
raw Peer payload. Higher-level naming and type recovery remains an ongoing
reverse-engineering layer and is tracked separately from byte completeness.

## Maintainable source recovery

`reconstruction/kernel_expansion/runtime.{hpp,cpp}` begins the source-level
lift. The first exact-byte-bound routines are:

- `LaunchBootXell` (`0x801f09d0`, 128 bytes), represented as a typed service
  workflow that loads `PPROOT:\OtherOS\BootXell.xex`, resolves ordinal one,
  invokes it on success, and unloads every non-null returned module handle.
- `DeleteObjectManagerSymbolicLink` and
  `CreateObjectManagerSymbolicLink` (`0x801f0480` and `0x801f05f0`, 144 bytes
  together), represented as typed wrappers around the resolved kernel object
  manager calls. Tests bind the `PAYLOAD:` link and target ordering and both
  zero/nonzero status conversions.
- `CreatePeerPressureRootSymbolicLink` (`0x801f1490`, 24 bytes), the
  table-exposed fixed adapter that binds `\??\PPROOT:` to
  `\Device\Harddisk0\Partition1\PeerPressure` through the recovered object
  manager wrapper.
- `GetNandWriteProtectionEnabled` and
  `SetNandWriteProtectionDisabled` (`0x801f14a8` and `0x801f14b8`, 40
  bytes together), represented as typed access to the byte that gates four
  installed NAND write-protection hooks. SettingsApp independently describes
  the policy as preventing applications from writing to console NAND. The
  setter uses only the caller's low byte and stores its Boolean inverse; tests
  bind initial enablement, both
  states, low-byte truncation, raw-byte reads, and safe host null handling.
- `InstallSfcxNandWriteProtectionHooks`, `FilterSfcxCreate`,
  `FilterSfcxDeviceControl`, `FilterSfcxWrite`,
  `FilterSfcxSetInformation`, and `IsAllowedSfcxWritableName`
  (`0x801f1cb0`, `0x801f3758`, `0x801f3828`, `0x801f3880`,
  `0x801f38b8`, and `0x801f4f28`, 592 bytes together), represented as typed
  SFCX driver dispatches. With protection enabled, the filters reject unsafe
  create access/dispositions, write-access IOCTLs outside functions
  `0x11..0x13`, raw-volume writes, and rename/delete information classes.
  Mobile filenames and exact `extended.bin`, `Static.settings`, and
  `Statistics.settings` components remain writable. Tests bind every recovered
  policy branch, disabled-policy short circuits, noncanonical nonzero flags,
  original-status propagation, dispatch installation, case sensitivity, and
  denial without forwarding. The initial policy byte, allowlist pointers, and
  all three names are hash-bound to the immutable image.
- `QueryPeerPressureVersionsFromFlash` and
  `ResolveNandXbppRecordOffset` (`0x801f1000` and `0x801f2f98`, 480 bytes
  together), represented as a typed flash service and pure NAND-geometry
  selector. The table-exposed query publishes current version `0x010001dd`,
  opens `\Device\Flash`, reads one 512-byte record, requires big-endian
  `XBPP` at offset `0x110`, and returns the installed version at `0x114`.
  The 48-byte geometry table is independently hash-bound to the immutable
  image. Tests bind all twelve cells, unsupported geometry, exact open and read
  parameters, byte order, output initialization, status propagation, and
  handle cleanup.
- `GetPeerPressureConfig` and `InitializeDefaultConfig` (104 bytes together),
  represented with the fully recovered 0x170-byte `PeerPressureConfig` layout:
  boot-animation type and theme, dashboard path, startup-update flag, eject
  action, and OtherOS name. Tests preserve the initializer's unusual
  always-false return after it zeros the singleton, enables the startup-update
  flag, and enters the locked config path.
- `GetPeerPressureConfigValueByHash` and
  `SetPeerPressureConfigValueByHash` (`0x801f1178` and `0x801f1328`, 792
  bytes together), represented as the table-exposed configuration ABI. Both
  scan only the six cached hashes: reads take the shared lock and
  return fixed string capacities or raw integer/Boolean values; writes take the
  exclusive lock, enforce the recovered input sizes, and persist in mode one
  before unlocking. Tests bind every reachable status and lock transition,
  retention after persistence failure, and the ABI-critical fact that the
  legacy `EjectButtonBehavior` hash reaches `EjectButtonAction` while the
  current-name hash does not.
- `LoadPeerPressureConfig` (`0x801f0648`, 256 bytes), represented as the full
  configuration bootstrap: initialize the global lock, allocate the 0x170-byte
  singleton with the recovered virtual-memory parameters, probe the exact
  object-manager `Config.ini` path, then parse it through the six-entry schema.
  Tests preserve the binary's two distinct file reads and fallback behavior:
  a failed probe invokes the always-false default initializer, while a failed
  parse directly resets the singleton and sets byte `0x129` without entering
  the locked default path. The fatal allocation path and code `0x12400` are
  also explicit.
- `ReadFileToAllocatedBuffer` (`0x801f14d0`, 472 bytes), represented as a typed
  file-service workflow. It preserves pending-I/O waiting, allocation of the
  file size plus one, NUL placement, failure cleanup order, and the rule that
  caller outputs change only after a successful read.
- `LoadStructuredConfigFile` (`0x801f1700`, 144 bytes), represented as the
  generic schema-parser wrapper layered on that loader. It always releases a
  successfully loaded buffer after the parser returns.
- `ParseConfigFileBuffer` and `ParseIniDocument` (`0x801f30b0` and
  `0x801f44e0`, 400 bytes together), represented as the complete read-side INI
  pipeline. The dispatcher destructively tokenizes the loaded buffer, retains
  a bounded 127-byte section name, emits section-entry and all-null
  section-finish callbacks in target order, ignores comments and blank lines,
  and rejects malformed nonempty input. The adapter binds those events to the
  destination, six-record schema, and typed value dispatcher, and exposes the
  target's parse-failure diagnostic through a typed hook.
- `ProcessPeerPressureConfigFile` and
  `RunLockedPeerPressureConfigFile` (`0x801f3000` and `0x801f3050`, 176 bytes
  together), represented as the fixed-path/config/schema adapter and its
  exclusive-lock wrapper. Tests bind the exact `Config.ini` path, six-entry
  schema count, critical-region and lock ordering, low-byte Boolean conversion,
  and the locked wrapper's observed always-false return.
- `WriteStructuredConfigFile` and its seven INI writer helpers (`0x801f3f00`,
  `0x801f7228-0x801f7b40`, 2,880 bytes together), represented as the complete
  mode-dependent transaction behind that adapter. Mode one merges an existing
  document; other low-byte modes regenerate it. The lift replaces known keys,
  preserves unknown lines, inserts missing entries before leaving a section,
  appends missing sections in schema order, serializes all four recovered value
  types, normalizes CRLF, and models the 1 KiB buffered write path. Tests bind
  the key-only case-insensitive replacement quirk, low-byte mode selection,
  127-byte section and 511-byte line limits, ordered buffer flushes, partial
  failure cleanup, and read-failure fallback. The closed binary's unsafe use of
  preserved text as a printf format and its unterminated-comment pointer wrap
  remain in the exact assembly oracle; the maintainable host lift makes both
  paths safe and tests literal percent preservation.
- `InstallKernelXexDataHandlers` (`0x801f1790`, 344 bytes), represented as a
  bounds-checked mapped-PE walker. It finds `.xedata`, installs both relocated
  handler words, updates the table count, and performs the recovered CPU and
  hypervisor cache-maintenance sequence. The earlier `FindXexSection` label
  described only the first part of the routine and has been corrected.
- `InstallRelativeBranchHook`, `ReturnImmediatelyHook`, and
  `InstallAbsoluteBranchHook` (`0x801f03c0`, `0x801f18e8`, and `0x801f18f0`,
  416 bytes together), represented as typed instruction-patch primitives. The
  relative helper preserves the target's signed branch range, low-byte link
  test, optional raw four-byte backup, and breakpoint failure path. The
  absolute helper emits the exact adjusted-high `lis`/`addi`/`mtctr`/`bctr(l)`
  stub, including the arithmetic carry edge at destination `0xffff8000`, and
  optionally preserves all sixteen overwritten bytes. Both retain the exact
  CPU-flush, physical-translation, and hypervisor-flush order. The address-taken
  bare-return hook is separated from the preceding XEX routine and models its
  observable preservation of incoming `r3`.
- `InstallKernelPatchesAndHooks` (`0x801f0748`, 632 bytes), represented as a
  typed file, allocation, patch-memory, and hook-installation workflow. It
  binds the exact `kernel_patches.bin` path and open/query/allocation flags,
  parses big-endian address-size-data records through the standalone sentinel,
  maintains both caches after every copy, and installs the recovered XEX,
  absolute, relative, Aurora, and SFCX hooks on every normal file-failure path.
  Tests preserve the post-allocation rounded read size, pending wait, accepted
  positive completion status, fatal allocation call, open-failure no-close
  quirk, read-failure allocation leak, ignored cleanup results, and exact fixed
  hook destinations. Bounds and null guards are identified as host-safety
  additions because the target trusts its bundled stream and fatal routine.
- `ConfigureSataUltraDmaMode` (`0x801f04b8`, 176 bytes), represented as a
  typed SATA-channel service. It reads the fixed channel's 236-byte ATA
  IDENTIFY data, scans the Ultra DMA capability mask from mode seven downward,
  rejects modes below two, clamps modes six and seven to six, and requests
  transfer mode `0x40 + mode`. Only a successful request triggers the recovered
  100-microsecond stall and second IDENTIFY; that refresh status is ignored.
  Tests bind exact arguments and ordering, positive and negative failures,
  every boundary mode, highest-bit selection, clamping, and ignored refresh
  failure. The source preserves the target's uninitialized stack-buffer
  contract: a successful IDENTIFY service call owns the full output.
- `ForceDvdRegionFreeHook` (`0x801f1c60`, 80 bytes), represented as a typed
  five-argument XConfig wrapper. It forwards every argument to the original
  secured-settings query, propagates every nonzero status unchanged, and on
  success compares only the low 16 bits of category and setting. Secured
  category two / `DVDRegion` setting four receives the recovered region-free
  value `0xff`; other successful queries remain untouched. Tests bind argument
  forwarding, status propagation, high-bit truncation, exact four-byte output
  replacement, adjacent-byte preservation, and the labeled host-only null
  guard.
- `FilterBlockedXexLoadPaths` and `IsBlockedXexLoadPath` (`0x801f19c8` and
  `0x801f3218`, 280 bytes together), represented as the Peer-specific XEX-load
  policy. Case-insensitive `launch.xex` suffixes are hidden only for the exact
  DLL-load flag eight, while `Plugins\Nova.xex` is always hidden. Blocked loads
  return `STATUS_OBJECT_NAME_NOT_FOUND` without reaching the original loader;
  allowed requests preserve all four arguments and the original result. Tests
  bind exact and longer suffixes, case folding, shorter/non-suffix paths, flag
  values `0`, `8`, `9`, and `0x10008`, and no-forward behavior. This policy is
  product-specific and directly corroborates Peer Pressure's documented lack
  of DashLaunch/plugin support.
- `ReplaceInstructionWithNop` (`0x801f2d70`, 72 bytes), represented as a typed
  instruction mutation with the recovered CPU flush, physical-address
  translation, and hypervisor data-cache flush ordering.
- `ReleaseBootstrapMapping` (`0x801f2db8`, 32 bytes), represented as the fixed
  release request tail-called through the newly recovered `MiFreeMappedMemory`
  thunk. The unknown type of its second kernel argument remains explicitly
  opaque rather than being guessed.
- `RecognizeGptDisk` (`0x801fa540`, 256 bytes) is upstream FatFs R0.16
  `test_gpt_header` with its inlined CRC helper, retained as a bounds-safe
  GPT-header recognizer and the exact recovered reflected CRC-32 loop. The
  loop treats the stored checksum field as four zero bytes, matching the
  pinned upstream source; the previous self-including interpretation was
  removed.
- `IsAsciiAlphaNumeric` (`0x801fa640`, 72 bytes), a direct typed predicate for
  the binary's three accepted ASCII ranges.
- `CompletePhaseTwoFileRestore` (`0x801fa688`, 272 bytes), represented as the
  newline-delimited recovery transaction in `update.phase2`: delete each target,
  move its underscore-suffixed backup into place, and delete the marker only
  after every entry succeeds. Tests also preserve the observed success result
  when the marker is absent and the ignored final line without a newline.
- `FindCharacterOnLine` (`0x801fcfd8`, 56 bytes), the phase-two parser's local
  scanner. Its typed lift preserves LF as both a searchable character and the
  hard stop for searches targeting any other character.
- `ClassifyFatBootSector` (`0x801fa320`, 440 bytes) is upstream FatFs R0.16
  `check_fs`, retained as a typed five-result classifier. Tests cover its exact
  exFAT prefix, FAT32 label,
  structural FAT fallback, MBR signature, unknown-sector, and read-error paths.
- `QueryFatVolumeCapacity` (`0x801f9ec0`, 80 bytes), represented with a
  flattened typed view of the four nested fields it reads. Tests preserve zero
  output plus success when capacity is unavailable and the target's two
  wrapping 32-bit multiplies before the result is zero-extended to 64 bits.
- `LaunchUpdateCheck` (`0x801fa798`, 320 bytes), represented as a synchronized
  state transition around `UpdateCheck.xex`: it detaches stale NIC state,
  dereferences the prior object under the spin lock, invokes ordinal one,
  records only a newer returned version, prepares the module, and unloads it.
- Three leaf callbacks immediately following `LaunchSettingsApp` are now
  separated from that function and represented in
  `package_callbacks.{hpp,cpp}`: `InitializeCallbackOutputs` (`0x801f8210`, 40
  bytes), `MapCallbackValue` (`0x801f8238`, 56 bytes), and
  `ClassifyStfsPackageMagic` (`0x801f8270`, 104 bytes). The classifier preserves
  the exact `CON `, `LIVE`, and `PIRS` mappings, its one-byte output, the
  unchanged-output failure path, and status `0x80070570`.
- `ParseDriveNumberPrefix` (`0x801f6af0`, 120 bytes) is upstream FatFs R0.16
  `get_ldnumber`, retained as a bounded UTF-16 cursor parser for the recovered
  one-decimal-digit drive syntax. Tests
  cover valid zero and nine prefixes, absent prefixes, malformed prefixes,
  control-character termination, cursor preservation, and null strings.
- `RegisterDriveDevice` and `UnregisterDriveDevice` (`0x801f3d90` and
  `0x801f3e08`, 216 bytes together), represented as the spin-lock-protected
  ten-slot device registry used to allocate the decimal drive-prefix digit.
  Tests bind first-free selection, full-table failure as minus one, slot
  clearing, and exact acquire/release cookie flow; the host lift safely rejects
  the out-of-range clear that trusted target callers never supply.
- `IsDriveDeviceSlotEmpty` and its byte-identical compiler duplicate
  (`0x801fa2e0` and `0x801fa300`, 64 bytes together), represented as the two
  media-state predicates over that same registry. Tests preserve low-byte slot
  masking and document that the duplicate is not a distinct algorithm.
- `BindDrivePath` (`0x801f3e68`, 152 bytes) is upstream FatFs R0.16 `f_mount`,
  retained as the per-prefix binding transaction layered on
  `ParseDriveNumberPrefix`. It clears a replaced
  binding's state byte, initializes the new binding's recovered state and drive
  number bytes, honors only the low byte of the probe flag, and passes the
  original unadvanced path to the optional media probe.
- `ParseSignedDecimal` (`0x801f78e0`, 88 bytes), represented as the shared
  config-text integer parser. Tests preserve its optional-minus-only grammar,
  lack of whitespace and plus-sign handling, first-nondigit stop, and exact
  low-32-bit wraparound rather than substituting host `atoi` behavior.
- `ComputeConfigKeyHash` (`0x801f7878`, 104 bytes), represented as the
  ASCII-case-insensitive reflected CRC-32 used by schema lookup. Tests bind five
  current-key hashes plus the shipped stale `EjectButtonBehavior` hash retained
  beside the renamed `EjectButtonAction` key, as well as case folding and
  empty-input behavior.
- `ApplyStructuredConfigValue` (`0x801f41e8`, 760 bytes), represented as the
  complete typed schema dispatcher over recovered 20-byte records. A cached-key
  hash match bypasses the key string comparison; a zero, mismatched, or stale
  cache falls back to a case-insensitive key comparison before matching the
  section. This exact behavior keeps the renamed `EjectButtonAction` record
  usable. The dispatcher then applies bounded optionally quoted strings,
  wrapping signed decimals, first-character Booleans, or `0x` colors. Color
  tests cover all four normalized float channels and the option bits that retain
  selected channels in raw 0-255 form.
- `IsVersionAtLeastComponentWise` (`0x801f87f8`, 80 bytes), the exact four-byte
  bound predicate used by `ParseDllConfigEntry`. It treats `0xff` on either side
  as a wildcard and intentionally preserves the binary's component-wise rather
  than lexicographic ordering.
- `MapCreateDispositionToOpenFlags` (`0x801f92f8`, 96 bytes), represented as
  the six-case standard file-create-disposition mapping used by both open
  workflows. Tests bind the exact `8, 0, 4, 16, 0, 8` results and the recovered
  debugger break followed by zero for invalid unsigned values.
- `UnsupportedDeviceOperation` (`0x801f62c0`, 40 bytes), represented as the
  device-operation vtable's explicit unsupported stub. It preserves both the
  `DbgBreakPoint` side effect and returned `STATUS_NOT_IMPLEMENTED` value.
- `PromoteLocalSigninState` (`0x801f4d08`, 48 bytes), represented as the small
  XAM sign-in policy hook that maps local state one to online state two and
  forwards every other returned state unchanged.
- `PromoteLocalSigninInfo` (`0x801f4ed8`, 80 bytes), the companion hook that
  applies the same promotion to the recovered state field at offset `0x0c`
  only after `XamUserGetSigninInfo` succeeds, while preserving its status.
- `CopyPathAndParentDirectory` (`0x801faac0`, 144 bytes), represented with the
  recovered object field offsets and 256-byte bound. It preserves the exact
  final-backslash split, leading-backslash behavior, low-byte flag comparison,
  and the contiguous terminator write at the full-capacity boundary.
- `CanOpenExistingPath` (`0x801faa08`, 184 bytes), represented as the exact
  `NtCreateFile` option set behind the path validator. Its typed service model
  proves that only a successful open is closed and accepted.
- `DecodeOem437Character` (`0x801fda38`, 72 bytes) is upstream FatFs R0.16
  `ff_oem2uni`, retained with the full recovered 128-entry OEM-US-to-UTF-16
  table. The table exactly matches code
  page 437; tests also preserve low-16-bit code-page selection, ASCII's unusual
  full-word passthrough, and zero for unsupported or wider extended values.
- `AllocateFileSystemPool` and `FreeFileSystemPool` (`0x801fafe0` and
  `0x801fb4e8`, 24 bytes together), represented as the shared kernel-pool
  boundary used by the file-system routines. Tests bind allocation tag
  `0x66467845`, pool type two, return forwarding, and the paired free call.
- `LoadBootAnimationTheme` (`0x801f1fc0`, 152 bytes), including the exact
  version-9199 theme directory and 37-byte name precision, 41-entry structured
  decoder dispatch, version validation, and atomic publication of the complete
  632-byte theme payload only after successful validation.
- `ReleaseDriveDeviceObject` (`0x801f5110`, 80 bytes), represented as the
  complete nested-device cleanup path. It unregisters the slot stored at nested
  offset `0xc0`, then dereferences the backing object at nested offset `0xa0`
  and finally the incoming owner object, preserving the binary's call order.
- `CloseFileSystemFile` (`0x801f5b88`, 128 bytes), the filesystem close-request
  path. Under the device critical-section boundary it releases a non-null file
  context and unlinks the file object's open-file list entry, then records
  success, completes the request with no priority boost, and returns success.
- `FatDateTimeToSystemTime` and `QueryCurrentFatTimestamp` (`0x801fa1d0` and
  `0x801fbde0`, 256 bytes together), the paired FAT/Xbox time adapters. Tests
  bind every packed date/time field, the two-second resolution, zero
  milliseconds, invalid-time zeroing, and the query-before-conversion order.
- `CloneFileSystemNameNode` (`0x801faf60`, 128 bytes), including the exact
  length-derived allocation size, low-bit flag forwarding, bounded UTF-16 name
  copy, allocation-failure path, and reciprocal source/clone link writes.
- `HandOffBootPartitions` (`0x801f0568`, 136 bytes), including all eight exact
  partition-name/device-path pairs, required `Partition1` failure breakpoint,
  seven optional bindings, physical-memory release type, and loader-state
  clearing. The four underlying pointer/string ranges are hash-bound too.

Their source coverage is bound to exact function hashes by
`KernelExpansion-peer-source-coverage.json`. Run the verifier and host tests
with `tools/test_kernel_expansion_reconstruction.sh`. The current layer covers
80 independently bounded routines totaling 13,512 original instruction bytes.
Five routines / 1,040 bytes are upstream FatFs regression models and 22 /
5,112 bytes remain unattributed. The other 53 current lifts / 7,360 bytes are
classified as product adapter or policy behavior, including the nine
source-covered app-local helpers, without implying personal authorship.

This source-level layer remains partial. The exact whole-file assembly is the
evidence oracle, not a claim that all 372 discovered entries are maintainable
source. The routine formerly labeled `RecognizeFat32Volume` also identifies
exFAT and returns multiple classification values; its corrected
`ClassifyFatBootSector` lift now makes those outcomes explicit. The former
`ParsePeerPressureConfig` label was also corrected to
`InitializeDefaultConfig`: the routine is reached on a config-read failure and
establishes the default singleton before entering the locked config path. Its
caller is now also lifted, including allocation, probing, six-entry schema
dispatch, and both fallback paths. The concrete table and destination layout,
common read and write adapters, parser, lock wrapper, mode-dependent INI rewrite
transaction, key hash, typed value dispatcher, tokenizer, serializer, and
buffered output helpers are all lifted.
