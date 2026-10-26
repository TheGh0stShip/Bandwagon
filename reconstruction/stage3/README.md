# Stage3 source composition

Stage3 is a derivative of the public BadUpdate v1.3 payload, not 5,600 bytes
of independently authored Peer code. Its maintainable source reconstruction is
therefore composed from two auditable layers:

1. The pinned `third_party/Xbox360BadUpdate` C++ and its small naked PowerPC
   syscall wrappers provide the inherited race/exploit engine.
2. This directory provides Peer's replacement entry flow, resident-dispatcher
   capability probe, and exact copied probe payload behind testable platform
   interfaces.

The original compiler helpers and literal kernel import thunks are platform
ABI machinery. They remain in the binary-exact assembly oracle and are not
misrepresented as application C++.

`analysis/stage3/source-coverage.json` maps the entire Stage3 file, including
all application functions, wrappers, imports, literals, and alignment. The
verifier checks the Stage3 hash, pinned upstream commit and source hashes,
contiguous whole-file coverage, every source path, and every named function:

```sh
python3 tools/verify_stage3_source_coverage.py
bash tools/test_stage3_reconstruction.sh
```

Target-only operations remain explicit. The public naked syscall functions
are retained as PowerPC assembly because the ISA boundary requires them; the
two Peer dispatch calls and embedded probe have both typed C++ orchestration
and byte-exact target representations.
