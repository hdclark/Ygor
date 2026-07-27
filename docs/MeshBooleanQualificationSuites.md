# Preparation, result-mode, attribute, and provenance qualification suites (P6.7)

Plan 16 P6.7 defines independent qualification catalogs for the product paths
that cannot be combined into one undifferentiated success claim. The public
interface is `YgorMeshesBooleanQualificationSuites.h`. It is self-contained,
C++17-only, deterministic, and fail-closed. It records qualification evidence;
it does not promote a backend or replace the P6.5 false-success observer.

## Frozen suite plan

`make_default_qualification_profile_suite_plan()` creates a versioned canonical
plan with three independently assessed families:

- **Preparation:** one strict-validation case, one diagnosis-only case, and one
  case for each of the nine normalization repair operations. Structural repairs
  and geometry-changing repairs remain distinct. Crack closure, seam-aware
  consolidation, non-planar handling, and sliver handling carry explicit
  binary64 model-tolerance bits and units; no other case may carry a tolerance.
- **Result representation:** exact stratified output, strict `exact_in_T` mesh
  output, and `certified_approximate_embedding_v1` are separate cases with
  separate policy and case digests. Mesh cases require re-ingestion, independent
  topology/embedding/orientation/nesting checks, and certificate replay. The
  approximate case additionally requires independently checked displacement and
  support-plane bounds.
- **Attributes and provenance:** the catalog enumerates every advertised
  transfer mode, conflict policy, identifier policy, merge policy, vertex-copy
  policy, sharp-edge policy, texture-seam policy, and construction-provenance
  policy. Dedicated evidence obligations cover seams, conflicts, omissions,
  multi-source mappings, and downstream-style contributor queries.

A plan is canonicalized by case identifier. Missing repair classes, missing
result modes, duplicate identifiers, unknown policy values, or incomplete
attribute-policy coverage are rejected. Canonical bytes and a domain-separated
plan digest bind the complete catalog.

## Explicit bounds; no hidden epsilon

`qualification_explicit_bound_evidence` stores the declared and observed limits
as exact binary64 bit patterns, together with the model unit and policy digest.
The independent checker requires:

- finite, non-negative values;
- a positive declared limit whenever a bound is required;
- an observed maximum no larger than the declared limit;
- a nonzero digest of the independent checker transcript, distinct from the
  suite's self-derived canonical evidence digest; and
- `hidden_epsilon_used == false`.

Exact and structural paths must carry the canonical zero-bound record. They
cannot smuggle a test epsilon into acceptance. Approximate and
geometry-changing paths cannot omit their declared unit, policy binding,
observed maximum, or independent verification.

## Preparation observations

Every preparation observation binds one source operand digest, one frozen case,
and a nonzero independent-verification transcript digest. Strict validation
must not publish a modified operand or normalization report. Diagnosis-only
must publish and independently verify a report but must not publish a prepared
operand or edits. A repair success must publish a report and output digest,
record at least one edit, and prove that the prepared operand passes strict
validation.

Repairs with no permitted coordinate movement require an exact-zero displacement
claim. Repairs that permit movement require per-edit displacement records and an
independently checked maximum within the declared model tolerance. Stale case,
policy, bound, or observation digests fail before report construction.

## Result-mode observations

Every result-mode observation retains and independently verifies the durable
exact-result digest. Exact stratified success publishes no mesh and carries no
approximation evidence. Strict `exact_in_T` and certified approximate mesh
successes require:

- a nonzero output digest;
- strict output re-ingestion;
- independent topology, embedding, orientation, and shell-nesting checks; and
- certificate replay from published bits.

The approximate mode additionally binds independent displacement and
support-plane evidence to the exact result and declared policy. Exact success
cannot be relabeled approximate, approximate success cannot be relabeled exact,
and a missing or stale replay/bound record is a blocking failure.

## Attribute and provenance observations

Each observation binds the exact-result digest, output geometry digest,
attribute report digest, and policy digest. Required seam, conflict, omission,
multi-source, and downstream-query evidence is explicit per catalog case.
Downstream source queries carry their own digest so a boolean flag cannot stand
in for replayable evidence.

All attribute cases share a geometry-invariance group. Report construction
requires the same exact-result digest and geometry digest across every policy
variant and requires an explicit proof that attribute values did not influence
exact topology. Comparing unrelated Boolean results cannot satisfy this gate. A
policy variant that changes geometry is retained as a blocking suite outcome
rather than silently accepted.

## Canonical report and release gate

`make_qualification_profile_suite_report(...)` accepts observations in any
arrival order, validates each against its frozen descriptor, canonicalizes them
by case identifier, and records passed counts and blocking issues. A complete
P6.7 report requires exactly one passing observation for every planned case and
one geometry digest for each invariance group.

`validate_qualification_profile_suite_report(...)` independently rebuilds the
report from the plan and observations. `qualification_profile_suite_gate_passes`
requires both the report and its frozen plan, canonical bytes, matching plan and
report digests, exact planned case counts, and zero blocking issues. Passing
P6.7 is qualification infrastructure only; actual
backend/result/preparation profiles remain unqualified until the frozen P6.10
campaign and reviewed P6.11 report pass every applicable release gate.

## Validation target

`Test_MeshesBooleanQualificationSuites` checks catalog completeness, canonical
replay, observation-order independence, complete report construction, every
explicit-bound rule, stale binding rejection, result-semantics separation,
attribute evidence requirements, geometry invariance, and blocking report
behavior. The focused GCC/Clang workflow is
`.github/workflows/mesh-boolean-p6-suites.yml`.
