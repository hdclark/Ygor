# Plan 12 Review Amendment: Degeneracy-Tolerant Polygon Triangulation

## Status, precedence, and implementation intent

This file is a normative implementation-plan amendment to `plan_12_degeneracy_tolerant_polygon_triangulation.md`. It records the independent Component 12 review required by `tracker.md` and integrates the reviewed Component 03 numerical contract, the reviewed Component 11 polygonal-output contract, and Component 13's cleanup-obligation contract.

The original plan remains normative except where this amendment is more specific or conflicts with it. In a conflict, this amendment controls. Implementers, reviewers, test authors, and verifiers must read both files and the corresponding Component 12 specification amendment.

The original plan's fixed V1 architecture remains suitable: occurrence-preserving constrained triangulation, exact boundary-halfedge retention, complete bridge enumeration, deterministic ear decomposition, pair-at-creation internal diagonals, transactional region work, canonical merge, coverage checks, independent verification, strict C++17, and no external dependencies. This amendment corrects three integration defects without changing that architecture:

1. replace obsolete `exact-nominal` terminology and semantics with Component 03's orthogonal `rounded_nominal`, `exact_relation`, and `uncertainty_enclosure` layers;
2. replace tolerance-facing area or determinant criteria with dimensionally valid length-valued cleanup-handoff certificates; and
3. make the concrete artifact/view/API type consistently `triangulated_output_complex<T,I>`.

## 1. Reviewed provider and schema versions

The reviewed implementation must assign new nonzero versions so artifacts produced under the corrected contract cannot be confused with earlier drafts. Use names conceptually equivalent to:

```text
projection_provider:                 authoritative_support_frame_projection_v1
region_model_provider:               occurrence_preserving_constraint_model_v1
predicate_provider:                  component03_truth_layers_planar_relations_v2
contour_provider:                    canonical_complete_visibility_bridge_v1
triangulation_provider:              bounded_indexed_ear_decomposition_v1
orientation_escalation_provider:     neighbor_chain_truth_layer_escalation_v2
diagonal_provider:                   atomic_pending_twin_diagonal_v1
degeneracy_provider:                 length_certificate_degenerate_cell_handoff_v2
producer_overlap_provider:           region_rank_interval_aabb_index_v1
coverage_provider:                   boundary_euler_area_nonoverlap_v1
verification_provider:               independent_truth_layer_cell_rebuild_v2
artifact_schema:                     triangulated_output_complex_v2
predicate_evidence_schema:           planar_truth_evidence_v2
triangle_category_schema:            dimensional_triangle_category_v2
cleanup_handoff_certificate_schema:  cleanup_handoff_length_certificate_v1
residual_obligation_schema:           triangulation_residual_obligation_v2
codec_schema:                        triangulated_output_codec_v2
replay_schema:                       triangulated_output_replay_v2
```

The exact constants must follow local registry naming, but the version split is mandatory. Component 12 must reject artifacts, capabilities, replay payloads, or verifier reports that mix the reviewed truth-layer/category schemas with obsolete versions.

## 2. Corrected API and artifact type

Use the concrete internal entrypoint:

```cpp
template<class T, class I>
stage_outcome<artifact_handle<const triangulated_output_complex<T,I>>>
build_triangulated_output_complex(
    const boolean_context_view<T,I>& context,
    const precision_context_view<T>& precision,
    const source_triangle_complexes_view<T,I>& source_triangles,
    const canonical_source_manifolds_view<T,I>& source_manifolds,
    const canonical_intersection_complex_view<T,I>& intersections,
    const retained_surface_complex_view<T,I>& retained,
    const polygonal_output_complex_view<T,I>& polygonal,
    const output_triangulation_capabilities<T,I>& capabilities);
```

All public-to-bounded compatibility records, codecs, queries, tests, and Component 13 imports use `triangulated_output_complex<T,I>` and `triangulated_output_complex_view<T,I>`.

`I` remains a descriptor and compatibility parameter. Component 12 entity identity uses owner-bound strong IDs and checked offsets, never public indices. No Component 12 ID aliases `I`, `size_t`, a pointer, or a hash.

## 3. File-level implementation amendments

The original production file list remains. Apply these additional responsibilities.

### 3.1 `OutputTriangulationTypes.h`

Add:

- closed enums for numerical truth-layer availability and disposition;
- `planar_truth_evidence_id` or the reviewed equivalent;
- `cleanup_handoff_length_certificate_id`;
- a closed length-metric enum, including endpoint separation, point-line deviation, point-segment deviation, minimum altitude/thickness, support-plane residual, patch-deviation bound, and invalid;
- a closed physical-dimension enum whose reviewed production value for cleanup handoff is `length`;
- explicit exact-relation-zero bookkeeping eligibility and topology-invariance dispositions;
- corrected triangle-category schema constants; and
- failure-detail records for truth-layer conflation, invalid units, unavailable certificates, and Component 13 capability mismatch.

