# Bandwagon project status

Status date: 2026-10-26

## Executive summary

Bandwagon has two distinct completion layers:

| Layer | Status | Meaning |
|---|---:|---|
| Evidence preservation | Complete for the current 33-artifact inventory | Raw payloads have whole-file evidence; XEX work preserves initialized code/data; public and platform-only artifacts have provenance records |
| Maintainable source recovery | 9 of 14 tracked targets complete | Readable source exists for the product-specific behavior, with upstream code mapped instead of rewritten |
| Replacement build/integration | Not complete | The recovered pieces are not yet a single source-built replacement distribution |

The project is not 64% complete by effort. The five partial targets include the
largest executables and most of the unresolved code. Their XEX ownership pass
is now exhaustive: 90.6% of XEX code bytes have been removed from the lifting
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
| UpdateCheck | Partial | All 804 entries partitioned; 16 of 19 product/mixed routines / 4,400 of 6,768 bytes covered | Recover the final 3 product entries / 2,368 bytes and resolve 5 unknown XDK/network wrappers / 608 bytes |
| SettingsApp | Partial | All 1,824 entries partitioned; 3 of 246 product/mixed candidates / 3,112 of 62,264 bytes have exact-entry behavioral models | Recover or reclassify the remaining 243 / 59,152 and resolve 13 XPR2/backend entries / 5,336 bytes |
| BootEdit | Partial | All 934 entries partitioned; 8 of 23 product routines / 2,424 of 13,696 bytes covered; one generic shared 376-byte container was removed from the product queue | Recover the remaining 15 / 11,272 and resolve 119 unknown entries / 31,712 bytes |

Across the five XEX ownership-pass targets, every one of 4,194 entries and
1,387,504 code bytes has an ownership assignment. The aggregate is 3,740
upstream or platform entries / 1,257,448 bytes, 317 conservative product/mixed
entries / 92,400 bytes, and 137 unknown entries / 37,656 bytes. Fifty-six
product entries / 19,608 bytes have hash-bound readable-source coverage,
leaving 261 / 72,792 bytes in the current uncovered product upper bound. This is a real
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
  adapter code; 16 / 4,400 bytes are source-covered, leaving 3 / 2,368.
- 187 entries / 76,232 bytes: stock pinned BearSSL 0.6.
- 170 entries / 43,648 bytes: pinned JsonCpp 00.11.0.
- 423 entries / 98,772 bytes: proven MSVC/Xbox platform and toolchain code.
- 5 entries / 608 bytes: XDK/network-facing wrappers still unknown and not
  excluded by role alone.

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
- 13 entries / 5,336 bytes: XPR2 resource and mixed Xbox 360/DX9 backend
  code remains unknown.

### BootEdit

- 23 entries / 13,696 bytes: product theme-editor behavior; 8 / 2,424 bytes are
  source-covered and 15 / 11,272 remain.
- 441 entries / 231,096 bytes: pinned Dear ImGui 1.86.
- 351 entries / 92,920 bytes: platform/toolchain code.
- 119 entries / 31,712 bytes: unresolved framework, lookup, and custom/mixed
  Xbox 360/DX9 backend code; not counted as product.
- The 376-byte `0x82053238` container is now unknown rather than product: it
  combines four generic/XDK-facing bodies, two shared exactly with SettingsApp,
  but no exact public/platform provenance has yet been pinned.

"Product-specific" means the code belongs to the Peer Pressure product delta.
It does not claim that Grimdoomer or any other individual personally authored
it.

## Remaining work, in order

1. Refine the 137 unknown XEX entries: 119 in BootEdit, 13 in SettingsApp, and
   5 in UpdateCheck; narrow conservative product/mixed buckets before
   lifting.
2. Recover the 50 known uncovered KernelExpansion product entries, prioritizing
   bootstrap, Xbox storage/FSD adapters, and product policy; fingerprint the 72
   unknown entries before lifting them.
3. Recover the 3,432-byte Stage2 product worker and semantically reconstruct the
   modified outer ROP/data rather than leaving it as generated assembly.
4. Recover the 261 currently uncovered XEX product/mixed candidates / 72,792
   bytes: UpdateCheck 3 / 2,368, SettingsApp 243 / 59,152, and BootEdit 15 /
   11,272. Continue narrowing SettingsApp's conservative upper bound before
   lifting generic helpers.
5. Produce target-buildable components, integrate them into a replacement
   distribution, and validate the static/host-test results against authorized
   Xbox hardware or an appropriate emulator environment.

## Working forecast

The ownership-denominator milestone is complete. The remaining planning range
is 27-55 substantive commits: roughly 2-5 for provenance refinement, 20-40 for
the product-source work across the five partial targets, and 5-10 for build and
integration closure (some work can overlap). At the present accuracy bar this
is approximately **3-7 weeks of sustained work to a source-built integration
candidate**. Authorized hardware/emulator validation is a separate external
gate and can extend that range. This forecast should narrow as the 137 XEX and
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
