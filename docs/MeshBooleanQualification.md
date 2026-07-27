# Mesh Boolean qualification evidence (P6.1)

Plan 16 qualification is evidence about one precisely declared
backend/result/preparation/workload profile. It is not inferred from build
success, component completion, backend maturity, or agreement with another
backend. The P6.1 interfaces in `YgorMeshesBooleanQualification.h` define the
canonical records that later P6 phases must populate.

No profile is promoted by these schemas. The in-tree
`experimental_exact_v1` backend remains experimental until a frozen campaign
passes every applicable Plan 16 gate and a reviewed report is bound into the
runtime selector.

## Canonical records

Three independently versioned records form one qualification evidence set:

1. `qualification_campaign_manifest` freezes what must be executed.
2. `qualification_result_summary` records normalized machine outcomes and
   replay artifacts.
3. `qualification_human_report` records the reviewed decision and the required
   narrative sections.

Each record has an eight-byte domain tag, a schema version, an exact payload
length, a canonical positional payload, and a digest over the semantic payload.
Decoders enforce caller-supplied record, string, and vector limits before
allocation. They reject unknown enum values, malformed booleans, truncation,
trailing bytes, duplicate or non-canonical ordering, stale digests, and
non-canonical re-encoding.

The campaign manifest binds all material qualification inputs:

- backend and adapter identity, version, build, capabilities, source/binary and
  dependency digests, and diagnostic-only status;
- result representation and realization semantics;
- strict or normalized preparation policy, unit, and exact binary64 tolerance
  bits where applicable;
- coordinate and index specializations;
- compiler, standard library, architecture, operating system, target, build
  mode, floating-point mode, flags, and environment digest;
- mandatory verifiers;
- corpus identity, source/provenance, redistribution class, case count,
  canonical content, category coverage, and expected-outcome digests;
- generator implementation, seed interval, and parameter-range digest;
- fuzz engine, dictionaries, mutators, seed set, duration, and worker count;
- operation-chain definition and required step range;
- resource, timeout, and cancellation policies;
- performance hardware and measurement protocol;
- release thresholds; and
- explicit exclusions and known limitations.

A campaign manifest must identify a clean repository commit and tree. Required
binding collections cannot be empty. Strict validation cannot carry a modeling
tolerance, and operation-chain definitions must require at least five steps.
These checks prevent a syntactically valid but materially underspecified
campaign from being treated as qualification evidence.

## Outcome taxonomy and accounting

Every executed case is normalized to exactly one
`qualification_outcome`:

- verified exact success;
- verified certified-approximate success;
- expected typed failure;
- unexpected typed failure;
- backend disagreement;
- verifier disagreement;
- false success;
- nondeterministic outcome;
- timeout or resource-limit outcome; or
- infrastructure failure.

Counts are dimensioned by backend, result representation, preparation policy,
operation, coordinate/index specialization, toolchain, and geometry category.
A summary recomputes blocking outcomes and false successes from the detailed
counts. False successes remain a separate severity metric and are never folded
into a generic failure rate. Overflow, duplicate dimensions/outcomes, zero
counts, or stale aggregate fields fail validation.

A machine summary can be incomplete while a campaign is still running, but an
incomplete summary cannot create runtime qualification evidence.

## Human report schema

A report binds the campaign and summary digests and carries one decision:
`candidate`, `qualified`, `rejected`, or `revoked`. It contains exactly one
ordered section for each required Plan 16 report area:

- executive result;
- repository and commands;
- platform matrix;
- corpus coverage;
- generators and fuzzing;
- outcomes;
- disagreements;
- sanitizer and determinism;
- performance, memory, and cancellation;
- promotion decisions;
- known limitations; and
- replay artifacts.

`render_qualification_human_report_markdown` produces deterministic Markdown,
and the report binds both the rendered Markdown digest and the complete report
digest. A `qualified` decision is invalid when either the blocking-outcome or
false-success count is nonzero.

## Material changes and compatibility review

The manifest carries two digests:

- `material_binding_digest` covers every campaign input that can change the
  meaning or strength of a qualification claim; and
- `manifest_digest` additionally covers explicit compatibility reviews.

Changing a backend capability, result/preparation policy, verifier, compiler or
floating-point mode, corpus, generator, fuzz campaign, chain, resource policy,
performance protocol, threshold, exclusion, limitation, or schema changes the
material binding and invalidates the prior claim.

`qualification_claim_remains_valid` accepts a changed material binding only
when the new manifest contains an approved compatibility review that names the
exact prior manifest digest and the exact
`qualification_material_change_digest(prior, next)`. The review itself carries
reviewer, rationale, and evidence digest. A broad waiver, mismatched prior
claim, stale review, or unapproved review has no effect.

## Runtime qualified-default bridge

The product-facing `qualification_manifest` remains a compact selector over
backend capability, representation, preparation mode, and workload profile.
It cannot authorize a qualified default by itself. Its
`qualification_evidence_binding` must contain nonzero digests for a validated
campaign manifest, complete machine summary, and reviewed human report.

`make_qualification_evidence_binding` is the only supported bridge. It requires:

- valid canonical campaign, summary, and report records;
- exact manifest/summary/report cross-bindings;
- the same repository commit in the manifest and summary;
- a complete summary;
- a `qualified` report decision; and
- zero blocking outcomes and zero false successes.

The resulting evidence binding participates in the compact selector digest, so
mutation or substitution makes `qualified_default` fail closed. P6.2 through
P6.11 must generate the corpus, execute the frozen campaign, resolve every
blocking outcome, and commit the reproducible evidence before any actual
profile is promoted.


## Independent backend comparison (P6.4)

`YgorMeshesBooleanQualificationComparison.h` and
`MeshBooleanQualificationComparison.md` define the diagnostic-only comparison
evidence used by P6.4. The frozen `axis_aligned_box_pair_v1` profile executes
all five Boolean operations against `experimental_exact_v1` and the
independently implemented axis-aligned-box adapter. It retains complete attempt
payloads and typed failures, exact open-cell probe classifications, semantic
differences, deterministic minimization transcripts, and explicit reviewed
resolution classifications. A complete campaign remains blocking until every
material disagreement is explained; comparison never selects or repairs the
producer result and does not promote any backend.

## False-success detection and accounting (P6.5)

`YgorMeshesBooleanQualificationAccounting.h` and
`MeshBooleanQualificationAccounting.md` define the external observer that audits
every published success, distinguishes failed checks from missing verifier
evidence, normalizes each execution to one outcome, and aggregates outcomes over
the full qualification dimensions. False successes retain explicit semantic,
binding, topology, certificate, occupancy, embedding, orientation, nesting,
attribute, and approximation-bound reasons and remain separate from safe typed
failures. A complete campaign can pass the P6.5 gate only with zero false
successes and zero other blocking outcomes.


## Controlled CAD-like corpus ingestion (P6.6)

`YgorMeshesBooleanQualificationIngestion.h` and
`MeshBooleanQualificationIngestion.md` define the source-controlled ingestion
boundary for internally generated, licensed external, and private CAD-like
workloads. Each record binds source/generator class, source system, intended
model tolerance, license/provenance, preparation report, expected outcome, exact
failure vocabulary, and redistribution policy. Repository and permitted
content-addressed bytes are digest-verified before use; private digest-only
artifacts are never materialized. External/private records require a distinct
bounded in-repository representative, and the manifest derives anonymized
category/outcome summaries and ordinary P6.1 corpus bindings from canonical
records rather than trusting caller-supplied totals.