Do not encode physical units as free-form strings.

### 3.2 `TriangulationPredicates.h/.cc`

Replace every `exact-nominal` field or branch with explicit Component 03 adapters for:

```text
rounded_nominal
exact_relation
uncertainty_enclosure
```

The predicate registry owns canonical query formation and immutable reuse only. Component 03 owns the arithmetic graph, exact-relation calculation, enclosures, finite checks, and precision evidence.

A query result must never collapse these layers into one sign enum. Store and serialize all layers even when the final disposition is definite.

### 3.3 `TriangulationEars.h/.cc`

Ear eligibility must use:

- uncertainty-enclosure proof for definite orientation and separation;
- exact-relation-zero evidence only as a prerequisite for a reviewed bookkeeping tie;
- a separate topology-invariance proof before any exact-zero tie disposition;
- length-valued cleanup-handoff evidence for categories requiring Component 13 review; and
- unchanged complete-key and generation rules.

Delete or reject any implementation branch equivalent to `abs(area) <= tolerance`, `abs(det) <= tolerance`, nominal-quality ranking, or tolerance-as-epsilon.

### 3.4 `TriangulationResiduals.h/.cc`

Every cleanup-required triangle or residual must own or reference at least one complete length-valued handoff certificate and name the compatible Component 13 obligation schema.

Residual construction must fail when:

- no finite conservative length certificate can be produced;
- the only evidence is small area, small determinant, poor aspect ratio, or a nominal metric;
- an area-to-length conversion lacks a strictly positive conservative denominator bound;
- exact stored-coordinate zero is being used to hide sign-spanning uncertainty;
- Component 13 lacks a compatible action/obligation capability; or
- alternative interpretations change positive-area coverage.

### 3.5 `TriangulationCoverage.h/.cc`

Bounded area remains required for coverage verification, but it has no cleanup-budget authority. Keep area evidence separate from cleanup-handoff certificates and from caller tolerance.

Coverage verification must detect area-cancelling overlap/missing-pocket mutations independently of total area, as already required.

### 3.6 `TriangulatedOutputComplex.h` and `TriangulationQueries.h`

Add immutable sections/views for:

- complete three-layer predicate evidence;
- exact-zero bookkeeping eligibility and topology-invariance certificates;
- cleanup-handoff length certificates;
- certificate-to-triangle/residual/obligation reverse maps;
- Component 13 compatibility records; and
- a zero Component 12 cleanup-budget-use certificate.

Component 13 must be able to import an obligation using narrow owner-checked views without reconstructing contours, choosing a formula, converting area to a length, or repeating a topology-affecting relation.

### 3.7 `TriangulationCodec.h/.cc`

Encode explicitly:

- all three numerical layers;
- formula and operation versions;
- exact-relation availability and sign;
- uncertainty bounds and causes;
- tie eligibility and topology-invariance evidence;
- physical-dimension and length-metric enums;
- length-certificate bounds and contributor references; and
- Component 13 compatibility versions.

Reject unknown enums, mixed schema versions, non-finite bounds, non-length handoff units, and nonzero reserved fields.

### 3.8 `TriangulationVerifier.h/.cc`

The verifier independently reconstructs and checks the corrected evidence. It must not trust producer category labels or certificate unit tags alone.

At minimum it must:

1. recompute or independently validate rounded nominal bits from the frozen graph;
2. recompute exact relation where the capability promises it;
3. independently validate uncertainty enclosures through Component 03 primitives and different grouping where prescribed;
4. prove that every definite disposition follows from the enclosure rather than the rounded nominal;
5. prove that every exact-zero tie has topology-invariance evidence and no positive-area alternative;
6. verify every cleanup-facing category has a finite length-valued certificate;
7. validate each dimensional derivation, including denominator positivity for area-to-altitude conversions;
8. verify no Component 12 budget reservation/commit exists;
9. verify Component 13 compatibility; and
10. reject forged fields even when counts, codec bytes, and top-level digests are internally updated by a mutation fixture.

## 4. Required evidence schemas

### 4.1 Planar truth evidence

Use a schema conceptually equivalent to:

```cpp
template<class T>
struct planar_truth_evidence {
    planar_predicate_evidence_id id;
    predicate_query_key key;
    formula_version formula;

    rounded_value<T> rounded_nominal;
    exact_relation_result exact_relation;
    finite_interval<T> uncertainty_enclosure;

    uncertainty_cause cause;
    definite_relation_disposition definite_disposition;
    exact_zero_bookkeeping_eligibility tie_eligibility;
    topology_invariance_certificate_id topology_invariance;
    final_predicate_disposition final_disposition;

    precision_trace_root_id trace;
    precision_ledger_ref inherited_precision;
    canonical_range<consumer_ref> consumers;
};
```

This is conceptual; use local types. The critical requirement is that no field or enum merges exact relation with uncertainty.

### 4.2 Cleanup handoff length certificate

Use a schema conceptually equivalent to:

```cpp
template<class T>
struct cleanup_handoff_length_certificate {
    cleanup_handoff_length_certificate_id id;
    cleanup_handoff_length_certificate_key key;
    cleanup_length_metric metric;
    physical_dimension dimension; // must be length

    finite_interval<T> deviation_or_thickness;
    precision_trace_root_id derivation_trace;
    precision_ledger_ref inherited_precision;

    topology_interface_ref interface;
    canonical_range<occurrence_separation_ref> forbidden_merges;
    component13_obligation_schema_version consumer_schema;
    bool budget_reserved; // must be false
    bool budget_committed; // must be false
};
```

The certificate is advisory evidence for Component 13 eligibility and independent review. It does not pre-authorize an edit and does not consume tolerance.

## 5. Corrected predicate and tie algorithm

For every canonical topology-affecting query:

1. Form one canonical query key containing formula version, support frame, normalized strong-ID operands, orientation parity, and consumer-independent semantics.
2. Ask Component 03 for the frozen rounded operation graph, exact relation when supported, and uncertainty enclosure.
3. Validate finite values, owner/version compatibility, nominal containment where applicable, and trace references.
4. If the uncertainty enclosure proves the required relation, record a definite disposition.
5. Otherwise run the reviewed deterministic aggregate escalation sequence, reusing canonical evidence.
6. If escalation remains sign-ambiguous, do not inspect nominal magnitude or tolerance as an epsilon.
7. An exact-relation-zero result may enter tie review only when the capability lists the query kind and the exact formula.
8. Construct an independent topology-invariance certificate proving that every permitted tie disposition preserves the same boundary, contours, holes, positive-area coverage, orientation, occurrence partition, and residual interface.
9. If topology invariance is proved, apply the complete-lineage bookkeeping rule and retain both exact-relation and uncertainty evidence.
10. Otherwise fail with `geometric_condition_exceeds_tolerance` or reject the candidate as appropriate.

No consumer may recompute an equivalent query using another operand order, algebraic grouping, or formula.

## 6. Dimensionally valid category and handoff algorithm

### 6.1 Definite positive triangle

A triangle is definite positive only when the uncertainty enclosure proves prescribed orientation and all topology/coverage relations are decisive. No cleanup certificate is needed unless another named final-validity condition requires Component 13 review.

### 6.2 Positive triangle requiring cleanup review

To classify `positive_area_within_cleanup_margin`:

1. prove positive orientation from the uncertainty enclosure;
2. identify a named Component 13 obligation class;
3. compute a conservative length-valued metric through Component 03;
4. verify the metric is finite and dimensionally length;
5. compare only that length interval with the read-only handoff threshold;
6. preserve the triangle and all topology unchanged; and
7. emit the complete handoff certificate without reserving budget.

Potential metrics include minimum altitude/thickness, endpoint separation, support residual, or a versioned patch-deviation bound. Aspect ratio and area are diagnostic only unless converted through a reviewed dimensional derivation.

### 6.3 Safe area-to-altitude derivation

When a provider derives an altitude bound from area, use a Component 03 formula equivalent to:

```text
height_upper = 2 * area_upper / base_length_lower
```

only when:

- `area_upper` is nonnegative and conservative;
- `base_length_lower` is finite and strictly positive under its uncertainty enclosure;
- all operations use outward rounding and fixed grouping;
- the base is the actual corresponding triangle edge or a reviewed support segment;
- units are recorded; and
- the resulting height is used only as a length certificate, not as proof of zero area.

If the base lower bound contains zero, no such certificate may be produced from that base. Try another reviewed metric or fail/retain the triangle for a different valid disposition.

### 6.4 Zero-measure and exact-zero residuals

A zero-measure residual requires aggregate bounded proof that no required positive-area cell is lost. Exact relation zero alone is insufficient. All boundary/internal-halfedge assignments, conceptual orientation, local fan/interface, and length certificates must be complete.

## 7. Canonical keys and ordering amendments

Extend complete keys as follows:

