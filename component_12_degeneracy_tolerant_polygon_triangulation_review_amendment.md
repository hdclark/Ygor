# Component 12 Review Amendment: Degeneracy-Tolerant Polygon Triangulation

## Status, precedence, and review conclusion

This file is a normative amendment to `component_12_degeneracy_tolerant_polygon_triangulation.md` produced by the independent Component 12 review required by `tracker.md`.

The original specification remains normative except where this amendment is more specific or conflicts with it. In a conflict, this amendment controls. Implementers and verifiers must read both files.

The review found that the original specification is structurally aligned with `broad_plan.md`, Component 11's reviewed polygonal-output contract, Component 13's cleanup contract, and Ygor's available in-tree polygon facilities. Its occurrence-preserving topology, pair-at-creation diagonals, deterministic bounded predicates, complete boundary accounting, transactional publication, independent verification, and no-external-dependency requirements are retained.

Three integration corrections are mandatory:

1. Component 12 must consume Component 03's reviewed three-layer numerical truth model rather than the obsolete phrase `exact-nominal tie`.
2. Caller tolerance is a length and must never be compared directly with area, squared length, determinant magnitude, angle, or a dimensionless quality score.
3. The concrete artifact and view type is consistently `triangulated_output_complex<T,I>` / `triangulated_output_complex_view<T,I>` because it preserves Component 11 owner-bound occurrence and halfedge domains parameterized by the public index descriptor `I`, even though Component 12's own strong IDs remain independent of `I`.

No change authorizes topology reconstruction from coordinates, tolerance welding, cleanup expenditure, external libraries, or reuse of incompatible legacy triangulators.

## A. Numerical truth-layer contract

### A.1 Required orthogonal evidence

Every topology-affecting planar query must preserve the following three distinct facts from Component 03:

- `rounded_nominal`: the exact stored bits produced by the frozen rounded operation graph;
- `exact_relation`: the exact algebraic sign or zero result for the versioned relation evaluated over the stored nominal floating-point operands as exact real values, when that capability is supported; and
- `uncertainty_enclosure`: the conservative set of relation values admitted by inherited coordinate uncertainty, projection uncertainty, and operation error.

These facts are orthogonal. In particular:

- an exact stored-nominal zero does not imply that the accepted geometric realizations are zero;
- a nonzero rounded nominal does not override an uncertainty enclosure containing both signs;
- exact expansion arithmetic proves only `exact_relation` for the declared formula and operands;
- only the uncertainty enclosure may prove that all accepted realizations have one sign; and
- a symbolic or lineage rule may not erase unresolved uncertainty.

The phrases `exact-nominal expansion`, `exact-nominal sign`, and `exact-nominal tie` in the original specification are replaced by the explicit terms above.

### A.2 Permitted Component 12 bookkeeping ties

Component 12 does not own Boolean contact semantics. Component 07 and the frozen Boolean context already resolved operation-specific symbolic ownership before Component 11. Component 12 may use a separately versioned triangulation-bookkeeping tie rule only when all of the following are true:

1. `exact_relation` proves zero for the exact declared stored-nominal relation;
2. the query and rule are explicitly listed in Component 12 capabilities;
3. the rule changes only decomposition bookkeeping, candidate ordering, or the representation of a zero-measure cleanup obligation;
4. every allowed disposition preserves the same Component 11 boundary halfedges, occurrence partition, contour domain, positive-area coverage, prescribed orientation, and cross-face topology;
5. no alternative allowed by `uncertainty_enclosure` can introduce or remove positive area, fill a hole, cross a constraint, invert a required triangle, or change a residual interface; and
6. the evidence records the exact-relation formula, uncertainty enclosure, rule version, complete lineage key, and proof of topology invariance.

If the uncertainty enclosure admits outcomes with different positive-area topology, Component 12 must continue the frozen escalation sequence or fail with `geometric_condition_exceeds_tolerance`. It must not classify the query as an exact tie merely because stored nominal bits satisfy an exact algebraic equality.

