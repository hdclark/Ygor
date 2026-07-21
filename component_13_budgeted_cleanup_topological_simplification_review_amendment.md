# Component 13 Review Amendment: Budgeted Cleanup and Topological Simplification

## Status, precedence, and review conclusion

This file is a normative amendment to `component_13_budgeted_cleanup_topological_simplification.md` produced by the independent Component 13 review required by `tracker.md`.

The original specification remains normative except where this amendment is more specific or conflicts with it. In a conflict, this amendment controls. Implementers, reviewers, test authors, Component 14, and Component 15 must read both files together with the reviewed Component 12 specification and plan amendments.

The review found that the original Component 13 architecture is aligned with `broad_plan.md` and should be retained. In particular:

- cleanup remains obligation-driven and operates on a private paired internal complex rather than on `fv_surface_mesh<T,I>`;
- exact topology and immutable lineage establish eligibility before geometry or tolerance is considered;
- every atomic edit preserves reciprocal pairing, exactly two uses per edge, and one closed fan per topological vertex occurrence;
- displacement and feature-removal costs are reserved before mutation and committed transactionally through Component 03;
- topology-distinct equal-coordinate occurrences are never welded by proximity or coordinate equality;
- changed geometry receives fresh bounded precision, residual, orientation, and spatial-bound evidence;
- every action has deterministic replayable before/after evidence;
- final cleanup is independently verified before Component 14 receives an immutable artifact; and
- production and normative-test code remain strict portable C++17 with no external dependencies.

The review identified five mandatory integration corrections:

1. Component 13 must consume the reviewed concrete predecessor type `triangulated_output_complex<T,I>` / `triangulated_output_complex_view<T,I>`, not the obsolete one-parameter spelling.
2. Component 13 must preserve Component 03's orthogonal `rounded_nominal`, `exact_relation`, and `uncertainty_enclosure` layers and must not use obsolete `exact-nominal` semantics.
3. Caller tolerance and cleanup budget are lengths. Area, squared length, determinant magnitude, angle, aspect ratio, and other differently dimensioned quantities must not be compared directly with them.
4. Every Component 12 cleanup-required triangle or residual imported by Component 13 must carry a reviewed finite length-valued handoff certificate. That certificate is advisory eligibility evidence, not action authorization or committed budget.
5. Component 13's artifact, certificates, codecs, capability negotiation, verifier, and Component 14/15 handoff must identify corrected schema versions so reviewed and obsolete evidence cannot be mixed.

No correction authorizes Boolean reclassification, topology reconstruction from coordinates, tolerance welding, hidden geometry movement, external libraries, or reuse of incompatible public-mesh repair routines.

## A. Corrected predecessor and API contract

### A.1 Concrete reviewed inputs

The Component 13 entrypoint and every owner-checked view must use concrete types equivalent to:

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

The exact local names may differ, but scalar/index compatibility parameterization may not. Component-owned IDs remain owner-bound strong IDs and do not alias `I`; `I` records compatibility with predecessor occurrence and public-index descriptors.

Occurrences of `triangulated_output_complex<T>` in the original Component 13 specification are read as `triangulated_output_complex<T,I>`. The same rule applies to predecessor view spellings that omit `I` even though the reviewed producing component parameterizes them by `T,I`.

### A.2 Capability and version compatibility

`cleanup_capabilities<T,I>` must explicitly declare support for:

- the reviewed Component 12 artifact schema;
- the reviewed three-layer predicate-evidence schema;
- the reviewed dimensionally valid triangle-category schema;
- `cleanup_handoff_length_certificate` and its closed metric/dimension enums;
- the reviewed residual-obligation schema;
- the Component 12 zero-cleanup-budget-use certificate;
- the Component 13 obligation-import, action-certificate, budget, artifact, codec, replay, and verifier schemas; and
- the Component 14 and Component 15 versions that consume the corrected evidence.

Unknown, zero, obsolete, mixed, or mutually incompatible versions are rejected before mutable import. A reviewed truth-layer schema must never be combined with an obsolete category, obligation, certificate, or codec schema.

