# Mesh Boolean Qualification Report (P6.11)

This is the reproducible qualification report assembled by P6.11. It is the
only record that may justify a `qualified_default` promotion, and it currently
promotes nothing. It is bound to the repository state recorded below and must be
re-read together with `docs/MeshBooleanQualification.md` (schemas),
`docs/MeshBooleanP610CandidateAssessment.md` (retained-campaign rejection),
`docs/MeshBooleanP610ManualCampaign.md` (campaign driver and P6.11 review
procedure), and `docs/MeshBooleanBetaTesting.md` (end-user failure-reporting
boundary).

## Decision

- **Decision: `candidate`** (no profile is `qualified`).
- **Claim scope: none.** No backend, result mode, preparation policy, workload
  profile, compiler, standard library, or architecture is production-qualified.
- The in-tree `experimental_exact_v1` backend remains `experimental`. The
  `independent_axis_aligned_box_v1` adapter remains a diagnostic-only comparator.
  `qualified_default` selection continues to fail closed.

## Repository and commands

- Reviewed branch: `boolean_symbolic`.
- Reported commit: `e78c38d` (the completed available-toolchain campaign
  evidence is bound to this commit; see the evidence directory `campaign.tsv`).
- Dirty state: the campaign evidence below was generated from a clean checkout;
  `git-status.txt` and `git-diff.patch` in the evidence directory record the
  proof. No qualification claim may be made from a dirty tree.
- Build: C++17, self-contained (no external geometry dependency). See
  `CMakeLists.txt` and `compile_and_install.sh`. Qualification tests build under
  `YGOR_BUILD_BOOLEAN_TESTS=ON`.

## Platform matrix

- The frozen P6.8 matrix (current/oldest GCC and Clang, Debug/Release,
  libstdc++/libc++, x86-64/AArch64, sanitizers, strict floating-point, 32/64-bit
  indices) is defined in `YgorMeshesBooleanQualificationMatrix.h` and
  `docs/MeshBooleanQualificationMatrix.md`.
- **The retained controlled campaign did not execute this matrix.** The
  retained evidence `p610-b8427a7a70dc-b9fb5f16437d-x86_64` reported 55 required
  steps, 18 passes, 14 failures, 23 blockers, 16 unresolved anomalies, and
  `campaign_status=incomplete_blocking`; its own checksum manifest could not be
  verified after log removal. See `docs/MeshBooleanP610CandidateAssessment.md`.
- The `--available-toolchain` limited campaign was then executed with driver
  schema v5 over the seven available profiles (current GCC Debug/Release/
  ASan+UBSan/libstdc++-debug and current Clang Debug/Release/ASan+UBSan) after
  resolving the campaign findings below. That run finalized
  `campaign_status=complete_limited_toolchain` at commit `e78c38d` with all 33
  required steps passed, 25 known-limitation steps, zero failed/blocked steps,
  zero unresolved anomalies, all 72 non-deferred manifest cases
  `verified_exact_success`, and every one of the six runnable fuzz allocations
  at or above the 600 aggregate CPU-second floor. It is candidate evidence only;
  the unavailable frozen matrix dimensions (oldest-supported compilers, libc++,
  ThreadSanitizer, AArch64) remain documented known limitations, and end-user
  beta testing still carries the residual validation burden before any profile
  closure.
- Consequently there is no committed passing full-matrix outcome, and no
  platform profile may be promoted.

### Available-toolchain campaign findings and resolutions

The limited campaign surfaced six distinct findings; each is resolved in-tree
and recorded here as an anomaly-plus-resolution rather than left blocking:

1. **B0-B8 output digest drift.** `MeshBoolean.PerformanceBaselines` `B0`-`B8`
   failed `frozen canonical output identity` while every structural counter and
   canonical byte count stayed identical. Root cause: the P2.1/P5.2
   `selected_exact_boundary` schema bump (3→4) and preparation-provenance fields
   intentionally changed the selection→realization→output digest chain. Resolved
   as an intended schema change by re-freezing the B0-B8 digests after review
   (commit `ec6977e`); the test passes under both GCC and Clang.
2. **Clang 7.0.1 destructor ABI mismatch.** Clang libstdc++ profiles failed to
   link with `undefined reference to exact_point3::~exact_point3()` (a D1/D2
   complete-vs-base-object destructor symbol split). Resolved by declaring
   out-of-line defaulted destructors for the exact arithmetic/kernel value
   types (commit `ec6977e`).
