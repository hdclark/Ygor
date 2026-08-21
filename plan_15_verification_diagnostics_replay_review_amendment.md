# Plan 15 Review Amendment: Independent Verification, Diagnostics, and Replay

## Status, precedence, and implementation intent

This file is a normative implementation-plan amendment to `plan_15_verification_diagnostics_replay.md`. It records the independent Component 15 review required by `tracker.md` and integrates the reviewed Component 13 numerical/dimensional cleanup contract, the reviewed Component 14 canonical-content/correspondence contract, Component 16 qualification boundaries, Component 17 deterministic execution services, and suitable existing Ygor verification functionality.

The original plan remains normative except where this amendment is more specific or conflicts with it. In a conflict, this amendment controls. Implementers, reviewers, test authors, codec authors, and public-result/replay consumers must read both files and the corresponding Component 15 specification amendment.

The original architecture remains suitable: one private final transaction, exact public-mesh readback, independent public topology reconstruction, structural internal/public proof, independently organized classification/selection checks, deterministic probes and operand occupancy, independent output-interaction search, construction and cleanup audit, primitive-ledger aggregation, report regeneration, fresh re-ingestion, canonical findings/replay, executable serial reference, deterministic merge, atomic publication, strict C++17, and no external dependencies.

This amendment corrects the reviewed integration defects without weakening that architecture:

1. consume reviewed Component 12/13/14 schemas and validated `T,I` compatibility;
2. preserve `rounded_nominal`, `exact_relation`, and `uncertainty_enclosure` as orthogonal evidence;
3. enforce physical dimensions, closed metrics, derivations, and distinct cleanup-cost roles;
4. verify semantic public canonicalization separately from presentation correspondence and automorphism classes;
5. separate public-success semantic digests from invocation-bound replay digests;
6. make triangle, shell, probe, occupancy, and contact acceptance uncertainty-complete and dimensionally valid;
7. use non-circular semantic Component 02 re-ingestion; and
8. improve/reuse appropriate exact-index portions of `YgorMeshesVerification` rather than treating all existing verification code as disposable.

## 1. Reviewed providers and schema versions

Retain original providers where semantics remain valid, but assign new nonzero reviewed versions wherever handoff, truth, dimension, correspondence, digest, or re-ingestion behavior changes. Names may follow local registries, but use distinctions conceptually equivalent to:

```text
predecessor_contract:              component14_reviewed_output_handoff_v2
candidate_schema:                  assembled_output_candidate_v2
verification_input:                reviewed_truth_dimension_correspondence_v2
public_incidence_primitive:        exact_index_directed_use_v2
public_topology_reconstruction:    sorted_use_pair_link_component_v2
semantic_canonical_verification:   public_minimum_and_orbits_v2
presentation_correspondence:       permitted_orbit_bijection_v2
triangle_geometry:                 three_layer_dimensioned_orientation_v2
shell_side:                        bounded_occupied_side_evidence_v2
probe_policy:                      dimensioned_clearance_cover_v2
operand_occupancy:                 bounded_half_open_ray_winding_v2
verification_spatial_index:        closed_aabb_rank_median_v2
forbidden_pair_relation:           uncertainty_authorized_support_v2
construction_audit:                reviewed_truth_dimension_residual_v2
cleanup_audit:                     reviewed_action_role_replay_v2
precision_aggregation:             typed_metric_role_path_reduction_v2
report_schema:                     semantic_presentation_verification_report_v2
reingestion:                       published_operand_reingestion_v2
finding_schema:                    class_aware_verification_finding_v2
logical_serialization:             semantic_presentation_domains_v2
result_schema:                     verified_boolean_result_v2
replay_schema:                     source_bound_verification_replay_v2
self_audit:                        reviewed_final_envelope_audit_v2
execution_reference:               serial_complete_final_verification_v2
```

Capability negotiation must reject:

- reviewed Component 15 with obsolete Component 12/13/14 truth, dimension, role, correspondence, codec, replay, or verifier schemas;
- cross-`T` or cross-`I` artifacts;
- a candidate codec lacking metric/dimension/role or correspondence-class sections;
- a Component 02 re-ingestion capability that is repair-oriented, recursive, or presentation-ID-dependent;
- zero, unknown, mixed, or contradictory versions before authoritative allocation.

