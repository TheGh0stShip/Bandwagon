# Bandwagon project status

Status date: 2026-10-26

## Executive summary

Bandwagon has two distinct completion layers:

| Layer | Status | Meaning |
|---|---:|---|
| Evidence preservation | Complete for the current 33-artifact inventory | Raw payloads have whole-file evidence; XEX work preserves initialized code/data; public and platform-only artifacts have provenance records |
| Maintainable source recovery | 10 of 14 tracked targets complete | Readable source exists for the product-specific behavior, with upstream code mapped instead of rewritten |
| Replacement build/integration | Not complete | The recovered pieces are not yet a single source-built replacement distribution |

The project is not 71% complete by effort. The four partial targets include the
largest executables and most of the unresolved code. Their XEX ownership pass
is now exhaustive: 90.7% of XEX code bytes have been removed from the lifting
queue as pinned upstream or proven platform/toolchain code. The remaining work
is still **dozens of material commits, not a few more commits**.

## Source-recovery scoreboard

| Target | Status | Completed | Remaining |
|---|---|---|---|
| Stage3 | Complete | Peer entry/probe reconstructed; inherited exploit routines mapped to pinned BadUpdate source | None in the current source-composition boundary |
| Stage4 | Complete | 1,764 Peer-specific bytes covered; 108-byte helper mapped to BadUpdate; 64 KiB clean-HV tail verified | None in the current source-composition boundary |
| Kernel patch stream | Complete | All 23 records have a typed effect table and exact serializer | None |
| HDD security trigger | Complete | Typed six-field structure and exact serializer | Private retail kernel pointers remain conservatively named, but no algorithm is missing |
| Flash DMA payload | Complete | Typed symbol set, semantic operation model, and exact 16 KiB builder | Two private retail globals remain conservatively named, but no algorithm is missing |
| Motherboard SMC patches | Complete | All executable replacement records for six boards have verified source coverage | Stock platform SMC firmware stays a reference boundary |
| AuroraFixer | Complete | All five product-specific routines covered; runtime/import code excluded | None |
| BootXell | Complete | All 256 entries classified; all 9 product routines / 2,944 bytes covered; 19,584 upstream bytes pinned; 52,312 platform bytes excluded | None in the current source-composition boundary |
| KernelExpansion | Partial | Exhaustive ownership for 372 entries; 46 product/local functions / 6,480 bytes currently have readable models | 50 known product entries / 18,828 bytes still need source; 72 entries / 16,024 bytes still need provenance; mixed data/types still need completion |
| HDD Stage2 | Partial | Exact whole-file reconstruction; worker split into public, platform, and candidate product ranges | 3,432 candidate product worker bytes plus the modified outer ROP and remaining data need semantic/source recovery |
| Updater | Complete | All 376 entries classified; all 20 product routines / 6,728 bytes have exact-entry readable models; 22 miniz entries are pinned and 334 platform/toolchain entries are excluded | None in the current source-composition boundary |
| UpdateCheck | Complete | All 804 entries partitioned; all 19 product/mixed routines / 6,768 bytes have readable models; five former unknown wrappers / 608 bytes positively match the clean Microsoft platform baseline | None in the current source-composition boundary |
| SettingsApp | Partial | All 1,824 entries partitioned; 3 of 246 product/mixed candidates / 3,112 of 62,264 bytes have exact-entry behavioral models; XPR2 4 / 688 source-covered | Recover or reclassify the remaining 243 / 59,152 and lift 9 / 4,648 unknown-provenance backend entries |
| BootEdit | Partial | All 30 known product entries / 15,752 bytes covered; 15 / 5,840 unknown-provenance backend/XPR2 bytes also source-covered | Resolve 72 unknown entries / 17,504 bytes; 57 / 11,664 still lack readable source |

Across the five XEX ownership-pass targets, every one of 4,194 entries and
1,387,504 code bytes has an ownership assignment. The aggregate is 3,783
upstream or platform entries / 1,270,208 bytes, 324 conservative product/mixed
entries / 94,456 bytes, and 85 unknown entries / 22,840 bytes. Eighty-one
product entries / 35,304 bytes have hash-bound readable-source coverage,
leaving 243 / 59,152 bytes in the current uncovered product upper bound. This is a real
denominator, but it can shrink as generic glue/templates are proven non-product.

## Current measured boundaries

### KernelExpansion

- 157 entries / 2,572 bytes: compiler/linker platform or toolchain.
- 47 entries / 18,728 bytes: strict upstream FatFs R0.16.
- 96 entries / 25,308 bytes: product/local adapter or policy behavior.
- 72 entries / 16,024 bytes: unknown and not counted as product-specific.
- Of 73 existing C++ lifts, 46 / 6,480 bytes count as product/local recovery;
  5 are FatFs and 22 are still unknown. The 9 local Xbox-facing helpers are
  product adapters, not platform code merely because they call platform APIs.

### HDD Stage2 worker

- 3,432 bytes: candidate product-specific.
- 4,092 bytes: public BadUpdate-derived.
- 932 bytes: compiler helpers and platform import thunks.
- An additional 9,124 bytes of ROP/data are exact public BadUpdate matches.

### Updater

- 20 entries / 6,728 bytes: product application and update policy.
- All 20 / 6,728 have exact-entry readable behavioral models and tests.
- 22 entries / 16,592 bytes: pinned upstream miniz 3.1.1.
- 334 entries / 65,320 bytes: MSVC/Xbox platform and toolchain code.
- 34 compiler-emitted STL template entries / 5,256 bytes are explicitly in
  the platform/toolchain bucket rather than the product queue.

