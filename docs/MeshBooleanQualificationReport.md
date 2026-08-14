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
- Reported commit: `e5cf632a9c0edd24e6b1bc557d83a6aa12cf702b`.
- Reported tree: `4a73450f671cd49b3261bcc99973d00e79bf32b0`.
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
- Consequently there is no committed passing platform-matrix outcome, and no
  platform profile may be promoted.

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
- **The shortened campaign has not been executed on controlled infrastructure.**
  The eight frozen fuzz-duration allocations and the non-deferred frozen-manifest
  entries remain deferred/blocking. The bounded CI checker test
  (`MeshBoolean.QualificationCandidate`) is a runner smoke test, not campaign
  evidence.

## Outcomes

- Normalized outcome taxonomy and false-success accounting are implemented in
  `YgorMeshesBooleanQualificationAccounting.h` and
  `docs/MeshBooleanQualificationAccounting.md`.
- No complete campaign outcome summary exists to certify success, typed-failure,
  or false-success rates for any workload profile.
- Zero false successes cannot be claimed; the gate remains open pending a
  controlled campaign and end-user beta-testing findings.

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
- A future controlled campaign must be re-run with driver schema v5
  (`scripts/run_mesh_boolean_p610_campaign.sh`) and reviewed against
  `docs/MeshBooleanP610ManualCampaign.md` before any profile may be promoted.
