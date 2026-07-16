# Mesh Boolean Benchmarks

`MeshBooleanBenchmark` emits one tab-separated `BENCH schema=2` record per
fixture, operation, and type pair. B0 preserves the original benchmark's two
boxes and B1-B8 are deterministic exact workload families. Use `--help` for
the selectors and run one case with, for example:

- B0: original two-box smoke workload.
- B1: disjoint subdivided shells.
- B2: transversely intersecting subdivided boxes.
- B3: coplanar overlapping face grids.
- B4: repeated exact event derivations.
- B5: the square through-column annulus and increasing disjoint columns.
- B6: classification-heavy contained and disconnected shells.
- B7: disconnected components plus a coupled intersecting component.
- B8: cancellation-heavy dyadic and adjacent floating-point coordinates.

```sh
taskset -c 2 ./bin/MeshBooleanBenchmark \
  --fixture B2 --size 2 --operation union --verification mandatory \
  --type double-u32 --threads 1 --warmup 2 --repetitions 11
```

## Build

Use a Release build with strict floating point enabled. Record the compiler
name and version, linker, CMake options, commit, and whether LTO is enabled.

```sh
cmake -S . -B build-release \
  -DCMAKE_BUILD_TYPE=Release \
  -DYGOR_BOOLEAN_STRICT_FP=ON \
  -DYGOR_BUILD_BOOLEAN_TESTS=ON \
  -DYGOR_BUILD_BOOLEAN_BENCHMARKS=ON
cmake --build build-release --target MeshBooleanBenchmark
```

Do not use fast-math. The benchmark target inherits the mesh Boolean strict-FP
flags (`-fno-fast-math`, no contraction, and the compiler-specific rounding
flags) from `mesh_boolean_strict_fp`.

## Measurement

Pin the process to one otherwise idle physical CPU for single-thread runs and
to a fixed physical-core set for multi-thread runs. Disable frequency-changing
background workloads and record the CPU model, governor, core set, memory, OS,
and kernel. Do not compare pinned and unpinned runs.

Use at least two warm-ups and 11 measured repetitions for routine comparisons.
Long-running fixtures may use fewer repetitions only when the deviation and
the changed policy are reported. The benchmark measures each repetition
separately and reports the median and median absolute deviation (MAD) for wall
total, producer stages, and verifier stages. Never report a single run as a
performance result.

The schema also reports typed outcomes, semantic identities, canonical output
identity, and deterministic structural counters. Compare those fields first;
a timing comparison is invalid if any normative identity or expected counter
changed. Timing and diagnostic counters are not correctness authorities.

## Comparison

Capture complete records from the baseline and candidate builds under the same
machine, pinning, compiler, build options, fixture selectors, and environment.
Join records by `fixture`, `size`, `op`, `T`, `I`, `verification`, and `threads`.
Reject pairs whose `outcome`, input digests, stage semantic digests, canonical
identity, or frozen structural counters differ. For accepted pairs, report the
median ratio or percent change together with both MAD values and repetition
counts. Keep machine-specific timing output outside normative source tests.

The core snapshot API is consumed in `MeshBooleanPerformanceSupport.h` through
`boolean_context::performance()`. Artifact-derived counters remain as a stable
fallback and cross-check. New snapshot fields should be adapted there rather
than spread through fixture generators or baseline assertions.

The baseline test currently compares mandatory verification at one and four
threads. When a public test policy for forcing exact-kernel filters off and on
is available, add that policy matrix in `observe_performance_fixture` and keep
the same semantic-byte and deterministic-counter equality checks.
