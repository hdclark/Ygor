# Plan 13 Review Amendment: Budgeted Cleanup and Topological Simplification

## Status, precedence, and implementation intent

This file is a normative implementation-plan amendment to `plan_13_budgeted_cleanup_topological_simplification.md`. It records the independent Component 13 review required by `tracker.md` and integrates the reviewed Component 03 numerical contract, the reviewed Component 12 cleanup handoff, and the Component 14/15 downstream contracts.

The original plan remains normative except where this amendment is more specific or conflicts with it. In a conflict, this amendment controls. Implementers, reviewers, test authors, and verifiers must read both files and the corresponding Component 13 specification amendment.

The original fixed V1 cleanup architecture remains suitable: obligation-driven private mutation, append-only generation-checked topology, exact link and fan reconstruction, deterministic serial action selection, transactional budget reservation, bounded patch correspondence, conservative interaction checks, canonical publication, independent action replay, strict C++17, and no external dependencies.

This amendment corrects five integration defects without weakening that architecture:

1. use `triangulated_output_complex<T,I>` / `triangulated_output_complex_view<T,I>` consistently;
2. replace obsolete `exact-nominal` semantics with explicit `rounded_nominal`, `exact_relation`, and `uncertainty_enclosure` evidence;
3. make every tolerance-facing cleanup metric dimensionally a length;
4. import Component 12's reviewed length-valued handoff certificate as advisory evidence only; and
5. version all affected schemas, codecs, replay records, capability declarations, and verifier reports so obsolete and reviewed evidence cannot mix.

## 1. Reviewed provider and schema versions

Retain the original provider set, but assign new nonzero reviewed versions wherever observable evidence or compatibility changes. Names may follow local registries, but use version distinctions conceptually equivalent to:

```text
obligation_import_provider:          component12_dimensional_handoff_import_v2
candidate_provider:                  obligation_seeded_dimension_checked_actions_v2
coordinate_provider:                 component03_truth_layer_bounded_segment_v2
retriangulation_provider:            truth_layer_interface_preserving_local_cell_v2
feature_cost_provider:               dimensioned_piecewise_affine_deviation_v2
budget_provider:                     component03_dimensioned_outward_sum_v2
certificate_provider:                truth_layer_dimensioned_patch_delta_v2
artifact_schema:                     cleaned_triangle_manifold_v2
obligation_schema:                   cleanup_obligation_v2
candidate_schema:                    cleanup_candidate_v2
action_certificate_schema:           cleanup_action_certificate_v2
budget_schema:                       cleanup_budget_evidence_v2
codec_schema:                        cleaned_triangle_manifold_codec_v2
replay_schema:                       cleanup_replay_v2
verification_provider:               independent_truth_dimension_replay_v2
```

Unchanged purely combinatorial providers may retain their existing version only when their serialized inputs and outputs are unaffected. Capability negotiation must reject a reviewed consumer combined with an obsolete Component 12 category, handoff, obligation, or predicate-evidence schema.

## 2. Corrected API and type compatibility

Use the internal entrypoint:

```cpp
template<class T, class I>
stage_outcome<artifact_handle<const cleaned_triangle_manifold<T>>>
build_cleaned_triangle_manifold(
    const boolean_context_view<T,I>& context,
    const precision_context_view<T>& precision,
    const source_triangle_complexes_view<T,I>& source_triangles,
    const canonical_source_manifolds_view<T,I>& source_manifolds,
    const canonical_intersection_complex_view<T,I>& intersections,
    const retained_surface_complex_view<T,I>& retained,
    const polygonal_output_complex_view<T,I>& polygonal,
    const triangulated_output_complex_view<T,I>& triangulated,
    const cleanup_capabilities<T,I>& capabilities);
```

All Component 12 imports, query views, codecs, fixtures, replay records, and mutation builders use `triangulated_output_complex<T,I>` / `triangulated_output_complex_view<T,I>`.

`cleaned_triangle_manifold<T>` remains independent of public index storage after intake. It must nevertheless encode the validated predecessor scalar/index descriptors and Component 14 compatibility so a cross-`I` artifact cannot be consumed accidentally.

## 3. File-level implementation amendments

Apply these additions to the original production file plan.

### 3.1 `CleanupTypes.h`

Add closed enums and strong IDs for:

- physical dimension, with cleanup-budget-facing production values equal to `length`;
- cleanup length metric;
- truth-layer availability and disposition references;
- Component 12 handoff-certificate references;
- advisory predecessor bound versus Component 13 proposed/reserved/committed cost roles;
- dimensional derivation kind;
- exact-relation misuse and uncertainty-not-proved rejection reasons; and
- corrected schema/version constants.

Do not encode units or metrics as free-form strings.

### 3.2 `CleanupPreflight.h/.cc`

Extend intake validation to:

- validate `T,I` descriptors across Components 11-13;
- validate reviewed Component 12 truth/category/handoff/obligation versions;
- reject mixed old/new schema combinations;
- verify every cleanup-facing Component 12 entity has one or more owner-correct finite length certificates;
- verify every certificate names a supported metric and Component 13 obligation schema;
- verify derivation and precision traces resolve through Component 03;
- verify Component 12 reserved and committed zero cleanup budget;
- verify exact-zero bookkeeping evidence includes topology-invariance proof; and
- preflight storage/work for truth records, dimension tags, certificate reverse maps, and independent derivation checks.

No mutable cleanup slot may be allocated before these checks pass.

### 3.3 `CleanupObligations.h/.cc`

Import and preserve:

- complete three-layer predecessor evidence;
- handoff-certificate IDs and reverse maps;
- metric, dimension, interval, derivation, and forbidden-merge fields;
- exact-zero bookkeeping and topology-invariance certificates;
- advisory-read-only disposition; and
- zero predecessor budget evidence.

An obligation cannot be created from small area, determinant magnitude, poor aspect ratio, nominal zero, or an untyped scalar threshold.

### 3.4 `CleanupCandidates.h/.cc`

Candidate generation requires a dimensionally valid obligation seed and exact topological action domain. Add preliminary rejection subcodes for missing/incompatible certificates, uncertainty not proving the required relation, invalid units, and unsupported dimensional derivations.

A candidate may compare preliminary costs only when the metrics and dimensions match and Component 03 proves disjoint ordered intervals. Otherwise scheduling uses canonical lineage keys.

### 3.5 `CleanupCoordinates.h/.cc`

Replace every reference to `exact-nominal evidence` with explicit Component 03 truth-layer views. Coordinate proposals record all three layers for support, residual, orientation, distance, and conditioning queries.

The bounded segment midpoint remains permitted, but its operation graph, nominal bits, uncertainty enclosure, construction uncertainty, and length-valued displacement are separate records.

### 3.6 `CleanupRetriangulation.h/.cc`

The narrow Component 12 local-cell adapter may reuse reviewed occurrence-preserving triangulation primitives only when it consumes the corrected truth-layer contract. It must not call obsolete exact-nominal branches or use area/tolerance comparisons.

Final triangle orientation is proved by the uncertainty enclosure. Small area and poor aspect ratio remain diagnostic unless a reviewed dimensional derivation produces a named length metric associated with a supported obligation.

### 3.7 `CleanupBudget.h/.cc`

Model all accepted cleanup costs with strong metric/dimension types. Separate:

- inherited and construction uncertainty;
- per-lineage displacement;
- exact-coverage zero-motion retriangulation;
- local feature-removal/deformation length;
- whole-component removal size;
- swept/support/clearance length; and
- topology-change authorization.

The Component 12 handoff certificate is never a reservation or commit. Component 13 builds an action-specific correspondence and computes a fresh proposal. Reservations and actual commits reference that proposal and may not cite an advisory bound as their sole calculation.

### 3.8 `CleanupCertificates.h/.cc`

Every action certificate additionally records:

- imported handoff certificate IDs and their advisory role;
- all decisive truth-layer records;
- metric and physical dimension for each action-facing value;
- dimensional derivation traces and denominator proofs;
- action-specific proposed, reserved, actual, and cumulative values;
- evidence that definite decisions follow from uncertainty enclosures;
- proof that no handoff value was promoted directly to authorization; and
- corrected schema/codec/replay versions.

### 3.9 `CleanedTriangleManifold.h`, `CleanupQueries.h`, and `CleanupCodec.h/.cc`

Add immutable sections and owner-checked views for:

- predecessor `T,I` compatibility;
- imported Component 12 truth and handoff records or stable references;
- certificate-to-obligation and action-to-certificate reverse maps;
- closed metric/dimension identifiers;
- advisory/proposed/reserved/committed/rejected cost roles;
- dimensional derivation evidence;
- corrected Component 14/15 compatibility; and
- reviewed producer/verifier reports.

