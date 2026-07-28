# Frozen candidate campaign execution and outcome closure (P6.10)

Plan 16 P6.10 is the execution boundary between the previously frozen
qualification inputs and the reproducible report assembled in P6.11. The C++17
interfaces are `YgorMeshesBooleanQualificationCandidate.h` and
`YgorMeshesBooleanQualificationCandidateRunner.h`.

The interfaces record, execute, and validate evidence from a controlled
campaign. They do not fabricate a campaign, treat a bounded CI smoke test as
controlled-host qualification, promote a backend, or weaken any P6.2-P6.9 gate.
The serialized schema remains version 1; checker version 2 names the stricter
expected-outcome and supplied-canonical-state semantics described below.

## Frozen campaign plan

`qualification_candidate_campaign_plan` embeds the canonical P6.1 manifest and
requires one passing, independently validated binding for every prerequisite:

- permanent corpus;
- construction-aware generation and operation chains;
- independent backend comparison;
- false-success accounting;
- controlled CAD-like ingestion;
- preparation/result/attribute suites;
- compiler, sanitizer, determinism, resource, and fuzz matrix; and
- performance, memory, resource, and cancellation methodology.

The plan also contains the complete execution inventory. Every execution entry
binds its source plan and case digests, full qualification dimensions, expected
outcome set, permitted typed failures, and regression-promotion policy. Missing,
duplicate, foreign, stale, or reordered input cannot change the canonical plan.
Validation requires the supplied in-memory gate and execution order to encode to
the stored canonical bytes; retaining old bytes after reordering or mutation is
rejected rather than silently re-canonicalized. Only verified exact or
certified-approximate success, expected typed failure, and explicitly contracted
timeout/resource-limit outcomes may be frozen as expected. Unexpected typed
failure, disagreement, false success, nondeterminism, and infrastructure failure
are always anomaly evidence; the plan cannot normalize them away. A dirty
repository manifest or stale checker version is rejected.

## Controlled execution runner

`run_qualification_candidate_campaign(...)` executes the frozen inventory in
canonical order through an infrastructure-supplied case executor. Every returned
observation is independently canonicalized and checked against the exact case
identifier and digest before it can enter the campaign.

Unexpected outcomes and performance/resource regressions automatically produce
an unresolved issue bound to that exact final observation. A caller may provide
retained historical issues and a reconciliation callback, but the ordinary
candidate closure rules still require reviewed evidence, affected-configuration
reruns, and permanent-regression promotion for engine defects.

Cancellation, executor errors, malformed observations, reconciliation errors,
and rejected publication all return typed failures. The publication sink is
invoked at most once and only after the entire campaign has been assembled and
canonicalized. Sink implementations are required to publish transactionally.
No partial campaign is passed to the sink.

## Final observations and retained issues

Each execution has one final canonical observation containing the normalized
outcome, exact typed failure code where applicable, accounting evidence,
semantic or failure digest, replay binding, run log, and any performance or
resource regression evidence. Campaign validation and encoding also require the
supplied observation and issue order, counters, and completion state to match the
stored canonical bytes, so stale canonical bytes cannot mask in-memory mutation.

Every unexpected typed failure, backend or verifier disagreement, false success,
nondeterministic result, timeout/resource outcome, infrastructure failure, or
performance/resource regression must be retained as an issue. An unresolved
issue is explicitly blocking. Silence is not resolution.

## Resolution requirements

A resolved issue must name a reviewer and rationale, retain independent evidence,
list every affected configuration, and bind one distinct rerun observation per
configuration. The final observation for the case must be an allowed outcome,
contain no remaining regression, and appear in the rerun evidence.

Resolved engine defects additionally require a permanent-regression binding with
minimized case, canonical case bytes, minimization transcript, promotion
artifact, and distinct before/after permanent-corpus digests. Infrastructure
resolutions are limited to infrastructure or timeout/resource issues. Policy
differences are limited to backend disagreements and bind the reviewed policy or
plan. Performance/resource resolutions bind the reviewed replacement gate; they
cannot silently discard the original P6.9 threshold.

## Closure gate

`qualification_candidate_campaign_gate_passes(...)` requires:

- a canonical frozen plan and all eight prerequisite gates;
- exactly one final observation for every execution case;
- all final outcomes and typed failure codes within their frozen contracts;
- no remaining performance or resource regression;
- every current anomaly bound to its exact retained issue;
- every historical resolved issue bound to a successful affected-configuration
  rerun; and
- zero unresolved or missing evidence.

The focused `MeshBoolean.QualificationCandidate` test exercises complete closure,
arrival-order independence, checker-version rejection, stale canonical in-memory
state, missing execution, unresolved false success, mandatory regression
promotion, stale rerun/case bindings, performance regression blocking, canonical
runner order, automatic anomaly retention, reviewed reconciliation, cancellation,
and no-partial-publication behavior. The GCC/Clang workflow is a checker and
runner smoke test only. Actual P6.10 campaign records must come from the
controlled infrastructure named by the manifest and remain candidate evidence
until P6.11 review.