### A.3 Query result disposition

Every planar predicate result must expose, without conflation:

- rounded nominal bits;
- exact-relation availability and sign/zero;
- uncertainty lower and upper bounds or equivalent enclosure;
- conditioning and uncertainty causes;
- whether the enclosure proves a definite relation;
- whether an exact-relation-zero bookkeeping rule was considered;
- whether topology invariance for that rule was proved;
- the final deterministic disposition; and
- precision-ledger, consumer, diagnostic, and replay references.

Unknown, unsupported, malformed, or contradictory combinations are rejected. A result enum must not hide the coexistence of `exact_relation == zero` and an uncertainty enclosure admitting both signs.

## B. Dimensionally valid degeneracy and cleanup handoff

### B.1 Tolerance-facing quantities

The caller's tolerance and Component 13 cleanup budget are lengths. Component 12 may use area, determinant, orientation, dot products, and other dimensioned quantities as bounded geometric evidence, but it must not compare any non-length quantity directly with caller tolerance or a cleanup displacement threshold.

The following comparisons are prohibited:

- `abs(area) <= tolerance`;
- `abs(orientation_determinant) <= tolerance`;
- `squared_edge_length <= tolerance`;
- `angle <= tolerance`;
- aspect ratio or dimensionless quality compared with tolerance; and
- an arbitrary epsilon derived by reusing the tolerance value in another unit.

### B.2 Required length-valued handoff certificate

Any triangle, edge fan, chain, pinched patch, or residual categorized for Component 13 review must carry a conservative `cleanup_handoff_length_certificate`. The certificate describes a length-valued upper bound on the geometric deviation, clearance, thickness, endpoint separation, perpendicular deviation, or patch-removal distance relevant to the named cleanup obligation. It does not reserve or spend budget.

At minimum the certificate contains:

- certificate and provider versions;
- owning region, triangle/residual, occurrence, edge, cycle, and obligation keys;
- the precise length-valued metric and its geometric meaning;
- a finite conservative lower/upper interval;
- inherited precision and Component 03 operation-trace references;
- any support-plane, base-length, altitude, clearance, or correspondence assumptions;
- proof that the metric is dimensionally a length;
- the Component 13 capability and obligation class expected to consume it;
- forbidden topology changes and occurrence merges;
- a statement that no budget reservation or commit occurred; and
- deterministic diagnostic, digest, and replay data.

Acceptable examples include:

- a bounded endpoint-separation upper bound for a zero-nominal-length paired edge;
- a bounded point-to-supporting-line or point-to-segment distance for a collinear chain;
- a bounded minimum-altitude or thickness upper bound for a sliver triangle;
- a conservative patch Hausdorff-style deviation bound for a versioned residual cell; or
- a bounded support-plane residual where that residual is the cleanup obligation's actual length-valued cost.

Area may contribute to such a certificate only through a dimensionally valid derivation. For example, an altitude upper bound may be derived from an area upper bound only when a strictly positive conservative base-length lower bound is available and the formula and outward rounding are supplied by Component 03. If no safe length-valued derivation exists, area alone cannot make the structure cleanup-eligible.

### B.3 Positive-area topology remains authoritative

A small area, small determinant, poor aspect ratio, or narrow appearance does not authorize Component 12 to drop a triangle, hole, corridor, or contour. Positive-area topology remains exactly the Component 11 domain.

A positive-area hole is never converted to a residual solely because a length certificate is below tolerance. Hole removal, component removal, and topology-changing simplification remain Component 13 actions subject to explicit policy and certificates; unsupported topology changes fail.

## C. Corrected triangle categories

The original category names may be retained for schema compatibility, but their eligibility is amended as follows.

### C.1 `definite_positive_area`

Required:

- the uncertainty enclosure proves the prescribed orientation sign;
- no non-length tolerance comparison is used;
- boundary, containment, noncrossing, and coverage evidence is decisive; and
- no cleanup obligation is required for final validity.