Unchanged Component 01 checked arithmetic, canonical bytes, SHA-256, resource, transaction, replay, and deterministic arbitration providers may retain versions only when their observable input/output contracts are unchanged.

## 2. Corrected entrypoint and intake

Use an API conceptually equivalent to:

```cpp
template<class T, class I>
stage_outcome<artifact_handle<const verified_boolean_result<T,I>>>
verify_and_publish_boolean_result(
    const boolean_context_view<T,I>& context,
    const precision_context_view<T>& precision,
    const assembled_output_candidate_view<T,I>& candidate,
    const final_verification_dependencies_view<T,I>& predecessors,
    const final_verification_capabilities<T,I>& capabilities);
```

Before reading semantic arrays or allocating large workspaces, validate:

- candidate/predecessor `T,I` descriptors;
- context owner, stable context digest, operation, policies, platform, and floating environment;
- complete Component 01-14 dependency chain and immutable lifetimes through join/commit/rollback;
- reviewed schemas and provider compatibility;
- pending-only topology/geometry/result statuses;
- absence of Component 15 success authority or final semantic digest;
- exact evidence ranges and reverse maps;
- all three truth layers for decisive records;
- metric, dimension, derivation, denominator, role, and tolerance class for every tolerance-facing record;
- Component 12 zero-budget-use evidence;
- Component 13 action/budget role transitions and committed records;
- Component 14 semantic canonicalization evidence, orbit/equivalence classes, and concrete presentation map;
- semantic versus presentation/replay domain separation; and
- checked count, byte, index, task, pair, probe, finding, replay, and work bounds.

Mixed reviewed/obsolete evidence or missing mandatory records fails before large allocation. A digest match is never evidence completeness.

## 3. File-level implementation amendments

Apply the following additions/corrections to Section 2.1 of the original plan.

### 3.1 `FinalVerificationTypes.h`

Add closed enums and strong IDs for:

- numerical truth layer and availability;
- physical dimension and closed metric;
- quantity role: advisory, proposed, reserved, committed, rejected, rolled-back, one-action, cumulative-lineage, local-removal, component-removal, swept/support/clearance, representation-effect, tolerance;
- dimensional derivation and denominator certificate;
- semantic canonical class, orbit, equivalence class, and presentation correspondence;
- semantic versus presentation report/codec/digest sections;
- authorized support simplex/class;
- re-ingestion semantic comparison disposition; and
- reviewed provider/schema constants.

Reserve zero for invalid. Reject unknown enums and nonzero reserved fields. Do not use free-form strings as truth, unit, metric, role, class, or authorization authority.

### 3.2 `VerificationPreflight.h/.cc`

Extend preflight to validate the complete reviewed version graph, `T,I`, pending statuses, three-layer evidence, metric/dimension/derivation/role tables, Component 12 zero budget, Component 13 committed action evidence, Component 14 semantic/orbit/presentation evidence, and Component 02 re-ingestion capability.

Build an evidence-completeness table covering every public vertex, facet, directed use, edge, component, shell, event, retained atom, cleanup action, precision path, report maximum, correspondence class, and replay reference.

Preflight all counts/products/prefix sums/byte sizes/ranges/sentinels and reserve hard resources before allocation. No mandatory gate may be truncated or skipped because an advisory target is exceeded.

### 3.3 `VerificationPublicMesh.h/.cc`

Keep exact const readback. Verify exact bit records, triangle rings, checked indices, distinct triangle indices, empty semantics, and absence of adapter normalization. Optional public arrays remain empty under V1.

Do not infer identity from coordinates or call public mesh mutators.

### 3.4 Improve/factor `YgorMeshesVerification.h/.cc`

Do not treat the existing implementation as merely fixtures. Preserve its public bool APIs, but factor or add a semantics-free read-only exact-index incidence layer with:

- complete directed-use record `(from,to,facet,corner)`;
- exact undirected endpoint key;
- deterministic sorted grouping;
- opposite-direction and edge-overuse witnesses;
- exact corner-incidence records usable for link reconstruction;
- checked count/index conversions; and
- no coordinate equivalence, epsilon, normal, or geometry decision.

