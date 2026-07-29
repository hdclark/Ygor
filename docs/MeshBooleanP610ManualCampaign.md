# Manual single-machine P6.10 candidate campaign

`scripts/run_mesh_boolean_p610_campaign.sh` is the restartable manual driver for
collecting the outstanding P6.10 execution evidence. It runs the repository's
mesh-Boolean qualification tests, retains every command and attempt, accumulates
the eight frozen fuzz-duration allocations, and writes a durable anomaly ledger
for later P6.11 review.

The driver is deliberately fail closed. It does not turn a missing compiler,
unsupported standard library, unavailable architecture, timeout, flaky retry, or
short smoke run into passing evidence. It continues with independent steps where
possible and returns nonzero while any required step or anomaly remains open.

## Host prerequisites

Use a clean checkout of the exact PR commit on a controlled Linux host. The
script requires Bash, Git, CMake, CTest, Ninja, GNU `time`, GNU `timeout`, `awk`,
and SHA-256 support through `sha256sum` or `shasum`. Install the current GCC and
Clang toolchains before starting. The full frozen matrix also needs:

- the oldest-supported GCC and Clang installed side by side;
- libc++ headers and libraries for the Clang libc++ debug/hardening profile;
- sanitizer runtimes for GCC/Clang ASan+UBSan and Clang TSan; and
- a native or emulated command for both frozen AArch64 toolchain cases; and
- a dispatcher command that executes every actual non-deferred frozen candidate
  manifest entry and emits the observation protocol documented below.

The default frozen matrix is x86-64-primary. Run this driver on an x86-64 host;
use the reviewed native/emulated command for the two AArch64 cases. A non-x86-64
primary host remains blocking rather than being silently relabelled.

The work directory contains restartable CMake builds and can be large. The
output directory contains review evidence. Put both outside the source tree.

## Validate the driver first

```bash
./scripts/run_mesh_boolean_p610_campaign.sh --self-test
```

This exercises atomic status files, pass resumption, failure classification,
anomaly retention, summary generation, and checksum generation without building
Ygor.

A short non-qualifying smoke run is useful before committing a host to the full
campaign:

```bash
./scripts/run_mesh_boolean_p610_campaign.sh \
  --output ../p610-smoke \
  --work ../p610-smoke-work \
  --smoke \
  --fuzz-cpu-seconds 2 \
  --fuzz-chunk-seconds 120
```

Smoke evidence is permanently marked `non_qualifying_smoke` and cannot close
P6.10. To remain practical on one workstation, smoke mode builds only the
P6.10 candidate checker plus representative end-to-end, metamorphic, and fuzz
targets under current GCC and Clang Debug profiles. The full non-smoke campaign
continues to build and execute the complete frozen inventory.

## Run the full campaign

Set the oldest-supported compiler commands explicitly. These values are part of
the retained environment evidence.

```bash
export P610_OLDEST_GCC_CC=gcc-OLD
export P610_OLDEST_GCC_CXX=g++-OLD
export P610_OLDEST_CLANG_CC=clang-OLD
export P610_OLDEST_CLANG_CXX=clang++-OLD
export P610_INDEPENDENT_ARCH_COMMAND=/path/to/run-one-aarch64-case.sh
export P610_NONDEFERRED_CAMPAIGN_COMMAND=/path/to/run-frozen-candidate-inventory.sh

./scripts/run_mesh_boolean_p610_campaign.sh \
  --output ../p610-candidate-evidence \
  --work ../p610-candidate-work \
  --phase contracts
```

Run the exact same command again after a machine interruption or after fixing an
infrastructure problem. Passed steps are skipped. Failed or interrupted steps
receive a new numbered attempt while all previous logs remain immutable. The
repository commit, tree, dirty working state, driver version, duration target,
and each fuzz allocation's command digest are frozen on the first invocation.
Changing any of them requires a new evidence directory.

The driver pre-registers all 55 mechanical steps before running any phase.
Therefore `--phase contracts`, `--phase fuzz`, or `--phase finalize` cannot report
complete evidence merely because the unselected phases were absent from the
ledger.

After the non-deferred profiles are complete, run the eight frozen 24 CPU-hour
allocations:

```bash
./scripts/run_mesh_boolean_p610_campaign.sh \
  --output ../p610-candidate-evidence \
  --work ../p610-candidate-work \
  --phase fuzz
```

Each allocation is executed in finite chunks and measured with GNU `time`. The
default target is 86,400 aggregate CPU-seconds per allocation. The progress TSV
is updated after every chunk, while only exit-code-zero chunks contribute to the
accumulated user+system CPU total. A chunk timeout or failed test is recorded as
an anomaly and contributes no passing duration. Restarting resumes from the
successful accumulated total.

The complete sequence can also be requested in one invocation:

```bash
./scripts/run_mesh_boolean_p610_campaign.sh \
  --output ../p610-candidate-evidence \
  --work ../p610-candidate-work
```

The command may take many days on one machine. Running it inside `tmux`,
`screen`, or a supervised service is recommended. Do not run two processes
against the same output directory. A `.lock` directory prevents accidental
concurrent writers. Remove a stale lock only after confirming that no campaign
process remains alive.

## Single-machine toolchain and architecture cases

The automatic x86-64 profiles mirror the frozen P6.8 descriptors rather than a
looser compiler smoke matrix:

- current GCC Debug and Release with libstdc++;
- oldest-supported GCC Debug and Release with libstdc++;
- current Clang Debug with libc++ and Release with libstdc++;
- oldest-supported Clang Debug with libstdc++ and Release with libc++;
- GCC ASan+UBSan with libstdc++;
- Clang ASan+UBSan with libc++;
- Clang TSan with libstdc++ and the concurrency tests;
- GCC libstdc++ debug iterators; and
- Clang libc++ debug/hardening mode.

Every profile builds the explicit mesh-Boolean qualification target list and
runs the non-fuzz `mesh_boolean` CTest set. The two current Release profiles also
retain B0-B8 and exact-arithmetic benchmark records. Missing tools become
`missing_configuration` anomalies; they are never ordinary skips.

The two frozen AArch64 descriptors are driven on the same physical host through:

```bash
export P610_INDEPENDENT_ARCH_COMMAND=/path/to/run-one-aarch64-case.sh
```

The command is invoked once for each case. It runs from the repository root and
receives:

```text
P610_ARCH_CASE_ID
P610_REQUIRED_ARCHITECTURE=aarch64
P610_REQUIRED_COMPILER_FAMILY
P610_REQUIRED_STANDARD_LIBRARY
P610_REQUIRED_BUILD_TYPE
P610_REQUIRED_COORDINATE
P610_REQUIRED_INDEX
```

For each invocation it must print one tab-separated observation marker:

```text
P610_OBSERVATION<TAB>CASE_ID<TAB>OUTCOME<TAB>ERROR_CODE<TAB>EVIDENCE_PATH
```

`EVIDENCE_PATH` is empty or is relative to the campaign output directory.
Without the command or its matching observation marker, each AArch64 case remains
an explicit blocker. A cross-compile that was not executed is not evidence.

## Actual non-deferred frozen manifest dispatcher

The repository's focused `MeshBoolean.QualificationCandidate` test validates the
checker; it does not execute the Boolean producer's frozen campaign inventory.
The manual driver therefore requires an explicit actual dispatcher:

```bash
export P610_NONDEFERRED_CAMPAIGN_COMMAND=/path/to/run-frozen-candidate-inventory.sh
```

It runs from the repository root with `P610_NONDEFERRED_OUTPUT_DIR` set to
`artifacts/nondeferred-frozen-manifest` in the evidence directory. It must execute
every non-deferred frozen case and print one `P610_OBSERVATION` marker per distinct
case. It may print `P610_ANOMALY` markers using the protocol below even when it
returns zero.

On success it must create `completion.tsv` in that directory:

```text
schema<TAB>1
status<TAB>complete
executed_case_count<TAB>POSITIVE_INTEGER
manifest_digest<TAB>64_HEX_DIGITS
observation_digest<TAB>64_HEX_DIGITS
```

The driver requires the number of distinct retained observation markers to equal
`executed_case_count`. A missing dispatcher, malformed completion record, or
missing observation remains blocking. This prevents checker-only CTest output
from being mistaken for campaign evidence.

## Fuzz allocation commands

The driver creates all eight frozen P6.8/P6.10 allocations:

1. GCC ASan+UBSan valid geometry;
2. GCC ASan+UBSan invalid/preparation;
3. Clang ASan+UBSan valid geometry;
4. Clang ASan+UBSan invalid/preparation;
5. Clang TSan valid geometry;
6. Clang TSan invalid/preparation;
7. unsanitized operation chains; and
8. unsanitized long-running exact-growth workloads.

The repository's current qualification CTest groups are the default commands.
Every actual command is retained in `commands/` and digest-bound from the
corresponding progress file. A dedicated or amended campaign worker can be used
without changing the driver by setting one of:

```text
P610_FUZZ_GCC_ASAN_VALID_COMMAND
P610_FUZZ_GCC_ASAN_INVALID_COMMAND
P610_FUZZ_CLANG_ASAN_VALID_COMMAND
P610_FUZZ_CLANG_ASAN_INVALID_COMMAND
P610_FUZZ_CLANG_TSAN_VALID_COMMAND
P610_FUZZ_CLANG_TSAN_INVALID_COMMAND
P610_FUZZ_OPERATION_CHAIN_COMMAND
P610_FUZZ_LONG_RUNNING_COMMAND
```

An override runs from the repository root and receives these variables:

```text
P610_BUILD_DIR
P610_ALLOCATION_ID
P610_CHUNK_ORDINAL
P610_ITERATION
YGOR_BOOLEAN_TEST_TIER=qualification
YGOR_BOOLEAN_GENERATED_CASES
YGOR_BOOLEAN_SEED_HIGH
YGOR_BOOLEAN_SEED_LOW
YGOR_BOOLEAN_ARTIFACT_DIR
```

The seed pair changes deterministically for each iteration. The C++ test config
honours the three `YGOR_BOOLEAN_*` seed/budget variables. The default number of
runs per chunk is 16 and the default generated-case budget is 128; they can be
changed with `P610_FUZZ_RUNS_PER_CHUNK` and
`P610_FUZZ_CASES_PER_RUN`. Changing them does not lower the 86,400 CPU-second
floor.

P6.11 must review the retained command, not merely the allocation name. Repeated
contract tests must not be described as broader geometric coverage than they
actually exercised. A specialized override must retain its own seeds, corpus,
failure index, minimization, replay, and promoted-regression artifacts under
`YGOR_BOOLEAN_ARTIFACT_DIR` where applicable.

## Recording and resolving anomalies

Every nonzero attempt is retained and automatically searched for false success,
backend/verifier disagreement, nondeterminism, resource/timeout, performance,
sanitizer, and typed engine failure markers. At least one generic anomaly is
written when no known marker is present.

A command can report structured records even when its process exits zero:

```text
P610_ANOMALY<TAB>CATEGORY<TAB>CASE_ID<TAB>DETAIL<TAB>EVIDENCE_PATH
P610_OBSERVATION<TAB>CASE_ID<TAB>OUTCOME<TAB>ERROR_CODE<TAB>EVIDENCE_PATH
```

Paths must be empty or relative files already present inside the evidence
directory. Malformed markers and unsafe or missing evidence paths create their
own blocking anomaly. Multiple observations for one case are retained rather
than coalesced, so nondeterministic or disagreeing outcomes cannot disappear.

Use the manual command for an observation that is not safely inferable from a
log:

```bash
./scripts/run_mesh_boolean_p610_campaign.sh \
  --output ../p610-candidate-evidence \
  --record-anomaly backend_disagreement case.identifier \
  'Independent backend occupancy differs on retained probe 17.' \
  artifacts/case.identifier/comparison.tsv
```

Recommended category tokens are:

```text
false_success
backend_disagreement
verifier_disagreement
nondeterministic_result
unexpected_typed_failure
resource_or_timeout
timeout
performance_regression
sanitizer_failure
infrastructure_issue
missing_configuration
other_anomaly
```

Do not delete an anomaly after a successful retry. Bind a reviewed resolution to
its stable ID instead:

```bash
./scripts/run_mesh_boolean_p610_campaign.sh \
  --output ../p610-candidate-evidence \
  --resolve-anomaly ANOMALY_ID reviewer-name \
  'Root cause fixed, minimized regression promoted, and affected profiles rerun.' \
  artifacts/resolutions/ANOMALY_ID/evidence.tsv
```