### C.2 `positive_area_within_cleanup_margin`

Required:

- the uncertainty enclosure proves the prescribed positive orientation;
- the triangle is topologically valid and participates in complete coverage;
- a named Component 13 review obligation is justified by a finite conservative length-valued handoff certificate;
- the certificate is within the policy's read-only handoff limit, using Component 03 length comparison;
- keeping the triangle remains permitted if Component 13 independently proves it final-valid; and
- no budget is reserved or spent by Component 12.

Area, determinant magnitude, aspect ratio, or nominal altitude alone is insufficient.

### C.3 `zero_area_cleanup_required`

Required:

- aggregate evidence proves that the unresolved structure contributes no required positive-area cell under the accepted bounded model, not merely that a stored nominal determinant is zero;
- all boundary and internal-halfedge assignments remain exact;
- a finite conservative length-valued handoff certificate bounds the cleanup-relevant geometry;
- the residual interface and conceptual orientation are unambiguous; and
- Component 13 advertises a compatible obligation/action schema.

If uncertainty admits a positive-area alternative that would alter coverage, this category is prohibited and the stage fails.

### C.4 `symbolic_exact_tie_cleanup_required`

This legacy category name means an authorized exact-relation-zero triangulation-bookkeeping handoff, not an uncertainty resolution. It is permitted only when Section A.2 is satisfied and a length-valued handoff certificate from Section B.2 is present.

An exact relation zero without topology-invariance proof is not eligible.

### C.5 Invalid categories

`invalid_uncertain_positive_area_topology` includes every case where:

- exact relation and uncertainty were conflated;
- a sign-spanning enclosure was resolved by a lineage rule without topology-invariance proof;
- a non-length quantity was compared directly with tolerance; or
- cleanup eligibility was inferred without a finite length-valued certificate.

## D. Residual and obligation contract additions

Every cleanup-required triangle or residual record must additionally include:

- the three numerical truth layers for each decisive relation;
- exact-relation-zero bookkeeping eligibility and topology-invariance evidence, when used;
- one or more length-valued handoff certificate IDs;
- the exact Component 13 obligation schema/capability version required;
- proof that all positive-area coverage outside the residual is preserved;
- a prohibition on deriving budget cost from area alone;
- a prohibition on merging topology-distinct occurrences solely because the certificate is small; and
- proof of zero Component 12 budget reservations and commits.

Component 13 must be able to import the obligation without reconstructing contours, recomputing an alternative predicate graph, converting area to a length with an ad hoc formula, or guessing the permitted occurrence quotient.

## E. Artifact type and API consistency

All concrete Component 12 artifact declarations, entrypoints, views, codecs, tests, and Component 13-facing interfaces use:

```cpp
template<class T, class I>
class triangulated_output_complex;

template<class T, class I>
class triangulated_output_complex_view;
```

Component 12-owned IDs remain strong owner-bound domains and do not alias `I`. The `I` parameter records and validates compatibility with the Component 11/public index descriptor and predecessor schemas; it does not authorize using public indices as internal identity.

Occurrences of `triangulated_output_complex<T>` in the original Component 12 specification are read as `triangulated_output_complex<T,I>`.

## F. Existing Ygor assessment confirmed

The review confirms the original reuse decisions:

- `YgorMathMonotoneDecomposition` is a useful algorithmic and fixture reference but is not a production or verifier provider because it normalizes away equal/collinear occurrences, rejects touching/zero-length constraints, uses raw coordinate ordering and `long double` policy, and emits mutable public meshes.
- `YgorMathConstrainedDelaunay` and `YgorMathDelaunay` are incompatible because they deduplicate or reject coincident constraints and do not preserve paired boundary-halfedge identity or bounded uncertainty evidence.
- public mesh hole filling, cleanup, orientation, and remeshing utilities are not Component 12 providers.
- Component 03 structured services, Component 11 topology, Component 01 infrastructure, and narrow semantics-free in-tree combinatorial helpers are the required reuse path.

