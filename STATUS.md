# Bandwagon project status

Status date: 2026-10-26

## Executive summary

Bandwagon has two distinct completion layers:

| Layer | Status | Meaning |
|---|---:|---|
| Evidence preservation | Complete for the current 33-artifact inventory | Raw payloads have whole-file evidence; XEX work preserves initialized code/data; public and platform-only artifacts have provenance records |
| Maintainable source recovery | 7 of 14 tracked targets complete | Readable source exists for the product-specific behavior, with upstream code mapped instead of rewritten |
| Replacement build/integration | Not complete | The recovered pieces are not yet a single source-built replacement distribution |

The project is not 50% complete by effort. The seven partial targets include the
largest executables and most of the unresolved code. A defensible calendar ETA
is not possible until their ownership partitions are complete. The honest scale
is **dozens of material commits, not a few more commits**.

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
| KernelExpansion | Partial | Exhaustive ownership for 372 entries; 37 product functions / 5,168 bytes currently have readable models | 50 known product entries / 18,828 bytes still need source; 72 entries / 16,024 bytes still need provenance; mixed data/types still need completion |
| HDD Stage2 | Partial | Exact whole-file reconstruction; worker split into public, platform, and candidate product ranges | 3,432 candidate product worker bytes plus the modified outer ROP and remaining data need semantic/source recovery |
| Updater | Partial | Exact initialized sections; broad application/miniz/runtime ownership ranges; core archive/update structures partly recovered | Function-level ownership across 376 entries, then source recovery of the product application slice |
| BootXell | Partial | Exact initialized sections; inih r62 pinned; 7 product routines covered | Whole-image ownership across 256 entries and remaining product logic |
| UpdateCheck | Partial | Exact initialized sections; BearSSL/JsonCpp pinned; 2 product routines covered | Whole-image ownership across 804 entries and remaining product HTTP/update glue |
| SettingsApp | Partial | Exact initialized sections; ImGui/BearSSL/JsonCpp pinned; initial semantic map | Ownership across 1,824 entries and readable source for product UI/settings behavior |
| BootEdit | Partial | Exact initialized sections; ImGui pinned; initial theme-workflow map | Ownership across 934 entries and readable source for product theme-editing behavior |

The five partial XEX applications contain 4,194 discovered function entries in
total. Most will be compiler, platform, or pinned library code and should be
excluded—not rewritten—but the ownership pass is still required to establish
the true closed-source denominator.

## Current measured boundaries

### KernelExpansion

- 166 entries / 3,884 bytes: platform or toolchain.
- 47 entries / 18,728 bytes: strict upstream FatFs R0.16.
- 87 entries / 23,996 bytes: product adapter or policy behavior.
- 72 entries / 16,024 bytes: unknown and not counted as product-specific.
- Of 73 existing C++ lifts, only 37 / 5,168 bytes currently count as product
  recovery; 5 are FatFs, 9 are generic platform helpers, and 22 are unknown.

### HDD Stage2 worker

- 3,432 bytes: candidate product-specific.
- 4,092 bytes: public BadUpdate-derived.
- 932 bytes: compiler helpers and platform import thunks.
- An additional 9,124 bytes of ROP/data are exact public BadUpdate matches.

"Product-specific" means the code belongs to the Peer Pressure product delta.
It does not claim that Grimdoomer or any other individual personally authored
it.

## Remaining work, in order

1. Finish provenance-first ownership partitions for Updater, BootXell,
   UpdateCheck, SettingsApp, and BootEdit. This removes public libraries and
   platform code from the reconstruction queue and produces the real remaining
   denominator.
2. Recover the 50 known uncovered KernelExpansion product entries, prioritizing
   bootstrap, Xbox storage/FSD adapters, and product policy; fingerprint the 72
   unknown entries before lifting them.
3. Recover the 3,432-byte Stage2 product worker and semantically reconstruct the
   modified outer ROP/data rather than leaving it as generated assembly.
4. Recover the product-specific application slices revealed by the five XEX
   ownership partitions.
5. Produce target-buildable components, integrate them into a replacement
   distribution, and validate the static/host-test results against authorized
   Xbox hardware or an appropriate emulator environment.

## Definition of 100%

Bandwagon is complete only when all of these are true:

1. Every executable function and initialized data range is classified as
   product-specific, exact/derived upstream, platform/toolchain, or documented
   unknown that cannot affect the replacement.
2. Every upstream range is bound to a pinned, redistributable source revision;
   every product modification to upstream code is explicit.
3. Every product-specific algorithm, policy, adapter, type, and data structure
   has readable maintainable source. Generated byte declarations alone do not
   qualify.
4. The replacement components build from source with reproducible instructions
   and without depending on original Peer Pressure binaries.
5. Host tests, byte/structure verifiers, and authorized target integration tests
   pass.

Until those gates pass, the project remains partial regardless of how much
assembly has been recovered.