3. **Instrumented-build resource pressure.** `--jobs 32` OOM-killed the
   ASan+UBSan and libstdc++-debug builds on this host. Resolved by running the
   campaign at `--jobs 8`; both instrumented profiles build and execute.
4. **Dangling references in property tests.** Four/five property tests bound a
   `const auto&` reference to a subobject of the temporary `shared_ptr` returned
   by `context->performance()`, producing heap-use-after-free under ASan.
   Resolved by copying the snapshot by value (commit `5aeaffa`).
5. **Verifier out-of-bounds on mutated ids.** The independent symbolic-registry
   verifier indexed `a.vertices[id.value_for_debug()]`/`a.curves[...]` without a
   bounds check, so a mutated-id artifact caused a heap-buffer-overflow instead
   of a clean rejection. Resolved by rejecting non-canonical ids and null
   upstream pointers before any dereference (commit `5aeaffa`).
6. **Sanitizer test timeouts.** Six correct tests (GlobalArrangement.Properties,
   CellClassification.Properties, Selection.Properties, Approximate.Adversarial,
   Replay, PerformanceBaselines) exceeded their fixed 300-600s CTest timeouts
   under ASan. Resolved by scaling every mesh Boolean CTest timeout by 6x under
   `WITH_ASAN`/`WITH_TSAN`/`WITH_MSAN` (commit `2603459`).
7. **GCC `_GLIBCXX_DEBUG` `yspan` iterator const-correctness.** The
   `gcc-current-libstdcxx-debug` build failed compiling `YgorStats.cc` because
   `yspan<T>::iterator` declared a `random_access_iterator_tag` but its
   `operator-`/`operator<`/`operator+`/`operator-` were not `const`-qualified,
   so `__gnu_debug::__get_distance` could not subtract two `const` iterators.
   Resolved by making the read-only iterator operators `const` (commit
   `e78c38d`); the profile now builds and its 72-test contracts step passes.
8. **Clang 7.0.1 ASan+UBSan shared-library link.** The two verifier-isolation
   `SHARED` libraries failed to link under Clang ASan+UBSan because
   `LINKER:--no-undefined` was applied while Clang 7.0.1 does not link the
   sanitizer runtime into shared libraries (the `__asan_*`/`__ubsan_*` symbols
   are resolved at load time from the executable's runtime). Resolved by not
   applying the `--no-undefined` self-containment check under sanitizer builds
   (commit `e78c38d`); the profile now builds and its 72-test contracts step
   passes, and the two Clang ASan fuzz allocations execute.
9. **Fuzz chunk sizing under ASan.** The GCC ASan+UBSan valid-geometry fuzz
   allocation's default chunk (16 runs x 128 generated cases of the
   Fuzz/EndToEnd/Metamorphic set) exceeded the 1800-second chunk wall limit, so
   no successful CPU time was retained. Resolved by rerunning the campaign with
   `P610_FUZZ_RUNS_PER_CHUNK=1`, `P610_FUZZ_CASES_PER_RUN=32`, and
   `--fuzz-chunk-seconds 3600`; the 600 CPU-second floor is unchanged and every
   allocation completed.

## Corpus coverage

- Corpus architecture, category coverage, and ingestion boundaries are defined
  in `YgorMeshesBooleanQualificationCorpus.h`,
  `YgorMeshesBooleanQualificationGeneration.h`,
  `YgorMeshesBooleanQualificationIngestion.h`, and their documentation.
- The permanent corpus has been expanded beyond the original box-dominated
  sample with construction-known generators, CAD-like ingestion records,
  operation chains, and retained minimized regressions, but the frozen candidate
  campaign that would certify corpus coverage at scale has not completed.
- Coverage is therefore not certified; corpus floors remain an open gate.

## Generators and fuzzing

- Construction-aware generators, invalid/preparation mutators, and operation
  chains are implemented and independently tested at the component level.
- The leadership cost-saving decision shortened every fuzz-duration allocation
  from the original 24 CPU-hours to a 600 aggregate CPU-second (10-minute)
  floor, and explicitly stated that implementation proceeds after a much more
  modest campaign with end-user beta testing carrying the residual validation
  burden. This is recorded in `docs/MeshBooleanQualification.md`,
  `docs/MeshBooleanP610ManualCampaign.md`, and `docs/MeshBooleanBetaTesting.md`.