No external dependency is allowed. No legacy routine may be copied in a way that imports coordinate normalization, ordinary floating policy, exception-only failures, or public-mesh topology.

## G. Failure and diagnostic additions

Add stable Component 12 subcodes for at least:

- numerical truth-layer conflation;
- exact-relation-zero tie not topology-invariant;
- exact-relation capability unavailable where required;
- dimensionally invalid tolerance comparison;
- cleanup handoff length certificate unavailable;
- cleanup handoff length bound non-finite or exceeds policy;
- area-to-length derivation lacks positive base-length lower bound;
- Component 13 obligation capability mismatch; and
- artifact scalar/index descriptor mismatch.

Expected bounded ambiguity maps to `geometric_condition_exceeds_tolerance`; malformed policy or incompatible capability maps to `input_contract_error` or `invalid_tolerance`; predecessor contradiction or producer/verifier disagreement maps to `internal_invariant_error`.

Every affected failure includes the rounded nominal, exact relation, uncertainty enclosure, length-certificate evidence, policy/version data, canonical entity keys, and replay payload.

## H. Required test amendments

In addition to the original suite, add:

1. **Truth-layer matrix tests** covering every combination of rounded nominal sign, exact relation sign/zero, and uncertainty enclosure sign set that is representable.
2. **Exact-zero-with-sign-spanning-uncertainty tests** proving that exact stored-nominal zero does not authorize a bookkeeping tie or cleanup category.
3. **Topology-invariant tie tests** proving that an allowed tie changes only traversal/decomposition bookkeeping and preserves identical boundary, coverage, holes, orientation, and residual interface.
4. **Dimensional misuse mutation tests** that replace a length certificate with area, squared length, angle, determinant magnitude, or aspect ratio and require verifier rejection.
5. **Length-certificate boundary tests** just below, at, and just above the read-only cleanup handoff threshold.
6. **Area-to-altitude derivation tests** with positive base lower bounds, zero-containing base bounds, extreme exponents, and outward-rounding limits.
7. **Scaling metamorphic tests** where coordinates and tolerance scale by `s`, lengths by `s`, areas by `s^2`, and determinant quantities by their proper dimension. Classification must remain consistent after the documented remap.
8. **Component 13 import tests** proving obligations can be consumed without contour reconstruction, ad hoc unit conversion, or alternative predicate evaluation.
9. **Type compatibility tests** for all supported `float`/`double` and `uint32_t`/`uint64_t` combinations and intentional scalar/index descriptor mismatches.
10. **Mutation tests** that forge an exact-relation-zero flag, hide a sign-spanning enclosure, alter a length certificate's units, or compare an area directly with tolerance while preserving counts and digests.

Existing tests described as placing `area` just below, at, or above caller tolerance are corrected: area boundaries are tested against independently versioned area/coverage bounds, while cleanup/tolerance boundaries use length-valued certificates. Scaling tests must verify the different dimensional exponents.

## I. Amended definition of done

Component 12 is not complete unless all original definition-of-done requirements and all of the following hold:

- all predicate schemas and records expose rounded nominal, exact relation, and uncertainty enclosure separately;
- no exact stored-coordinate zero is treated as symbolic authorization by itself;
- every bookkeeping tie has a versioned topology-invariance proof;
- no area, squared length, determinant, angle, or dimensionless quality is compared directly with caller tolerance;
- every cleanup-facing category and residual has a finite conservative length-valued handoff certificate;
- Component 13 can import every obligation without inventing a unit conversion or predicate;
- concrete Component 12 artifact/view APIs consistently use `<T,I>`;
- the independent verifier rejects every truth-layer and dimensional mutation; and
- all original topology, determinism, resource, cancellation, replay, strict C++17, and no-external-dependency gates continue to pass.