Component 15 may reuse tuple construction, exact key helpers, and read-only range validation. It must still own independent grouping, link traversal, component reconstruction, evidence, findings, and arbitration. Component 14's high-level verifier must not share Component 15's complete reconstruction control flow.

Keep `TriangleIsDegenerate` and current nominal boolean checks as legacy/secondary APIs; they are not publication authorities.

### 3.5 `VerificationTopology.h/.cc`

Retain independent exact-index reconstruction:

- emit and sort `3F` directed uses;
- group exact index endpoint pairs;
- require exactly two opposite uses;
- reconstruct reciprocal pairs;
- reconstruct each vertex link as one closed corner cycle;
- reject bow-ties even when all edges are two-use;
- reconstruct components and shells;
- propagate orientation consistency;
- compute checked Euler/genus summaries; and
- retain canonical witnesses.

Coordinate-equal distinct indices remain distinct. Compare Component 13/14 claims only after reconstruction.

### 3.6 Split `VerificationBijection.h/.cc`

Replace the original rule that refines ambiguity, chooses a representative, and then requires stored-map equality.

Implement two proofs:

1. **semantic canonical-content proof**
   - reconstruct Component 14's public-semantic incidence model independently;
   - verify the selected canonical public content is the minimum required by the reviewed labeling contract;
   - verify public bits, oriented triangles, occurrence/multiplicity roles, component order, and public-content bytes/digest;
   - exclude detailed source/action/presentation identities from the public semantic domain.

2. **correspondence-class and concrete-map proof**
   - validate every `correspondence_equivalence_class` member set, public-position set, normalized payload, incidence/orientation constraints, and orbit certificate;
   - prove uniquely distinguished records have exact maps;
   - prove automorphism-equivalent records admit the claimed permitted bijections;
   - prove the candidate concrete map is total, bijective, owner/range/topology correct and belongs to the permitted set;
   - derive triangle, directed-use, edge, component, and shell correspondence consistently from one vertex/graph correspondence;
   - prove representative substitution cannot alter semantic bytes, reports, findings, or status.

Never select a representative by cleaned ID, public position, source order, traversal, allocation history, hash/pointer, task order, or search branch order.

### 3.7 `VerificationTriangleGeometry.h/.cc`

For each triangle, preserve and audit `rounded_nominal`, `exact_relation`, and `uncertainty_enclosure` separately. Evaluate projected orientations through fixed Component 03 expression graphs. Select projection only through conservative comparisons among same-dimension values. Require the enclosure to prove the prescribed nonzero orientation.

Do not compare area, determinant magnitude, angle, aspect ratio, or squared length directly with caller tolerance. A required thickness/altitude check must consume a reviewed length-valued Component 03 derivation with a strictly positive certified denominator. Component 13 must already have resolved cleanup obligations.

Shell orientation propagation establishes consistency only. Absolute occupied-side/nesting evidence must be bounded and uncertainty-complete. Signed volume is optional `length_cubed` diagnostic evidence, never a direct length-tolerance comparison or sole side authority.

### 3.8 `VerificationLineage.h/.cc`

Retain complete Component 07-14 trace coverage. Add:

- reviewed truth/dimension/role references;
- correspondence class/orbit membership;
- unique versus invariant-class provenance disposition;
- presentation-only concrete map references;
- proof that distinct equal-bit event/occurrence identities remain separate; and
- representative substitution invariance.

Missing unique attribution inside an exact class is not a failure when invariant class provenance is complete. Fabricated unique attribution is a failure.

### 3.9 `VerificationClassification.h/.cc` and `VerificationSelection.h/.cc`

Retain independent zero-delta graph reconstruction, quotient potentials, seed winding, truth-table selection, orientation, multiplicity, and coincident ownership.

All bounded relations used to justify graph edges or side transitions must preserve the reviewed three-layer evidence. Exact relation over nominal operands cannot override an enclosure admitting a different crossing/contact disposition.

Semantic keys for symmetric output regions must use class/orbit keys rather than arbitrary concrete representatives.

### 3.10 `VerificationProbePolicy.h/.cc`

Retain canonical positive barycentric anchor candidates and fixed primitive integer directions, but add typed evidence:

- anchor interior enclosure;
- definite direction/orientation relation;
- positive length-valued offset interval;
- lower bound exceeding coordinate/probe uncertainty and representation floors;
- upper bounds from local prism, intended support, unrelated geometry, and policy clearance;
- derivation/metric/dimension records; and
- exact coverage and fallback attempts.

Enumerate dyadic offsets deterministically. Do not normalize directions or use a universal epsilon. Select the minimum complete accepted attempt key after canonical merge, never first worker completion. If no required region has a safe probe, fail.

### 3.11 `VerificationOperandOccupancy.h/.cc`

Build independent source-operand query structures from validated immutable source triangles. Use Component 03 bounded relations, checked signed crossings, and the frozen half-open feature-ownership rule.

A ray result is accepted only when uncertainty proves the crossing set and final winding. Symbolic tie handling applies only to exact eligible ties. Ordinary uncertainty tries the next canonical ray or fails. No call to `YgorMeshesOrient` nominal ray routines is permitted.

### 3.12 `VerificationAabbTree.h/.cc`

Retain the independent top-down rank-median tree. Every primitive/node bound is a Component 03 conservative closed AABB with explicit length dimension and required precision inflation.

Before keeping greenfield code, document assessment of existing BSP/R-tree/octree/cell helpers. Reuse one only if a capability proof establishes conservative closed bounds, deterministic build/traversal, no nominal pruning, no proximity identity, complete counters, owner/version/resource/replay support, and independence from producer control flow. Otherwise retain the planned tree.

Compare accelerated candidate sets with exhaustive unordered all-pairs for bounded Component 16 fixtures. Large disjoint cases must not perform all-pairs narrow phase.

### 3.13 `VerificationTriangleRelations.h/.cc`

For each candidate pair construct:

```text
possible_support = support admitted by uncertainty enclosures
definite_support = support proved present
exact_authorization = support permitted by exact public topology
policy_authorization = reviewed coincident/same-patch ownership support
```

Shared-edge authorization permits only the exact shared edge/endpoints. Shared-vertex authorization permits only the exact vertex. Equal coordinate bits with distinct indices are disjoint absent explicit lineage authorization.

Accept only when `possible_support` is a subset of exact/policy authorization and no forbidden definite support exists. Definite excess is forbidden; unresolved excess fails closed. AABB overlap and proximity are not authorization.

Do not call Component 07 high-level control flow. Shared Component 03 primitives are allowed, but the pair decision graph must be independently organized and mutation-tested.

### 3.14 `VerificationConstructionAudit.h/.cc`

Replace obsolete exact-nominal language with three-layer evidence. Validate formula, rounding, conditioning, parentage, envelope dominance, metric, dimension, derivation, and role.

Do not accept a construction because the nominal point satisfies a residual when the enclosure does not. A tolerance-facing area/determinant residual requires a reviewed conversion to a length-valued support/residual bound.

### 3.15 `VerificationCleanupReplay.h/.cc`

Replay from the reviewed `triangulated_output_complex<T,I>`, not Component 13 final topology or cumulative summaries. Preserve reviewed truth-layer records, length certificates, role transitions, action-specific correspondence, and correspondence classes.

For each action verify exact before state, topology eligibility, occurrence splitting, replacement support, dimensioned displacement/removal/swept/clearance costs, reservation/commit/rollback, no new interaction, exact after state, and obligation updates.

Do not rediscover actions. Component 12 handoff certificates remain advisory. Compare the final replayed complex with Component 13 and Component 14 under exact unique mappings plus verified automorphism classes.

### 3.16 `VerificationPrecisionAudit.h/.cc`

Use strong typed records:

```text
dimensioned_verification_value =
    (role,
     metric,
     physical_dimension,
     conservative_interval_or_bound,
     derivation_reference,
     ledger_or_action_reference,
     semantic_entity_or_equivalence_class)
```

Maintain separate maxima for input precision, construction uncertainty, cleanup uncertainty, representation effect, one-action displacement, cumulative lineage displacement, local removal, component removal, support/clearance, and each tolerance class.

Only compare compatible metric/dimension values. Equal maxima use complete typed witness keys. Advisory records cannot drive committed maxima; rolled-back records cannot contribute to success.

### 3.17 `VerificationReports.h/.cc`

