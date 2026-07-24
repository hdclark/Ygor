# Mesh Boolean product contract (schema 3)

This document defines the product boundary introduced by Plan 15 P0, the
durable exact-result authority introduced by P1, and the reusable strict
preparation service introduced by P2.1. It does not promote any Boolean backend,
implement backend fallback, or implement approximate realization. Those
behaviours remain gated by later tracker components.

## Current support boundary

The in-tree symbolic engine is identified as `experimental_exact_v1` and has
maturity `experimental`. It is never selected by `qualified_default` unless a
qualification manifest names the exact backend build, capability digest,
preparation mode, result representation, and workload profile. Experimental or
candidate backends require both `explicit_backend` (or another explicitly named
producer policy) and `allow_experimental_backend=true` under
`allow_explicit_unqualified` qualification policy.

The currently implemented evaluator still requires strict, already-valid
operands. Unknown-provenance STL, OBJ, scan, or CAD tessellations are not an
implicitly supported workflow. `strict_validation`, `diagnosis_only`, and
`normalized` are distinct preparation contracts. No normalization operation is
enabled by default, and a geometry-changing normalization policy must state its
model unit, positive tolerance, and enabled operation set. Diagnosis-only
normalization, structural irrelevant-storage removal, and policy-authorized
exact duplicate consolidation and orientation repair are available. Explicit
geometry-changing, attribute-seam-aware near-duplicate vertex consolidation is
also available; all other geometry-changing repair classes remain later P2
work.

## Strict operand preparation

`validate_operand_strict` runs the authoritative Component 2 validator and
returns an immutable `prepared_operand<T,I>`. The prepared operand owns its
input mesh, so later mutation or destruction of caller storage cannot change a
request. Its certificate binds coordinate/index types, input and prepared
digests, strict policy, exact-kernel policy, Component 2 semantic artifact,
verification report, invariant set, and the fact that geometry was not changed.

`encode_prepared_operand` and `decode_prepared_operand` provide canonical,
resource-bounded serialization. Decoding rejects stale bindings, corruption,
truncation, trailing bytes, type overflow, and non-canonical records. A backend
request made from prepared operands verifies those bindings, retains immutable
operand lifetimes, and reruns Component 2 validation in the request context.
Prepared and raw paths therefore have equivalent strict topology semantics;
prepared input is not a bypass around validation.

Prepared-operand schema 3 additionally retains the immutable pre-normalization
source whenever a normalization report is attached. This lets decoding and each
later Boolean request independently replay source mappings and edits against the
published output; the source is provenance evidence and is never evaluated as
the backend operand.

P2.1 strict preparation performs no tolerance operation, snapping, welding,
orientation repair, or other healing. The normalization service separately
supports `structural_only` with exactly one of
`irrelevant_storage_removal`, `exact_duplicate_consolidation`, or
`orientation_repair` enabled.
Irrelevant-storage removal first requires a strict-valid source, removes
vertices unreferenced by every facet, and stably compacts indices and aligned
per-vertex storage. Exact duplicate consolidation retains the lowest source
ordinal for exact floating-value coordinate classes (the two signed-zero
encodings represent the same exact zero), requires all present normals and
colours in a class to compare exactly equal, rewrites connectivity, and removes only
facets that become identical under cyclic rotation or reversal. Conflicting
attribute seams and any collapse or unrelated defect fail closed. Stored
involved-face indices are treated as derived data and rebuilt when present.

Orientation repair first solves exact opposite-edge-use parity independently
for each closed orientable shell, then reconstructs the exact shell containment
forest and enforces outward material shells at even depth and inward cavity
shells at odd depth. It changes only facet ring direction: coordinates,
undirected incidence, facet ordinals, attributes, and metadata remain unchanged.
Open, non-manifold, non-orientable, intersecting, contacting, duplicate, or
ambiguously nested geometry is not healed by this operation.

The geometry-changing `seam_aware_vertex_consolidation` policy requires an
explicit model unit and positive tolerance. Distances are compared as exact
dyadic rationals against the binary64 policy tolerance. Vertices are considered
in source order and merge into the lowest retained compatible ordinal; unequal
present normals or colours preserve a seam and prohibit that merge. The
operation does not remove facets or accept collapsed connectivity. Every actual
move carries a canonical bounded squared-displacement record in the declared
unit, and every merge carries source mapping and topology evidence.

All normalization operations rerun full strict validation before publication
and emit canonical source-to-prepared maps and edit evidence. Structural
operations claim exact-zero displacement; seam-aware near-duplicate
consolidation records every nonzero movement. Diagnosis-only remains the default
and performs no edits. The legacy strict-policy field
`remove_unused_storage=true` continues to fail closed so edits cannot bypass the
normalization report.