```text
predicate_query_key +=
    (truth_layer_schema_version,
     exact_relation_formula_version,
     uncertainty_formula_version)

planar_truth_evidence_key =
    (predicate_query_key,
     rounded_nominal_bits,
     exact_relation_availability_and_sign,
     uncertainty_enclosure_bits,
     final_disposition,
     topology_invariance_key_or_null)

cleanup_handoff_length_certificate_key =
    (owning region/triangle/residual/obligation complete key,
     length_metric,
     physical_dimension,
     derivation formula version,
     ordered contributor lineage keys,
     topology interface key,
     Component 13 consumer schema)
```

Numerical interval values participate in complete record content and digest, but not as hidden discovery-order or nominal-quality tie breakers. Candidate ordering remains category then complete lineage as specified in the original plan.

## 8. Preflight and capability amendments

Before projection or authoritative allocation, validate:

- Component 03 exports the reviewed three-layer capabilities required by every Component 12 query kind;
- exact relation is available only for declared formulas and bounded capacities;
- Component 03 length-metric operations needed by enabled cleanup categories are present;
- every enabled residual class maps to a supported Component 13 obligation schema;
- scalar `T` and index descriptor `I` match every predecessor artifact and capability;
- artifact, evidence, category, codec, replay, and verifier versions are mutually compatible; and
- no policy expresses an area, determinant, angle, squared length, or dimensionless threshold as caller tolerance.

A malformed dimensional policy is rejected before triangulation as `invalid_tolerance` or `input_contract_error`.

Resource preflight must include persistent and temporary counts/bytes/work for truth-layer evidence, topology-invariance proofs, length certificates, dimensional derivation traces, verifier scratch, and mutation/replay detail when enabled.

## 9. Checkpoint amendments

Retain the original 25 checkpoints and add these mandatory checks inside the corresponding phases:

- capability validation checks reviewed truth-layer and dimensional schemas;
- predicate construction stores all three layers;
- orientation escalation records exact-zero consideration separately from uncertainty;
- triangle/residual acceptance constructs and validates length certificates before commitment;
- per-region coverage verifies that area evidence was not used as budget authority;
- global preservation audit confirms no hidden coordinate or budget change;
- canonical codec verifies physical dimensions and schema compatibility; and
- independent verification rejects truth-layer or dimensional misuse before stage commit.

Cancellation polling and transactional rollback must include the new evidence/certificate tables and traces.

## 10. Component 13 handoff amendments

`TriangulationQueries.h` must expose narrow views that let Component 13 retrieve, for each obligation:

- exact member triangles, residual cells, boundary uses, internal halfedges, occurrences, and local fan/interface;
- prescribed conceptual orientation;
- all occurrence-separation and forbidden-merge constraints;
- three-layer predicate evidence relevant to the obligation;
- one or more length-valued cleanup-handoff certificates;
- source/event/carrier/retained-use provenance;
- Component 03 trace/ledger references;
- supported candidate action classes as nonbinding hints;
- the exact Component 13 schema version; and
- proof of zero Component 12 budget use.

Component 13 remains responsible for independent eligibility, proposal construction, actual length-valued cost, budget reservation/commit, no-new-intersection checks, and final action certificates. Component 12 must not preselect an edit or claim that a small certificate guarantees cleanup success.

## 11. Failure mapping amendments

Add stable subcodes for:

- unsupported reviewed truth-layer capability;
- rounded/exact/enclosure contradiction;
- exact relation unavailable for required tie review;
- exact-zero bookkeeping tie not topology-invariant;
- sign-spanning uncertainty improperly resolved;
- dimensionally invalid policy threshold;
- non-length cleanup certificate;
- non-finite or invalid length bound;
- unsafe area-to-length conversion;
- zero-containing denominator in a dimensional derivation;
- missing cleanup handoff certificate;
- Component 13 obligation schema mismatch; and
- scalar/index artifact descriptor mismatch.

Use Component 01 deterministic failure arbitration. Expected geometric ambiguity is not `internal_invariant_error`; malformed committed predecessor evidence or producer/verifier disagreement is.

## 12. Test-plan amendments

Add or extend tests as follows.

### 12.1 Truth-layer matrix

For orientation, segment relation, containment, area sign, and support residual, generate cases covering:

- rounded positive/negative/zero;
- exact relation positive/negative/zero/unavailable; and
- enclosure positive-only, negative-only, zero-only, zero-plus-positive, zero-plus-negative, and both-sign.

Assert the correct disposition and serialized evidence. Exact zero plus both-sign enclosure must never become a tie automatically.

### 12.2 Tie topology invariance