The codec explicitly encodes these fields. It rejects unknown enum values, non-length budget-facing metrics, non-finite intervals, stale references, mixed versions, nonzero reserved fields, and contradictory roles.

### 3.10 `CleanupVerifier.h/.cc`

The verifier independently reconstructs the corrected evidence and must not rely on producer unit tags, category labels, or summary maxima. It:

1. validates Component 12/13 schema and `T,I` compatibility;
2. checks every handoff certificate and reverse map;
3. confirms Component 12 used zero cleanup budget;
4. reconstructs truth-layer evidence where capabilities require it;
5. checks every definite action decision follows from an uncertainty enclosure;
6. validates dimensions and derivations independently;
7. recomputes action correspondence, displacement, removal, clearance, reservations, actual commits, and cumulative totals;
8. rejects advisory-cost promotion;
9. verifies thin-feature retention/removal reasons; and
10. rejects producer-shaped mutations with corrected superficial counts and digests.

## 4. Corrected intake algorithm

Before mutable import:

1. validate context owner, operation, policies, floating profile, `T,I`, and capability versions;
2. validate Component 12's reviewed artifact, predicate, category, handoff, residual, codec, and verifier schemas;
3. validate every triangle/residual/obligation range and canonical reverse map;
4. validate every decisive `rounded_nominal`, `exact_relation`, and `uncertainty_enclosure` record;
5. validate every exact-relation-zero bookkeeping disposition and topology-invariance certificate;
6. validate every handoff metric is dimensionally length, finite, and supported;
7. validate derivation traces and strictly positive denominator lower bounds where used;
8. validate every obligation names a supported Component 13 action schema and forbidden quotient set;
9. validate Component 12's zero budget-use certificate;
10. derive checked counts, bytes, and work for corrected evidence; and
11. only then construct the private cleanup complex.

A malformed or incompatible handoff is a contract failure. A contradiction in a committed predecessor artifact is an invariant failure. A valid but unsolved obligation remains an expected cleanup failure.

## 5. Truth-layer decision algorithm

For each authoritative Component 13 orientation, distance, residual, support, clearance, or intersection query:

1. form one canonical query key with formula version, strong-ID operands, role, orientation parity, and consumer-independent semantics;
2. request Component 03's frozen rounded graph, exact relation when supported, and uncertainty enclosure;
3. validate finite values, ownership, versions, nominal containment where applicable, and precision traces;
4. accept a definite relation only when the uncertainty enclosure proves it;
5. preserve exact-relation evidence for diagnostics and supported bookkeeping, never as a replacement for enclosure proof;
6. reject an action when uncertainty admits a topology, orientation, coverage, embedding, clearance, or budget-changing alternative;
7. store one immutable evidence record reused by all dependent action decisions; and
8. encode the final disposition and failure arbitration key.

No consumer recomputes the same relation using a different algebraic grouping or operand order. No candidate waits for floating arithmetic to change by iteration.

## 6. Dimensionally valid action-cost algorithm

### 6.1 Imported evidence

A Component 12 handoff certificate enters the obligation graph as an immutable advisory witness. It can establish that a named length-valued condition deserves review. It cannot establish action topology, policy permission, or cost.

### 6.2 Action-specific topology and correspondence

For every candidate:

1. reconstruct the exact closed patch, exterior interface, links, fans, components, occurrence restrictions, and protected provenance;
2. simulate the proposed quotient, split, retriangulation, fold removal, chain removal, or component removal;
3. reject any action that violates manifoldness, occurrence separation, selected boundaries, or V1 topology policy;
4. construct a complete old/new patch correspondence;
5. compute bounded per-lineage displacement and feature-removal deviation with Component 03; and
6. separately compute swept/support/clearance and component-size metrics required by policy.

### 6.3 Dimensional checks

Every proposal value carries a metric and dimension. Compare it with caller tolerance only when the policy declares that metric as a length-valued authorization. Area, determinant, squared length, angle, quality, topology counts, and nominal volume cannot be substituted.

An area-derived altitude uses a reviewed outward-rounded formula only with a finite strictly positive base-length lower bound. If the denominator interval contains zero, try another reviewed metric or reject the proposal.

### 6.4 Reservation and commit