## Authoritative result and representations

`exact_result_handle` is an immutable, shared-lifetime owner populated from a
verified `selected_exact_boundary`. It carries no `boolean_context` owner token
and remains valid after the producing invocation is destroyed. Its canonical
record owns exact rational coordinates, selected surface occurrences, local
spherical links, topology obstructions, construction relations, side decisions,
source contributors, and operation/backend/preparation provenance.

`read_exact_result` independently reconstructs the result from canonical bytes;
it does not trust producer-owned derived tables or context tokens. Decoding
checks the canonical digest, all cross-references and certificate counts, exact
topology, backend and preparation bindings, resource limits, and canonical
re-encoding. `request_later_realization<T,I>` records a type- and policy-bound
request against the retained exact-result digest. Exact-coordinate consumers
can use the decoded rational boundary immediately; full strict finite-`T`
envelope integration and certified approximate execution remain P3 work.

`evaluate_boolean_product_result` is the expert productization path for the
current in-tree backend. It publishes the verified exact authority before
calling Components 11 and 12. A topology rejection or strict finite-`T`
realization failure therefore returns an `exact_stratified` envelope with the
failed realization attempt instead of erasing the exact success.

`boolean_product_result<T,I>` is a tagged envelope with exactly three result
representations:

- `exact_stratified`: the authoritative exact boundary, including empty and
  stratified non-manifold topology. It has no mesh payload.
- `exact_in_T_mesh`: a strict mesh realization whose emitted binary floating
  coordinates equal the exact targets. This remains a special-purpose mode,
  not the ordinary CAD-output target.
- `certified_approximate_mesh`: a separately tagged embedding bound to an
  explicit displacement/tolerance policy and certificate. It never claims
  exact point-set equality.

A failed mesh realization may be recorded while the exact result remains a
successful, retained product result. Product-option validation rejects any
policy that permits discarding that exact authority.

## Semantic policy versus search policy

`product_realization_semantics` states what a success means.
`realization_search_policy` states how candidates may be explored. They are
separate fields and are validated against the requested representation.
Changing nearest-value, neighbouring-value, candidate, or backtrack limits
cannot turn an exact request into approximate success.

## Backend selection and fallback

The supported selection contracts are:

- `explicit_backend`: one caller-named producer, no fallback.
- `qualified_default`: only a profile in the bound qualification manifest.
- `diagnostic_compare`: one declared producer plus independent comparators;
  agreement is evidence and never majority-vote publication.
- `explicit_fallback_chain`: a caller-ordered chain and an explicit allow-list
  of failure categories.

Fallback is rejected for internal invariant errors, capability mismatches,
backend or verifier disagreement, stale bindings, replay mismatch, exact-result
serialization errors, and qualification-policy violations. A later fallback
executor must retain the primary failure and all attempted backend identities.
P0 freezes these rules but does not execute a fallback chain.

## Versioning and decoding

Options, artifacts, errors, certificates, and replay bindings use schema 3.
Canonical records contain an eight-byte domain tag, schema, exact payload
length, and positional payload. Decoders reject:

- unknown schema, enum, backend, capability, or error values;
- malformed booleans, truncation, trailing bytes, and non-canonical duplicate
  lists;
- configured record, string, and vector limits before allocation;
- stale capability, manifest, exact-result, and cross-layer digest bindings;
- any reinterpretation of strict `exact_in_T` as approximate output.

Backend provenance contains stable backend ID, adapter semantic version, build
identifier, maturity, capability bits, and capability digest. Product results,
exact results, realization certificate references, fallback records, and replay
bindings carry or validate this provenance.

## Error categories

The product error schema retains the original engine categories and adds:
`normalization_required`, `normalization_failed`, `backend_unavailable`,
`backend_capability_mismatch`, `backend_disagreement`, `backend_unqualified`,
`exact_result_serialization_error`, `attribute_transfer_conflict`,
`approximation_policy_rejected`, and `qualification_policy_violation`.
`stale_binding`, `replay_mismatch`, and `verifier_disagreement` are explicit
fail-closed contract errors and are never fallback-authorized.

## Current non-goals

The current productized scope does not:

- change Components 3 through 10 or their exact semantics;
- normalize or repair an operand;
- execute strict finite-`T` or approximate embedding search from a deferred
  realization request;
- wrap backend execution or run independent adapters;
- transfer application attributes; or
- claim any production-qualified backend/result/preparation profile.