Split final records into:

- semantic public reports/status/evidence;
- correspondence equivalence and invariant provenance;
- presentation-only concrete correspondence/replay data; and
- non-authoritative diagnostics.

Semantic reports contain topology, geometry, truth-layer coverage, typed maxima, probe/pair evidence, cleanup/precision evidence, re-ingestion, resources, determinism, and accepted exceptions. Presentation-only changes must not alter semantic report bytes or status.

### 3.18 `VerificationCodec.h/.cc`

Define separate framed domains:

1. public mesh content;
2. public semantic topology/geometry reports;
3. semantic class/orbit/provenance evidence;
4. complete verification artifact/evidence;
5. presentation correspondence;
6. source-bound replay;
7. findings/truncation;
8. re-ingestion;
9. pass evidence; and
10. aggregate verified result.

The ordinary public digest is the public-success semantic digest. Presentation correspondence and source input order are excluded from it. Invocation replay binds exact source inputs/options and the concrete presentation map and may differ for semantically equivalent invocations.

Use Component 01 canonical bytes and SHA-256 only. Full bytes/structure remain authoritative under collisions.

### 3.19 `VerificationReingestion.h/.cc`

Use a reviewed `published_operand_reingestion_v2` Component 02 capability. It executes the ordinary path required for later Boolean operands, including structural/solid/geometry checks and Component 02 local verification, but does not recurse into Component 15.

Do not reuse Component 14 round-trip state, Component 15 topology arrays, producer IDs, repair, normalization, deduplication, or orientation mutation.

Compare exact public bits/cycles, edge/link/component/shell semantics, occupied-side/nesting, duplicate-coordinate occurrence separation, semantic canonical classes/orbits, and imported precision. Do not require transient IDs or a concrete automorphism representative to match.

### 3.20 `VerificationFindings.h/.cc`

Semantic finding keys use canonical semantic entity/equivalence-class/orbit keys. Presentation-map failures may attach exact source-bound map excerpts, but the semantic arbitration prefix must not depend on an arbitrary representative.

Retain deterministic sorting, full-content coalescing, primary-slot reservation, explicit truncation, exact numerical witnesses, and focused replay selectors.

### 3.21 `VerifiedBooleanResult.h`, `FinalVerification.h/.cc`, and `VerificationSelfAudit.h/.cc`

The verified result is private-factory-only and non-default-constructible. The factory requires:

- exact mandatory completion mask;
- no rejecting findings;
- verified semantic canonical content;
- valid correspondence classes and concrete map membership;
- completed dimensioned geometry/probe/pair/construction/cleanup/precision gates;
- semantic reports and public-success digest;
- source-bound replay;
- non-circular re-ingestion;
- reconciled resources and joined workers;
- accepted self-audit; and
- final cancellation poll.

Self-audit validates semantic/presentation domain separation, unchanged Component 14 public mesh, complete class/orbit evidence, final status authority, resource reconciliation, codec/digest references, and success-factory exclusivity. It does not repair or rerun missing gates.

## 4. Corrected checkpoint order

Replace the original stable checkpoint list with:

1. capability/owner/type validation;
2. reviewed version and pending-status audit;
3. dependency/evidence graph audit;
4. truth/dimension/role/correspondence preflight;
5. checked count/index/byte/work preflight;
6. aggregate resource reservation;
7. public lexical readback;
8. directed-use extraction/grouping/pairing;
9. vertex links;
10. components/shells/orientation consistency;
11. semantic canonical public-content proof;
12. correspondence class/orbit proof;
13. concrete presentation-map membership proof;
14. three-layer triangle geometry;
15. occupied shell-side evidence;
16. lineage and class-level provenance audit;
17. classification groups;
18. quotient potentials/winding;
19. selection/orientation/multiplicity;
20. independent verification hierarchy build/audit;
21. forbidden pair enumeration;
22. uncertainty/authorization pair classification;
23. probe coverage planning;
24. anchor/direction/dimensioned offset construction;
25. probe clearance;
26. independent operand occupancy;
27. probe acceptance;
28. construction audit;
29. cleanup replay/final correspondence comparison;
30. precision/tolerance aggregation;
31. semantic/presentation report regeneration;
32. semantic and presentation codec/digest audit;
33. fresh semantic Component 02 re-ingestion;
34. findings and mandatory completion evidence;
35. source-bound replay finalization;
36. private verified-result proposal;
37. final self-audit;
38. resource reconciliation;
39. worker join;
40. final cancellation poll; and
41. atomic commit or deterministic failure.