## B. Numerical truth-layer contract

### B.1 Orthogonal evidence

Every topology-affecting or geometry-acceptance query used by Component 13 must preserve these distinct facts from Component 03:

- `rounded_nominal`: the exact stored bits produced by the frozen rounded operation graph;
- `exact_relation`: the exact algebraic sign or zero of the declared relation over the stored nominal floating-point operands as exact real values, when supported; and
- `uncertainty_enclosure`: the conservative set of relation values admitted by inherited coordinate uncertainty and operation error.

These facts are not interchangeable:

- exact stored-nominal zero does not prove that all accepted realizations are zero;
- nonzero rounded nominal does not override an enclosure containing zero or both signs;
- expansion arithmetic proves only the declared exact relation over stored nominal operands;
- only the uncertainty enclosure may prove a relation for every accepted realization; and
- no symbolic, lineage, action-priority, or coordinate-choice rule may erase unresolved uncertainty.

The phrases `exact-nominal evidence`, `exact-nominal sign`, and `exact-nominal tie` in the original specification or plan are replaced by explicit references to these three layers.

### B.2 Component 12 bookkeeping evidence

Component 13 may consume a Component 12 exact-relation-zero bookkeeping disposition only as immutable predecessor evidence. It must verify that:

- the exact-relation formula and schema are supported;
- the accompanying uncertainty enclosure is present;
- Component 12's topology-invariance certificate is present and valid;
- no positive-area alternative was hidden;
- the same boundary halfedges, occurrence partition, orientation, coverage, and residual interface are preserved; and
- the corresponding length-valued handoff certificate is present.

Component 13 must not reinterpret that disposition as geometric certainty, Boolean ownership, collapse eligibility, or budget authorization.

### B.3 Component 13 query disposition

For every bounded orientation, distance, residual, clearance, intersection, or support query, evidence must record:

- formula and operation versions;
- rounded nominal bits;
- exact-relation availability and sign/zero;
- uncertainty enclosure and causes;
- whether the enclosure proves the required relation;
- the final deterministic disposition;
- precision-ledger and action/candidate consumers; and
- diagnostic and replay references.

If the enclosure admits outcomes that differ in manifoldness, orientation, embedding, coverage, clearance, or allowed budget, the action is rejected or the stage fails with a precise expected geometric status. The implementation must not wait for a numerical accident or use an exact-relation zero as a hidden tie breaker.

## C. Dimensionally valid tolerance and cleanup handoff

### C.1 Tolerance-facing quantities

The caller's tolerance and every ordinary Component 13 displacement/removal budget are lengths. Component 13 must not directly compare tolerance with:

- area or oriented-area magnitude;
- squared edge length;
- a determinant whose physical dimension is not length;
- angle;
- aspect ratio or another dimensionless quality score;
- triangle count, valence, or Euler characteristic; or
- an arbitrary scalar epsilon copied into a different unit.

Prohibited branches include `abs(area) <= tolerance`, `abs(orientation_determinant) <= tolerance`, `squared_length <= tolerance`, and `angle <= tolerance`.

Every threshold, interval, budget proposal, reservation, commit, report field, codec field, and verifier comparison must identify a closed metric and physical dimension. Unknown or inconsistent dimensions are contract failures, not implicit conversions.

### C.2 Required Component 12 handoff certificate

Every Component 12 cleanup-required triangle, edge, chain, fan, pinched patch, or residual imported by Component 13 must reference at least one valid `cleanup_handoff_length_certificate` containing:

- owner, schema, metric, and derivation versions;
- exact triangle/residual/obligation and occurrence keys;
- a closed length metric and physical dimension equal to `length`;
- a finite conservative interval;
- Component 03 derivation and inherited-precision references;
- support, base-length, clearance, or correspondence assumptions;
- forbidden occurrence merges and topology changes;
- the Component 13 obligation capability expected to consume it;
- proof that Component 12 reserved and committed no cleanup budget; and
- deterministic digest, diagnostic, and replay data.