For every allowed bookkeeping tie kind, mutate or enumerate alternate dispositions and independently prove identical:

- Component 11 boundary assignment;
- contour/hole domain;
- positive-area coverage;
- orientation;
- occurrence partition;
- internal diagonal pairing; and
- residual interface.

A fixture whose alternate disposition changes any item must fail tie eligibility.

### 12.3 Dimensional mutation tests

Corrupt a valid artifact by replacing a length certificate with:

- area;
- squared length;
- determinant magnitude;
- angle;
- aspect ratio; or
- a length value tagged with the wrong derivation/unit.

Update counts and digests consistently in the fixture. The independent verifier must reject every mutation.

### 12.4 Length boundary tests

For each enabled length metric, test conservative intervals:

- definitely below the handoff threshold;
- touching the threshold;
- straddling the threshold;
- definitely above the threshold;
- containing zero;
- extreme finite values; and
- non-finite/overflowing derivations.

The handoff threshold is read-only; no test may observe a Component 12 budget commit.

### 12.5 Scaling tests

Under an exact power-of-two scale `s`:

- coordinates and tolerance scale by `s`;
- lengths and length certificates scale by `s`;
- areas scale by `s^2`;
- orientation determinants scale by the appropriate dimension; and
- dimensionless quantities remain dimensionless.

After documented remapping, topology, categories, tie eligibility, and failures must agree. A provider that compares area directly with tolerance will be detected by this suite.

### 12.6 Area-to-length derivation tests

Cover:

- exact positive base lower bound;
- base lower bound just above zero;
- base interval containing zero;
- very large and very small exponents;
- subnormal operands;
- outward rounding at finite limits;
- alternative base choices; and
- forged denominator/certificate references.

### 12.7 Component 13 import tests

Build Component 13 contract fixtures proving it can import every obligation and certificate without:

- reconstructing polygon contours;
- choosing a different predicate formula;
- deriving a length from area itself;
- inferring permitted occurrence merges; or
- reading mutable Component 12 storage.

### 12.8 Type matrix

Exercise all supported combinations:

```text
float,  uint32_t
float,  uint64_t
double, uint32_t
double, uint64_t
```

Also inject mismatched scalar/index descriptors and require preflight rejection.

## 13. Implementation sequence amendments

Retain the original implementation sequence, with these reviewed gates inserted:

1. **Version/schema migration gate.** Add reviewed truth-layer, category, certificate, artifact, codec, replay, and verifier versions; reject mixed versions.
2. **Component 03 capability gate.** Implement adapters and tests proving all three layers remain separate.
3. Continue original projection and region-model work.
4. **Predicate/tie gate.** Before hole integration, complete truth-layer registry and topology-invariance tie proofs; zero required truth-layer mutation survivors.
5. Continue original hole integration and ear decomposition.
6. **Dimensional category gate.** Before residual publication, implement length certificates and prohibit non-length tolerance comparisons; scaling and dimensional mutation suites pass.
7. Continue original diagonal materialization, residual handoff, producer coverage, canonical merge, codec, replay, verifier, resource, cancellation, fuzz, sanitizer, and performance gates.
8. **Component 13 compatibility gate.** Every supported residual imports through reviewed queries without contour reconstruction or ad hoc unit conversion.

Do not mark Component 12 implementation complete until all original and amended gates pass.

## 14. Reviewed definition of done

The Component 12 implementation handoff is complete only when:

- the original plan's topology, boundary, diagonal, coverage, determinism, transaction, resource, cancellation, replay, and independent-verification requirements all remain satisfied;
- every topology-affecting query preserves rounded nominal, exact relation, and uncertainty enclosure as separate evidence;
- exact stored-coordinate zero never authorizes a tie without a versioned topology-invariance proof;
- no sign-spanning uncertainty is converted to zero by tolerance or lineage;
- caller tolerance is compared only with dimensionally valid length-valued quantities;
- every cleanup-facing triangle/residual has a conservative finite length certificate and compatible Component 13 schema;
- area is used for coverage and may enter cleanup handoff only through a reviewed dimensionally valid derivation;
- no Component 12 code reserves or commits cleanup budget;
- the artifact and view API consistently use `<T,I>` while strong IDs remain independent of `I`;
- the independent verifier rejects all truth-layer, dimensional, unit, certificate, schema, and descriptor mutations;
- existing Ygor monotone/Delaunay/public-mesh routines remain references or fixtures only, not production/verifier providers;
- all code and normative tests are portable strict C++17, in-tree, deterministic, and free of external dependencies; and
- Component 13 can consume every obligation directly and perform its own certified cleanup without guessing topology, units, or numerical semantics.