Create a Component 03 budget proposal from the action-specific evidence. Reserve before mutation. After all topology and geometry checks, compute the actual conservative cost and require it to be no greater than the reservation. Atomically commit topology, precision, and budget, or roll all of them back.

Record handoff advisory bound, action proposal, reservation, actual commit, and cumulative per-lineage total as distinct immutable records.

## 7. Candidate keys and scheduler amendments

Extend complete keys with:

```text
obligation_key +=
    (component12_truth_schema,
     handoff_certificate_metric,
     handoff_certificate_key,
     derivation_key,
     forbidden_quotient_digest)

candidate_key +=
    (action_cost_metric,
     action_cost_dimension,
     truth_evidence_digest,
     action_formula_versions)

action_key +=
    (selected_proposal_key,
     reservation_key,
     actual_commit_key,
     dimensional_certificate_digest)
```

Numerical intervals participate in scheduling only for the same metric/dimension and only when definitively ordered. Otherwise canonical topology and lineage determine order.

Candidate generation remains obligation-driven. Do not generate a quality-only collapse, flip, smoothing move, or removal merely because an advisory handoff bound is small.

## 8. Atomic action transaction amendments

Insert these required checks into the original action sequence before budget reservation and mutation:

- validate imported handoff evidence and its advisory role;
- reconstruct all required truth-layer records;
- validate action metric/dimension compatibility;
- construct and independently check dimensional derivations;
- compute action-specific correspondence and costs;
- compare only named length metrics with policy budgets; and
- prove no valid thin feature is being removed solely for small area or quality.

After replacement construction, independently recompute affected truth layers, dimensional costs, intersection/clearance evidence, and cumulative displacement before commit.

Failure at any phase releases all reservations and leaves the mutable state, evidence tables, candidate generations, and obligation graph byte-equivalent to the pre-action state.

## 9. Final artifact and downstream handoff

The final `cleaned_triangle_manifold<T>` publishes:

- finite authoritative bounded coordinates and fresh precision records;
- reciprocal pairs, exactly two edge uses, definite outward triangles, and one closed fan per occurrence;
- no residual cells or pending cleanup obligations;
- complete imported handoff and truth evidence;
- complete action-specific dimensioned proposals, reservations, commits, and cumulative totals;
- all dimensional derivations and witness keys;
- corrected Component 12/13/14/15 compatibility versions;
- canonical bytes, digests, diagnostics, and replay; and
- an independent verifier report covering the corrected contracts.

Component 14 may assemble and canonicalize but must not reinterpret units, choose a different action metric, lower a bound, or promote advisory evidence. Component 15 independently verifies the public-facing topology, budget, precision, forbidden intersections, Boolean side semantics, and evidence completeness.

## 10. Existing Ygor reuse decisions

The review confirms the original plan's greenfield internal cleanup core is necessary.

Do not use as Component 13 providers:

- `mesh_remesher<T,I>`: it mutates a public mesh in place, uses midpoint/valence/smoothing heuristics and raw nominal geometry, and lacks occurrence lineage, exact link conditions, budgets, remote-intersection checks, replay, and transactional publication;
- `fv_surface_mesh<T,I>` duplicate/degenerate/simplification helpers: they operate after loss of paired-edge and occurrence identity and may merge or delete features without Component 03 certificates;
- legacy `YgorMeshesBoolean{,2,3,4,5}` cleanup/output paths: they use coordinate-derived identity, snapping or ordinary floating policy, and do not implement the bounded cleanup contract;
- public hole filling, orientation, refinement, zippering, BSP cleanup, or tetrahedralization repair utilities; and
- public spatial indexes as authoritative relation providers.

Reuse and improve only narrow contract-compatible infrastructure:

- Component 03 truth layers, bounded operations, precision ledger, finite AABBs, relation services, and dimensioned budget service;
- Component 12 occurrence-preserving local triangulation primitives through a corrected narrow adapter;
- Component 06/03 rank-Morton and finite-AABB primitives as semantics-free accelerators;
- Component 01 IDs, checked counts, resources, cancellation, transactions, deterministic failure arbitration, canonical bytes, SHA-256, diagnostics, and replay; and
- Component 17 deterministic task infrastructure while retaining serial semantic references.

Do not create a second interval package, physical-unit system, exact-arithmetic package, tolerance ledger, or symbolic policy inside Component 13.

## 11. Test and qualification amendments

Add these gates to Section 24 of the original plan.

### 11.1 Contract and schema tests