Acceptable metrics include endpoint separation, point-to-line or point-to-segment deviation, minimum altitude or thickness, support-plane residual, clearance, and a versioned patch-deviation bound.

A missing, non-finite, dimensionally invalid, stale, or unsupported certificate is rejected before mutable import. Component 13 must not reconstruct an ad hoc length from area, guess the intended quotient, or silently substitute another metric.

### C.3 Advisory evidence is not authorization

A handoff certificate establishes only that Component 12 represented an explicit cleanup obligation with a conservative length-valued geometric witness. It does not establish that:

- an edge collapse satisfies the exact link condition;
- an occurrence merge is allowed;
- a feature may be removed;
- a replacement patch preserves coverage or orientation;
- remote geometry is clear;
- Boolean side semantics remain valid; or
- the action-specific cost fits the remaining budget.

For every candidate, Component 13 independently reconstructs topology and computes the selected action's patch correspondence, per-lineage displacement, feature-removal size, swept/support deviation, clearance, uncertainty, reservation, and actual committed cost. The imported certificate and the Component 13 action cost remain separate immutable records.

### C.4 Safe dimensional derivation

Area may contribute to a length certificate only through a reviewed Component 03 derivation with correct units and conservative denominator bounds. For example, an altitude upper bound may use a formula equivalent to:

```text
height_upper = 2 * area_upper / base_length_lower
```

only when `base_length_lower` is finite and strictly positive under its uncertainty enclosure, the base is the corresponding reviewed support segment, grouping and outward rounding are fixed, and the result is recorded as a length. If the denominator enclosure contains zero, that derivation is unavailable.

Such a derivation does not prove zero area and does not by itself authorize removal.

## D. Corrected cleanup obligation and action semantics

### D.1 Intake and reconciliation

The Component 13 preflight must verify, before mutable allocation:

- complete Component 12 truth-layer, category, handoff, and obligation evidence;
- exact certificate-to-entity and entity-to-certificate reverse maps;
- owner, version, scalar/index, formula, metric, and dimension compatibility;
- topology-invariance evidence for every accepted exact-relation-zero bookkeeping disposition;
- no hidden positive-area alternative in a cleanup-required category;
- Component 12's zero cleanup-budget-use certificate; and
- sufficient resource bounds for imported evidence and independent verification.

A committed Component 12 contradiction is `internal_invariant_error`. A validly represented but unsolved cleanup obligation remains an expected cleanup/tolerance failure, not an invariant failure.

### D.2 Candidate eligibility and ordering

A candidate begins from exact topology and lineage plus a supported obligation. The handoff length may participate in preliminary feasibility only through Component 03 typed comparisons.

Numerical cost may order candidates only when both quantities use the same metric and dimension and their conservative intervals are definitely disjoint. Overlapping intervals, different metrics, and equal bounds fall back to canonical lineage/action keys. Nominal floating values are never hidden tie breakers.

### D.3 Final triangle acceptance

A final Component 13 triangle must have:

- three distinct topological vertex occurrences;
- definite prescribed orientation proved by its uncertainty enclosure;
- finite bounded coordinates and fresh precision references;
- no unresolved cleanup obligation;
- no prohibited zero-length edge or invalid support residual;
- no forbidden self or nonadjacent intersection; and
- complete action and predecessor lineage.

Small area or poor aspect ratio alone does not make a triangle invalid. A definite positive thin feature remains unless a named policy-authorized action independently satisfies every topology, geometry, budget, and semantic condition.

### D.4 Budget and precision accounting

All accepted action costs must be strongly typed and separately recorded:

- inherited input precision;
- construction uncertainty;
- per-original-lineage cumulative displacement;
- local feature-removal/deformation length;
- whole-component removal size;
- swept/support/clearance length; and
- topology-change authorization.

Uncertainty is not movement. A Component 12 handoff bound is not an action commit. Maximum individual movement does not replace cumulative per-lineage displacement. Reports and output precision must be reconstructed from committed ledger records rather than mutable summaries.