- **The shortened campaign has been executed on the available toolchain.** The
  `--available-toolchain` limited run executed the non-deferred frozen manifest
  via the in-tree dispatcher (72 cases, all `verified_exact_success`) and the
  six runnable 600-CPU-second fuzz allocations on this host to
  `campaign_status=complete_limited_toolchain`. The full controlled campaign
  (oldest compilers, libc++, ThreadSanitizer, AArch64, and the two TSan fuzz
  allocations) remains deferred and documented as known limitations. The
  bounded CI checker test (`MeshBoolean.QualificationCandidate`) is a runner
  smoke test, not campaign evidence.

## Outcomes

- Normalized outcome taxonomy and false-success accounting are implemented in
  `YgorMeshesBooleanQualificationAccounting.h` and
  `docs/MeshBooleanQualificationAccounting.md`.
- The available-toolchain limited campaign produced a completed candidate
  outcome summary at commit `e78c38d` (see the evidence directory
  `summary.tsv`): 33/33 required steps passed, zero unresolved anomalies, and
  all 72 non-deferred manifest cases `verified_exact_success`. It does not
  certify success, typed-failure, or false-success rates for any production
  workload profile.
- Zero false successes are observed in the limited campaign, but zero false
  successes cannot be claimed at controlled-campaign scale; the gate remains
  open pending end-user beta-testing findings.

## Disagreements

- Independent backend comparison is defined in
  `YgorMeshesBooleanQualificationComparison.h` and
  `docs/MeshBooleanQualificationComparison.md` and executes in diagnostic-only
  mode; agreement is evidence, not promotion.
- No material disagreement has been resolved against a completed campaign.
  Any unexplained disagreement remains a blocking gate.

## Sanitizer and determinism

- Component-level sanitizer, determinism, replay, and schedule-matrix tests
  exist and pass in CI, but the full P6.8 matrix has not produced committed
  campaign outcomes. No suppression is reviewed as complete.

## Performance, memory, and cancellation

- The frozen methodology is in `YgorMeshesBooleanQualificationPerformance.h`
  and `docs/MeshBooleanQualificationPerformance.md`; P0 baseline comparisons
  from `plan_speed.md` remain regression constraints.
- No performance, peak-memory, exact-number-growth, verifier-overhead, or
  cancellation-latency numbers are published for a completed controlled
  campaign.

## Promotion decisions

- **No promotion.** `qualified_default` selection requires a validated
  `qualification_evidence_binding` whose campaign manifest, complete machine
  summary, and reviewed human report all bind to a `qualified` decision with
  zero blocking outcomes and zero false successes. No such binding exists.
- Revocation and demotion are explicit: `make_qualification_demotion_report`
  moves a previously `qualified` profile to `revoked` (false success or
  unexplained disagreement) or `candidate` (schema incompatibility or material
  platform defect), and
  `qualification_report_authorizes_promotion` guarantees a demoted report can
  never re-authorize `qualified_default`. A material schema/capability/platform
  change invalidates a prior claim unless an approved compatibility review binds
  the exact change (`qualification_claim_remains_valid`).

## Known limitations

- `exact_in_T` is a strict special-purpose mode, not the practical CAD output
  target; many valid intersections are expected to return
  `output_not_representable`.
- Strict, already-valid operands are required; unknown-provenance STL/OBJ/scan
  tessellations need explicit normalization with a reviewed report.
- The shortened campaign is a floor, not statistical completeness; end-user beta
  testing is a first-class part of the release path (see
  `docs/MeshBooleanBetaTesting.md`).

## Replay artifacts

- The retained (rejected) campaign evidence lives outside the source tree at
  `p610-candidate-evidence/` (campaign `p610-b8427a7a70dc-b9fb5f16437d-x86_64`)
  with `campaign.tsv`, `summary.tsv`, `steps.tsv`, `attempts.tsv`,
  `observations.tsv`, `anomalies.tsv`, `resolutions.tsv`, and `SHA256SUMS`. It is
  diagnostic only; it is not qualification evidence and cannot support
  promotion.
- The `--available-toolchain` limited campaign evidence was produced at
  `p610-available-evidence-r2/` (outside the source tree) at commit `e78c38d`
  with the same protocol and `available_toolchain=1`; it finalized
  `campaign_status=complete_limited_toolchain` with zero unresolved anomalies.
  It is candidate evidence only and cannot support promotion.
- A full controlled campaign on controlled infrastructure, plus end-user beta
  testing, is still required before any profile may be promoted.