Cover every supported `T,I` combination and deliberate descriptor mismatch. Reject unknown, zero, obsolete, and mixed Component 12/13 schema versions before mutable allocation.

### 11.2 Truth-layer tests

Exercise every representable combination needed by cleanup queries, including:

- rounded nominal zero/nonzero;
- exact relation negative/zero/positive/unavailable; and
- uncertainty enclosure negative-only, zero-only, positive-only, zero-containing, and sign-spanning.

Require acceptance only from the enclosure-proved relation.

### 11.3 Handoff-certificate tests

Test valid and malformed certificates: missing, duplicate, stale owner, stale trace, non-finite, wrong dimension, wrong metric, incompatible obligation schema, hidden positive-area alternative, and false budget use.

### 11.4 Advisory boundary tests

Provide small valid handoff bounds paired with:

- failed link conditions;
- prohibited occurrence merges;
- protected source/carrier features;
- remote intersections;
- ambiguous orientation; and
- over-budget action-specific correspondence.

The handoff certificate must not authorize any action.

### 11.5 Dimensional mutation tests

Mutate length into area, squared length, determinant, angle, or aspect ratio; compare area directly with tolerance; remove the positive denominator proof; or relabel units while adjusting scalar values and digests. Required mutation survivor count is zero.

### 11.6 Scaling metamorphic tests

Under power-of-two scale `s`, verify:

- lengths and tolerance scale by `s`;
- areas and squared lengths scale by `s^2`;
- higher determinant quantities scale by their correct physical dimension;
- dimensionless quality remains dimensionless; and
- action selection, topology, and success/failure remain equivalent after documented remapping.

### 11.7 Thin-feature tests

Include definite positive but tiny-area triangles, thin corridors, narrow shells, and poor-aspect-ratio features that satisfy final validity and therefore remain. Include nearby fixtures whose uncertainty enclosures fail orientation/clearance and therefore fail or require an independently authorized action.

### 11.8 Independent action-cost tests

Compare imported handoff bounds, action proposals, reservations, actual commits, and exact-oracle costs. Verify they are distinct records and that no advisory value is silently promoted.

### 11.9 Downstream handoff tests

Component 14 and Component 15 fixtures must reconstruct every metric, dimension, truth-layer, derivation, and advisory/committed role from immutable Component 13 views. Missing evidence must fail closed.

## 12. Implementation sequence amendments

Modify the gates in Section 25 as follows:

1. **Schemas, versions, and API:** include corrected `T,I` compatibility, truth layers, dimensions, handoff certificates, and mixed-schema rejection.
2. **Predecessor intake:** require complete Component 12 certificate and truth-evidence audits before mutable import.
3. **Obligation graph:** preserve certificate reverse maps and reject non-dimensional obligation seeds.
4. **Candidate scheduler:** permit numerical ordering only for same-metric disjoint intervals.
5. **Coordinate/retriangulation providers:** use uncertainty-enclosure acceptance and no area/tolerance branches.
6. **Patch correspondence, budget, and precision:** use strong dimensioned records and independently recomputed action cost.
7. **Interaction checks:** encode truth layers and length-valued clearance.
8. **Certificates/codec/replay:** encode all corrected evidence and version splits.
9. **Independent verifier:** reject truth conflation, dimensional misuse, and advisory-cost promotion.
10. **Qualification:** add all tests in Section 11 of this amendment before Component 14 implementation begins.

## 13. Amended definition of done

In addition to Section 26 of the original plan, implementation is not complete until:

- all Component 12 inputs use the reviewed `T,I` artifact/view type;
- obsolete exact-nominal semantics are absent from production and normative tests;
- every decisive query preserves all three numerical truth layers;
- every cleanup-facing threshold/cost is dimensionally valid and caller tolerance is used only as a length;
- every imported cleanup obligation has a valid length-valued handoff certificate;
- every action recomputes its own exact topology, correspondence, displacement, removal, swept, clearance, and budget evidence;
- no handoff certificate is promoted directly to authorization or committed cost;
- valid thin features are retained when they satisfy the final contract;
- corrected artifacts, codecs, replay payloads, and verifier reports reject obsolete or mixed schemas;
- Component 14/15 receive complete immutable evidence without reinterpretation; and
- the complete truth-layer, dimensional, mutation, scaling, type-matrix, and advisory-bound qualification suite passes with zero required mutation survivors.