### UpdateCheck

- 19 entries / 6,768 bytes: product/mixed update, XHttps, and certificate
  adapter code; all 19 / 6,768 bytes are source-covered.
- 187 entries / 76,232 bytes: stock pinned BearSSL 0.6.
- 170 entries / 43,648 bytes: pinned JsonCpp 00.11.0.
- 428 entries / 99,380 bytes: proven MSVC/Xbox platform and toolchain code,
  including five wrapper containers matched byte-completely to clean Microsoft
  2.0.17559 bootanim code after constrained relocation normalization.
- 0 entries / 0 bytes remain unknown.

### BootXell

- 9 entries / 2,944 bytes: product policy and two raw-DEFLATE miniz
  derivatives, all source-covered.
- 2 entries / 808 bytes: pinned upstream inih r62.
- 18 entries / 18,776 bytes: pinned upstream miniz 3.1.1.
- 227 entries / 52,312 bytes: MSVC/Xbox platform and toolchain code.

### SettingsApp

- 246 entries / 62,264 bytes: conservative product/mixed upper bound;
  individual templates and generic helpers still require review.
- 3 entries / 3,112 bytes have exact-entry readable models; 243 / 59,152 remain.
- 376 entries / 165,512 bytes: pinned Dear ImGui 1.86.
- 187 entries / 76,136 bytes: stock pinned BearSSL 0.6.
- 153 entries / 39,680 bytes: pinned JsonCpp 00.11.0.
- 17 entries / 15,848 bytes: pinned miniz 3.1.1.
- 832 entries / 263,796 bytes: platform/toolchain and import thunks.
- 13 provenance-unknown entries / 5,336 bytes: XPR2 resource and mixed Xbox
  360/DX9 backend; XPR2 4 / 688 are source-covered, leaving 9 / 4,648 without
  readable source.

### BootEdit

- 30 entries / 15,752 bytes: known product theme-editor behavior; all 30 /
  15,752 bytes are source-covered. This includes the exact typed payload,
  codec wrappers, and ordered frame-start/shutdown integration.
- 442 entries / 231,184 bytes: pinned Dear ImGui 1.86.
- 390 entries / 104,984 bytes: platform/toolchain code, including the complete
  `std::string`/`std::vector<SavedTheme>` copy/move/sort family.
- 72 entries / 17,504 bytes remain provenance-unknown; 15 / 5,840 bytes of
  the Xbox 360/DX9 backend and XPR2 loader are now source-covered, leaving
  57 / 11,664 without readable source.
- The 376-byte `0x82053238` container is now unknown rather than product: it
  combines four generic/XDK-facing bodies, two shared exactly with SettingsApp,
  but no exact public/platform provenance has yet been pinned.

"Product-specific" means the code belongs to the Peer Pressure product delta.
It does not claim that Grimdoomer or any other individual personally authored
it.

## Remaining work, in order

1. Refine the 89 unknown XEX entries: 76 in BootEdit and 13 in SettingsApp;
   narrow conservative product/mixed buckets before lifting.
2. Recover the 50 known uncovered KernelExpansion product entries, prioritizing
   bootstrap, Xbox storage/FSD adapters, and product policy; fingerprint the 72
   unknown entries before lifting them.
3. Recover the 3,432-byte Stage2 product worker and semantically reconstruct the
   modified outer ROP/data rather than leaving it as generated assembly.
4. Recover the 250 currently uncovered XEX product/mixed candidates / 66,672
   bytes: SettingsApp 243 / 59,152 and BootEdit 7 / 7,520. Continue narrowing
   SettingsApp's conservative upper bound before lifting generic helpers.
5. Produce target-buildable components, integrate them into a replacement
   distribution, and validate the static/host-test results against authorized
   Xbox hardware or an appropriate emulator environment.

## Working forecast

The ownership-denominator milestone is complete. The remaining planning range
is 24-50 substantive commits: roughly 1-4 for provenance refinement, 18-36 for
the product-source work across the four partial targets, and 5-10 for build and
integration closure (some work can overlap). At the present accuracy bar this
is approximately **3-7 weeks of sustained work to a source-built integration
candidate**. Authorized hardware/emulator validation is a separate external
gate and can extend that range. This forecast should narrow as the 126 XEX and
72 KernelExpansion unknown entries are resolved.

Run `python3 tools/report_recovery_dashboard.py` for the live hash-bound
function/byte table rather than relying on prose that may lag a refinement.

## Definition of 100%

Bandwagon is complete only when all of these are true:

1. Every executable function and initialized data range is classified as
   product-specific, exact/derived upstream, platform/toolchain, or documented
   unknown that cannot affect the replacement.
2. Every upstream range is bound to a pinned, redistributable source revision;
   every product modification to upstream code is explicit; every proprietary
   platform/toolchain exclusion has positive provenance and a documented ABI
   replacement boundary.
3. Every product-specific algorithm, policy, adapter, type, and data structure
   has readable maintainable source. Generated byte declarations alone do not
   qualify.
4. The replacement components build from source with reproducible instructions
   and without depending on original Peer Pressure binaries.
5. Host tests, byte/structure verifiers, and authorized target integration tests
   pass.

Until those gates pass, the project remains partial regardless of how much
assembly has been recovered.
