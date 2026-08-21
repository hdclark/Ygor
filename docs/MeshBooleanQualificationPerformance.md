# Performance, memory, and cancellation qualification methodology (P6.9)

Plan 16 P6.9 freezes the measurement contract that the P6.10 candidate campaign
must execute before any mesh Boolean profile can be promoted. The public C++17
interface is `YgorMeshesBooleanQualificationPerformance.h`. It records canonical
plans, raw observations, exact statistics, reviewed thresholds, and transactional
failure evidence. It does not turn a developer workstation run or a continuous
integration smoke test into qualification evidence.

## Controlled measurement protocol

`make_default_qualification_performance_plan()` requires a controlled strict-FP
Release protocol with:

- three warm-up runs and seven measured runs;
- an odd measured-run count of at least five;
- retained per-run samples rather than only aggregates;
- a monotonic wall clock and a process CPU clock;
- an exclusive controlled host with recorded CPU affinity and frequency policy;
- a recorded allocator policy;
- peak RSS from an external process observer; and
- authoritative engine-accounted bytes recorded separately from RSS.

Every observation binds the exact protocol, workload recipe, hardware,
environment, and command through canonical digests. Arrival order is irrelevant:
observations and samples are canonicalized by case identifier and ordinal.
Missing, duplicated, truncated, or stale evidence fails closed.

Median and median absolute deviation are reconstructed from the retained raw
samples. Wall-clock values are qualification evidence only on the controlled
runner. Continuous CI validates the schema and exact gate arithmetic; it does
not impose noisy elapsed-time gates.

## Frozen workload catalog

The default plan contains small, medium, large, and adversarial cases for every
Plan 16 P6.9 area:

- strict validation;
- normalization diagnosis;
- each of the nine independently selectable normalization repair classes;
- backend evaluation;
- topology preflight;
- durable exact-result serialization;
- strict `exact_in_T` realization;
- certified approximate realization;
- mandatory verification;
- bounded exhaustive verification;
- multi-step operation chains with re-ingestion;
- authoritative-byte resource failure; and
- mid-stage cancellation.

The catalog also retains the original nontrivial `plan_speed.md` B1-B7 families
as explicit baseline comparisons. Each case binds its operation, coordinate and
index specialization, expected normalized outcome, geometry category, recipe,
and complete required metric mask.

## Required measurements

Each measured run records all of the following as unsigned integer values:

- wall and process CPU time;
- externally observed peak resident memory;
- authoritative engine-accounted bytes;
- peak exact-number bit and limb growth;
- candidate, event, and patch counts;
- verifier time;
- realization search nodes and connected components;
- exact-result/output serialization bytes;
- cancellation latency; and
- the normalized qualification outcome plus canonical success or failure
  binding.

Counts that are inapplicable to a stage may be zero. Timed measurements and peak
RSS must be present and nonzero. Successful samples bind one canonical semantic
digest; failed samples bind one canonical failure digest. All repetitions for a
case must retain the same typed outcome and semantic binding.

## Frozen performance gates

The default plan preserves the reviewed `plan_speed.md` P13 targets exactly:

1. B1-B7 must achieve at least a four-times geometric-mean wall-time speedup.
2. B6 must achieve at least a five-times wall-time speedup.
3. No individual B1-B7 case may have a candidate median above 115 percent of its
   baseline median.
4. Candidate authoritative bytes may not exceed the equivalent baseline
   artifact's authoritative bytes.

Geometric-mean and ratio comparisons use exact arbitrary-width integer products;
no floating-point logarithm, epsilon, rounding tolerance, or overflow-prone
fixed-width product decides a gate.

A frozen threshold cannot be silently edited or removed. A revision requires a
canonical approved review naming the exact prior and revised threshold digests,
a reviewer, rationale, and independent evidence digest. The revised value then
becomes the value enforced by the report checker. Unknown reviews, broad waivers,
unapproved reviews, stale bindings, and nonblocking replacements fail closed.

## Resource and cancellation transactionality

Resource-limit and cancellation observations are separate adversarial workload
classes. Passing evidence requires:

- the declared limit or cancellation to trigger;
- the exact expected typed failure;
- identical publication-state digests before and after execution;
- complete transaction rollback;
- no partially published artifact;
- a nonzero replay binding; and
- cancellation latency at or below the frozen case limit for every measured
  run.

These checks preserve the product rule that measurement pressure cannot weaken
transactional publication or convert a safe failure into partial success.

## Canonical plan and report

Every protocol, case, threshold, review, sample, observation, statistic, plan,
and report has a domain-separated digest. Report construction independently
revalidates observations, reconstructs medians and median absolute deviations,
evaluates all threshold relations, and counts every missing, extra, invalid, or
failing item as blocking.

`qualification_performance_gate_passes(...)` requires one valid observation for
every frozen case, every blocking threshold to pass, a complete canonical
report, and zero blocking issues. P6.9 freezes this machinery and methodology.
P6.10 must execute it on controlled infrastructure and resolve every outcome;
P6.11 must publish the resulting report before any profile is promoted.

## CI smoke versus qualification evidence

`Test_MeshesBooleanQualificationPerformance` checks catalog coverage, canonical
replay, raw-sample reconstruction, observation-order independence, exact
geometric-mean and ratio gates, digest-bound threshold review, authoritative-byte
regression rejection, and transactional cancellation/resource failure.

`.github/workflows/mesh-boolean-p6-performance.yml` builds and runs that bounded
checker with current GCC and Clang. Those jobs prove the P6.9 implementation and
fail-closed tests compile. They do not claim exclusive hardware, stable CPU
frequency, representative RSS, baseline timing, or completion of the candidate
campaign.
