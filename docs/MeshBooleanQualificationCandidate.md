# Frozen candidate campaign execution and outcome closure (P6.10)

Plan 16 P6.10 is the execution boundary between the previously frozen
qualification inputs and the reproducible report assembled in P6.11. The C++17
interface is `YgorMeshesBooleanQualificationCandidate.h`.

The interface records and validates evidence from a controlled campaign. It does
not fabricate a campaign, treat a bounded CI smoke test as controlled-host
qualification, promote a backend, or weaken any P6.2-P6.9 gate.

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
A dirty repository manifest is rejected.

## Final observations and retained issues

Each execution has one final canonical observation containing the normalized
outcome, accounting evidence, semantic or failure digest, replay binding, run
log, and any performance or resource regression evidence.

Every unexpected typed failure, backend or verifier disagreement, false success,
nondeterministic result, timeout/resource outcome, infrastructure failure, or
performance/resource regression must be retained as an issue. An unresolved
issue is explicitly blocking. Silence is not resolution.

## Resolution requirements

A resolved issue must name a reviewer and rationale, retain independent evidence,
list every affected configuration, and bind one rerun observation per
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
- all final outcomes within their frozen contracts;
- no remaining performance or resource regression;
- every historical resolved issue bound to a successful affected-configuration
  rerun; and
- zero unresolved or missing evidence.

The focused `MeshBoolean.QualificationCandidate` test exercises complete closure,
arrival-order independence, missing execution, unresolved false success,
mandatory regression promotion, stale rerun/case bindings, and performance
regression blocking. The GCC/Clang workflow is a checker smoke test only. Actual
P6.10 campaign records must come from the controlled infrastructure named by the
manifest and remain candidate evidence until P6.11 review.
