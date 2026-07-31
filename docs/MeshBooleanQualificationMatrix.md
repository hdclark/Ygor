# Compiler, sanitizer, determinism, resource, and fuzz qualification matrix (P6.8)

Plan 16 P6.8 freezes the execution matrix that must be completed before a mesh
Boolean profile can enter the P6.10 candidate campaign. The public C++17
interface is `YgorMeshesBooleanQualificationMatrix.h`. It records and validates
qualification evidence; it does not execute the full multi-day campaign, turn a
continuous-integration smoke run into qualification evidence, or promote a
backend.

## Frozen toolchain and platform matrix

`make_default_qualification_matrix_plan()` requires independent cases covering:

- current and oldest-supported GCC and Clang;
- Debug and optimized builds;
- libstdc++ and libc++;
- x86-64 and AArch64 as an independent 64-bit architecture;
- binary32/binary64 coordinate instantiations and 32-bit/64-bit index paths;
- strict default-rounding and controlled rounding-matrix floating-point modes;
- GCC and Clang ASan/UBSan configurations;
- a TSan configuration that runs the concurrency suite; and
- libstdc++ debug iterators and libc++ debug/hardening mode.

Every observation retains exact compiler, standard-library, operating-system,
target, and compile-flag strings plus environment, build-log, test-log, and
canonical-result digests. A mandatory configuration cannot be represented by a
bare “skipped” flag. The schema supports a documented skip only when the frozen
descriptor explicitly permits it, and then requires a reason and independent
evidence digest. The default P6.8 plan permits no skips.

An unsanitized run does not claim that a sanitizer ran. ASan/UBSan evidence must
be clean, TSan evidence must additionally pass the concurrency suite, and debug
library configurations must prove the intended mode was active. Every executed case must independently verify its frozen strict floating-point
contract.

## Determinism equivalence matrix

The frozen plan varies all Plan 16 dimensions independently:

- worker count;
- task partitioning;
- queue bounds;
- broad-phase implementation;
- predicate filter acceptance/fallback behaviour;
- allocation-order perturbation;
- unordered-container/hash seed;
- all supported ambient rounding modes; and
- replay in separate processes.

Each case belongs to an explicit equivalence group. Report construction compares
the canonical exact artifact, result bytes, typed failure, deterministic
diagnostics, and certificate digests for every member of that group. Timing and
other declared non-semantic counters are intentionally absent from the
signature. An individually well-formed alternate result remains observable but
creates a blocking report issue. Rounding variants must prove controlled mode
selection, and process variants must prove that they actually ran separately.

## Resource, timeout, and cancellation matrix

Authoritative-byte, work-unit, wall-timeout, and cancellation limits are
separate cases. Passing evidence requires the declared limit to trigger the
expected typed failure, identical pre/post publication-state digests, complete
transaction rollback, no partial publication, and successful replay from a
retained digest. Cancellation additionally has a frozen maximum observed
latency. Exceeding that latency, publishing partial state, returning the wrong
error, or omitting replay evidence fails closed.

These records assess observable semantics. They do not authorize a test harness
to inspect or depend on private implementation counters that are not part of
the declared resource contract.

## Fuzz-duration and failure-preservation matrix

Every sanitizer/configuration pair has independent valid-geometry and
invalid/preparation campaigns. Each campaign has a minimum aggregate duration
of 86,400 CPU-seconds (24 CPU-hours). Operation-chain fuzzing and long-running
unsanitized exact-growth fuzzing have the same frozen minimum. The plan rejects
smaller values rather than treating a short smoke run as equivalent evidence.

A fuzz observation binds engine/version, workers, wall and aggregate CPU time,
seed set, dictionary, mutator, corpus, failure index, and replay digests. Before
a campaign can close, every unique outcome must be serialized, minimized, and
promoted to the permanent regression corpus. Unresolved, false-success,
nondeterministic, or infrastructure outcomes remain blocking. A campaign with
zero discoveries is valid only when it still retains nonzero canonical empty
failure-index and replay bindings.

## Canonical plan, observations, and report

Every descriptor and observation has a domain-separated digest. Plans are
canonicalized by identifier and reject duplicates, unknown enum values,
incomplete axis coverage, stale digests, or malformed semantic combinations.
`make_qualification_matrix_report(...)` accepts observations in any arrival
order, independently validates each one, canonicalizes by case identifier, and
records missing, invalid, extra, or determinism-disagreeing evidence as blocking
issues.

`validate_qualification_matrix_report(...)` reconstructs the complete report
from its plan and observations. `qualification_matrix_gate_passes(...)` requires
that reconstruction, one passing observation for every frozen case, a complete
report, and zero blocking issues. This P6.8 gate is qualification
infrastructure. The actual candidate evidence is executed and bound in P6.10,
then reviewed and published in P6.11.

## CI smoke versus qualification evidence

`Test_MeshesBooleanQualificationMatrix` validates the schema, catalog coverage,
canonical replay, observation-order independence, sanitizer/debug/concurrency
requirements, deterministic equivalence, resource transactionality, duration
floors, failure preservation, and stale-binding rejection.

`.github/workflows/mesh-boolean-p6-matrix.yml` runs that bounded contract test
with current GCC and Clang. Those jobs prove that the P6.8 checker compiles and
its fail-closed logic remains intact. They do **not** claim to have run the
oldest compiler, libc++/debug-library, AArch64, sanitizer-duration, or 24-hour
fuzz matrix. Qualification evidence must retain the exact full-matrix versions,
commands, environments, logs, durations, and replay artifacts required by the
frozen plan.