Poll cancellation at every checkpoint and deterministic work interval, never by time.

## 5. Existing Ygor reuse matrix

| Existing facility | Suitable reuse | Prohibited authority |
|---|---|---|
| `fv_surface_mesh<T,I>` | exact final carrier and const readback | mutating repair, normalization, derived caches |
| `YgorMeshesVerification` | improve/factor exact lexical/index/directed-use primitives | nominal degeneracy or bool-only final publication gate |
| `YgorMeshesOrient` | fixtures and non-authoritative comparisons | proximity identity, shell side, rays, contact, publication |
| BSP/R-tree/octree/cells | reuse only after conservative deterministic capability proof | nominal pruning, uninflated bounds, insertion-order semantics |
| Component 01 services | IDs, errors, transactions, resources, bytes, SHA-256, replay, arbitration | duplicating infrastructure |
| Component 03 services | bounded arithmetic, truth layers, typed dimensions/metrics/ledgers | producer high-level grouping/control flow |
| Components 09/10 | immutable claims to compare | direct reuse as independent classification/selection proof |
| Components 13/14 | immutable evidence/candidate | trusting summaries/maps/status as final authority |
| Component 17 | deterministic worker/task/merge services when integrated | a second unbounded scheduler or schedule-dependent semantics |

Maintain an include/dependency matrix proving verifier translation units do not include producer-private grouping, selector, canonical-map, search, cleanup-reducer, report-builder, or scheduler headers for the fact they independently check.

## 6. Required tests and mutations

### 6.1 Reviewed handoff and status

Test mixed reviewed/obsolete schemas, cross-`T`, cross-`I`, missing truth/dimension/role/class records, stale dependencies, illegal final status, premature digest/conversion token, invalid lifetimes, and insufficient resources. Reject before large allocation.

### 6.2 Truth-layer mutations

Construct cases where:

- rounded nominal is zero but exact relation is nonzero;
- exact relation is zero but uncertainty spans both signs;
- exact relation is nonzero but uncertainty includes zero;
- producer disposition follows nominal/exact relation despite unresolved enclosure; and
- symbolic tie is invoked without exact eligibility.

Repair counts/digests where practical. Component 15 must reject through the intended independent check.

### 6.3 Dimension, derivation, and role mutations

Mutate area as length, squared length as length, volume as length, angle as tolerance, untyped epsilon, hidden unit conversion, zero-containing denominator, wrong derivation, incompatible metric comparison, advisory-to-committed promotion, rolled-back contribution, one-step-versus-cumulative confusion, and local-versus-component removal confusion.

### 6.4 Automorphism and correspondence

Use exact symmetric components with several valid presentation correspondences. Require identical public content, semantic reports, semantic digest, and status across representative/source permutations.

Mutate class members, public-position sets, orbit certificate, incidence constraints, normalized lineage multiset, concrete map membership, induced facet/edge/component map, or semantic codec contamination by presentation data. Reject every invalid mutation without rejecting a valid alternative representative.

### 6.5 Geometry, probes, occupancy, and pair authorization

Add:

- dimensioned projection/triangle boundary tests;
- denominator-crossing-zero thickness derivations;
- no-safe-probe and third-surface tests;
- uncertainty wider than shared-edge/shared-vertex authorization;
- equal-coordinate disjoint occurrence contact;
- authorized/unauthorized coincident support;
- exact-tie versus ordinary-uncertainty ray cases; and
- accelerated versus exhaustive all-pairs comparisons.

### 6.6 Cleanup, precision, reports, and digests

Mutate truth layers, dimensions, roles, action ordering, reservation/commit/rollback, class correspondence through cleanup, final artifact, cumulative paths, maxima, report sections, domain separation, public semantic digest, replay digest, collision behavior, and final status.

### 6.7 Re-ingestion and existing utility tests