The evidence path must be inside the output directory. Its SHA-256 digest is
written to `resolutions.tsv`. This mechanical binding does not decide whether a
resolution is sufficient: engine defects still require minimized permanent-test
promotion and affected-configuration reruns under the P6.10 closure rules.

## Durable output contract

Only the evidence directory is intended for review or addition to the PR. The
sibling work directory is disposable build state.

| Path | P6.11 meaning |
|---|---|
| `campaign.tsv` | Schema, driver version, campaign ID, exact commit/tree, dirty-state flag, architecture, duration floor, and smoke flag. |
| `environment.txt` | First invocation environment snapshot retained for convenience. |
| `environment/invocation-*.txt`, `invocations.tsv` | Every restart's phase, exact commit/tree/dirty state, resource options, tools, compilers, selected environment, and snapshot digest. |
| `git-status.txt`, `git-diff.patch` | Reproducible dirty-state proof. A dirty campaign remains blocking. |
| `steps.tsv` | Frozen driver step inventory and required/optional status. |
| `state/*.status` | Atomic latest status for each step. Historical attempts are never erased. |
| `attempts.tsv` | One row per command attempt with exit status, wall/CPU time, peak RSS, command file, and log file. |
| `commands/*.sh` | Exact replayable commands. Fuzz progress binds their digests. |
| `logs/<step>/` | Numbered complete stdout/stderr and GNU-time records. |
| `fuzz-progress/*.tsv` | Per-chunk CPU accounting, status, memory, log, immutable command path, and command digest for each frozen allocation. |
| `artifacts/` | Test-produced replay, result, failure, minimization, and other retained evidence. |
| `observations.tsv` | Structured per-case outcomes emitted by actual architecture/candidate/fuzz commands, including evidence and originating log. |
| `anomalies.tsv` | Stable step and case IDs plus unresolved failure/disagreement/infrastructure records. |
| `resolutions.tsv` | Reviewer, rationale, evidence path, and evidence digest for resolved anomaly IDs. |
| `summary.tsv` | Required/pass/fail/block counts, unresolved anomaly count, and fail-closed campaign status. |
| `README.md` | Generated top-level status warning for reviewers. |
| `SHA256SUMS` | Digest of every retained evidence file except itself and the live lock. |

After copying, pruning, or adding any permitted external-artifact reference,
rerun:

```bash
./scripts/run_mesh_boolean_p610_campaign.sh \
  --output ../p610-candidate-evidence \
  --phase finalize
```

Do not edit `summary.tsv` or `SHA256SUMS` by hand.

## P6.11 evaluation procedure

Before using these files for P6.11, a reviewer must:

1. verify every line in `SHA256SUMS`;
2. confirm `campaign.tsv` names the reviewed PR commit/tree and says
   `repository_dirty=false` and `smoke=0`; verify every invocation snapshot has
   the same frozen repository state and the digest in `invocations.tsv`;
3. confirm every required `steps.tsv` entry has a passing atomic state and that
   every attempt has its command and log;
4. independently sum user+system CPU for exit-code-zero rows in each of the
   eight `fuzz-progress` files and require at least 86,400 seconds per file;
5. verify each progress row's command digest and review the command for the
   claimed geometry/failure/chain coverage;
6. reconcile `observations.tsv` with the non-deferred `completion.tsv`, AArch64
   cases, expected outcomes, typed errors, replay/result/failure digests, and all
   command logs;
7. inspect all failed attempts and prove each has a corresponding anomaly;
8. require every anomaly to remain blocking or have one reviewed resolution with
   immutable evidence, affected-profile reruns, and permanent regression
   promotion where required;
9. reconcile sanitizer, deterministic replay, timeout/resource, cancellation,
   performance, and independent-backend records against the P6.8/P6.9 gates;
10. verify all referenced replay/minimization/result artifacts are present or
    validly content-addressed; and
11. keep the backend profile at `candidate` unless all applicable P6.10 and
    final release gates are independently satisfied.

`campaign_status=complete_candidate_evidence` means only that the driver's
mechanical inventory is complete and has no unresolved ledger rows. It is not a
self-approval or a production promotion decision.