## E. Artifact and downstream handoff additions

The immutable `cleaned_triangle_manifold<T>` must additionally contain or reference:

- validated predecessor `T,I` descriptors;
- reviewed Component 12 artifact/category/obligation versions;
- imported truth-layer and handoff-certificate sections or stable immutable references;
- certificate-to-obligation and action-to-certificate mappings;
- explicit advisory, proposed, reserved, committed, and rejected cost roles;
- closed metric and dimension identifiers for every tolerance-facing value;
- dimensional derivation traces;
- corrected Component 13 artifact, codec, replay, and verifier versions; and
- corrected Component 14/15 compatibility declarations.

Component 14 must copy and aggregate these records without reinterpreting units, lowering bounds, or promoting advisory values. Component 15 must be able to independently reconstruct every dimensional derivation, action cost, cumulative displacement, and report maximum.

## F. Independent verification additions

The Component 13 verifier must additionally:

1. validate reviewed Component 12 and Component 13 schema/version compatibility;
2. independently verify every imported handoff certificate's owner, metric, dimension, interval, trace, and reverse maps;
3. verify Component 12 committed zero cleanup budget;
4. reconstruct all three truth layers for decisive cleanup queries where capabilities require it;
5. verify definite geometric dispositions follow from uncertainty enclosures rather than rounded nominal or exact relation alone;
6. validate every area-to-length or squared-length-to-length derivation and denominator precondition;
7. recompute action-specific displacement/removal/clearance costs rather than trusting predecessor handoff bounds;
8. reject dimensional mutations even when counts, unit tags, logical bytes, and superficial digests are updated consistently;
9. verify valid thin features are not removed solely from area or quality; and
10. publish corrected verifier, codec, and replay versions.

Component 15 remains the final publication authority and independently repeats the mandatory public-facing budget, topology, intersection, side, and evidence-completeness checks.

## G. Existing Ygor assessment confirmed

The review confirms the original plan's primary reuse decisions.

- `mesh_remesher<T,I>` in `YgorMeshesRemeshing` is a general public-mesh quality remesher. Its midpoint collapse, valence flip, smoothing, raw nominal geometry, public index substitution, exception behavior, and lack of lineage/budget/intersection certificates make it unsuitable as a Component 13 provider.
- `fv_surface_mesh<T,I>` cleanup helpers remain useful public utilities but cannot preserve Boolean occurrence identity, paired halfedges, Component 03 budgets, or topology-distinct equal-coordinate vertices.
- legacy `YgorMeshesBoolean{,2,3,4,5}` implementations are not cleanup providers. Their coordinate-derived identities, snapping or ordinary floating assumptions, public-mesh assembly, and missing bounded action certificates conflict with the broad plan.
- public hole filling, orientation, subdivision, zippering, BSP cleanup, and tetrahedralization cleanup utilities are also unsuitable as authoritative providers because they operate after loss of the required internal identity or use unrelated repair semantics.

Required reuse remains:

- Component 03 bounded points, truth layers, predicates, residuals, finite AABBs, precision ledger, and dimensioned tolerance-budget service;
- Component 12 occurrence-preserving local triangulation primitives through a narrow reviewed adapter;
- Component 06/03 finite-AABB and rank-Morton primitives as semantics-free accelerators;
- Component 01 owner-bound IDs, checked arithmetic, resources, cancellation, transactions, deterministic arbitration, canonical bytes, SHA-256, diagnostics, and replay; and
- Component 17 deterministic task infrastructure while retaining a serial semantic reference.

Do not create a second interval package, physical-unit system, exact-arithmetic package, tolerance ledger, or symbolic policy inside Component 13.

## H. Failure and diagnostic additions

Add stable Component 13 subcodes for at least:

- predecessor scalar/index descriptor mismatch;
- obsolete or mixed Component 12/13 schema versions;
- numerical truth-layer conflation;
- exact relation used without enclosure proof;
- exact-relation-zero bookkeeping evidence missing topology invariance;
- cleanup handoff certificate missing, stale, non-finite, or unsupported;
- cleanup handoff physical dimension not length;
- dimensionally invalid tolerance comparison;
- invalid dimensional derivation or nonpositive denominator bound;
- advisory handoff bound promoted to action authorization or committed cost;
- Component 12 nonzero cleanup budget use;
- same-metric requirement violated during numerical candidate ordering; and
- Component 14/15 compatibility mismatch.

Expected bounded ambiguity maps to `geometric_condition_exceeds_tolerance`, `cleanup_budget_exceeded`, or `result_geometry_not_validated` as appropriate. Malformed policy/capability data maps to `input_contract_error` or `invalid_tolerance`. A contradiction in a committed artifact or producer/verifier disagreement maps to `internal_invariant_error`.

Every failure includes canonical entity keys, metric and dimension, rounded nominal, exact relation, uncertainty enclosure, inherited and action-specific bounds, policy/version data, and deterministic replay payload where applicable.

## I. Required test amendments

In addition to the original suite, add:

1. **Type compatibility tests** for all supported `float`/`double` and `uint32_t`/`uint64_t` combinations plus intentional `T,I` descriptor mismatches.
2. **Truth-layer matrix tests** covering every representable combination needed by cleanup orientation, distance, residual, clearance, and intersection queries.
3. **Exact-zero/sign-spanning tests** proving exact stored-nominal zero does not authorize a collapse, retriangulation, or final orientation when uncertainty spans outcomes.
4. **Handoff-certificate tests** for missing, duplicate, stale-owner, stale-trace, non-finite, wrong-dimension, wrong-metric, incompatible-obligation, and hidden-positive-area mutations.
5. **Advisory-not-authorization tests** where a small valid handoff bound accompanies a failed link condition, prohibited occurrence merge, protected source feature, remote intersection, or over-budget action correspondence.
6. **Dimensional misuse mutation tests** replacing length with area, squared length, determinant, angle, or aspect ratio and requiring zero survivors even with corrected counts and digests.
7. **Area-to-altitude derivation tests** with positive, zero-containing, and extreme-exponent base-length enclosures and outward-rounding boundary cases.
8. **Scaling metamorphic tests** where lengths/tolerance scale by `s`, areas/squared lengths by `s^2`, and higher quantities by their proper dimension.
9. **Thin-feature retention tests** for definite positive tiny-area triangles, corridors, narrow shells, and poor-aspect-ratio features that satisfy the final contract.
10. **Independent action-cost tests** distinguishing imported handoff evidence, proposed cost, reservation, actual commit, cumulative displacement, and exact-oracle cost.
11. **Component 14/15 handoff tests** proving every metric, dimension, truth layer, derivation, and cost role can be reconstructed from immutable views.
12. **Schema mutation tests** proving obsolete/reviewed evidence cannot be mixed across artifact, codec, replay, or verifier versions.

## J. Reviewed definition of done

Component 13's review step is complete only when the original specification and plan, as amended, require all of the following:

- reviewed `triangulated_output_complex<T,I>` intake and `T,I` compatibility;
- explicit preservation of all three Component 03 numerical truth layers;
- dimensionally valid tolerance and cleanup-budget comparisons;
- a valid length-valued handoff certificate for every Component 12 cleanup-facing obligation;
- independent action-specific topology, correspondence, displacement, removal, swept, clearance, and budget evidence;
- no direct promotion of predecessor advisory evidence to authorization or committed cost;
- retention of valid thin features that satisfy the final contract;
- corrected immutable artifact, codec, replay, verifier, and downstream compatibility versions;
- zero required mutation survivors for truth-layer, dimensional, handoff, and advisory-cost corruption;
- strict portable C++17 production and normative-test code with no external dependency; and
- Component 14 can consume the cleaned artifact without cleanup, welding, topology repair, reorientation, coordinate change, unit reinterpretation, or obligation interpretation.