For every valid result, run ordinary-path Component 02 re-ingestion and a follow-on Boolean fixture. Test exact automorphism-equivalent canonicalization without transient-ID equality.

Test improved `YgorMeshesVerification` exact-index primitives for boundaries, same-direction uses, overuse, duplicate directed uses, exact-coordinate duplicate indices, arbitrary valence, and bow-ties that edge counts alone miss. Preserve existing bool API behavior.

Matched mutations must prove sharing low-level tuple/key utilities does not cause Component 14 and Component 15 to share the same high-level error.

### 6.8 Scaling metamorphics

Under exact power-of-two scale `s`, verify:

- length evidence scales by `|s|`;
- area and squared-length evidence scale by `s^2`;
- volume evidence scales by `|s|^3`;
- dimensionless evidence is unchanged;
- derived length tolerance evidence scales correctly; and
- semantic topology/status follows the frozen policy.

Every required mutation must be rejected by its intended check. Zero survivors remains mandatory.

## 7. Revised implementation sequence

Amend Section 24.13 of the original plan to:

1. reviewed schemas/versions/errors/result-factory skeleton — no unverified success path;
2. Component 14 reviewed handoff, `T,I`, truth/dimension/role/class preflight;
3. improve/factor `YgorMeshesVerification` exact-index primitives and preserve legacy API;
4. public lexical reader and malicious adapter tests;
5. independent edge/link/component/shell reconstruction;
6. semantic canonical-content proof;
7. correspondence-class/orbit/concrete-map proof;
8. dimensioned three-layer triangle/shell geometry;
9. reviewed lineage and class-level provenance audit;
10. classification quotient potentials;
11. selection/orientation/multiplicity/contact ownership;
12. independent closed-AABB tree and exhaustive-pair seam;
13. authorized-support uncertainty-complete pair graph;
14. dimensioned probes and independent operand occupancy;
15. reviewed construction truth/dimension audit;
16. reviewed cleanup replay and correspondence preservation;
17. typed metric/role ledger and budget aggregation;
18. semantic/presentation reports and completion evidence;
19. class-aware findings/arbitration/localization;
20. semantic versus presentation codecs/digests/replay;
21. non-circular semantic Component 02 re-ingestion;
22. verified factory/self-audit/public conversion paths;
23. deterministic parallel integration through Component 17 adapters when available; and
24. complete qualification with zero required mutation survivors.

Do not expose ordinary `bounded_boolean` success before Step 24 passes.

## 8. Amended definition of done

In addition to the original Section 25, Component 15 implementation is complete only when:

- the complete reviewed Component 12/13/14/15 version and `T,I` graph is enforced;
- every decisive check preserves and audits `rounded_nominal`, `exact_relation`, and `uncertainty_enclosure` separately;
- no final disposition uses exact relation or rounded nominal to override unresolved uncertainty;
- every tolerance-facing value has a supported metric, dimension, role, derivation, and witness;
- no area, squared length, determinant, angle, volume, or quality value is compared directly with length tolerance;
- advisory, proposed, reserved, committed, rejected, rolled-back, one-action, cumulative, local-removal, and component-removal records remain distinct;
- semantic public canonicalization is independently verified;
- exact automorphism classes and orbit certificates are verified without fabricating a unique representative;
- the concrete presentation correspondence belongs to the permitted class and induces consistent maps;
- public semantic reports, status, and digest are invariant under valid representative substitution;
- invocation replay remains exact and source-bound without contaminating the public semantic digest;
- triangle, shell, probe, occupancy, and forbidden-contact decisions are uncertainty-complete and dimensionally valid;
- exact contact authorization limits possible support to the permitted simplex or reviewed coincident support;
- fresh Component 02 re-ingestion is ordinary-path, semantic, and non-circular;
- suitable exact-index functionality in `YgorMeshesVerification` is improved/reused while nominal/proximity legacy geometry remains non-authoritative;
- every new truth/dimension/automorphism/digest/re-ingestion mutation is rejected by its intended check;
- scaling tests respect each physical dimension; and
- all production and normative-test code remains self-contained strict portable C++17 with no external dependency.

`tracker.md` may mark Component 15 complete when this reviewed plan amendment and corresponding specification amendment are committed. That tracker mark records completion of the planning-review step, not implementation or release qualification.
