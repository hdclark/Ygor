# Plan 12: Degeneracy-Tolerant Polygon Triangulation

## 0. Scope and fixed V1 design

Implement **only Component 12** from `component_12_degeneracy_tolerant_polygon_triangulation.md`. Consume the verified immutable artifacts from Components 01-11 and publish one immutable `triangulated_output_complex<T,I>` for Component 13. Do not repeat Boolean classification or selection, alter output-occurrence identity, move coordinates, spend cleanup budget, collapse or weld topology, remove a component, assemble `fv_surface_mesh<T,I>`, merge result polygons, or publish a public Boolean result.

Freeze V1 as the following deterministic provider set:

```text
projection_provider:              authoritative_support_frame_projection_v1
region_model_provider:            occurrence_preserving_constraint_model_v1
predicate_provider:               component03_compute_once_planar_relations_v1
contour_provider:                 canonical_complete_visibility_bridge_v1
triangulation_provider:           bounded_indexed_ear_decomposition_v1
orientation_escalation_provider:  neighbor_chain_area_lineage_escalation_v1
diagonal_provider:                atomic_pending_twin_diagonal_v1
degeneracy_provider:              explicit_degenerate_cell_handoff_v1
producer_overlap_provider:        region_rank_interval_aabb_index_v1
coverage_provider:                boundary_euler_area_nonoverlap_v1
verification_provider:            independent_constrained_cell_rebuild_v1
```

The executable serial implementation is the semantic reference. Parallel execution may process different face regions in private storage, but canonical projections, bridge choices, ear choices, triangle and diagonal topology, residual obligations, failure selection, diagnostics, bytes, and digests must match the serial reference exactly.

V1 is an occurrence-preserving constrained ear-decomposition provider, not a quality triangulator. It deliberately selects bridges and ears from complete canonical keys after bounded admissibility, rather than using nominal shortest-edge, largest-area, Delaunay, or aspect-ratio heuristics. A future quality provider requires new nonzero provider versions and must preserve every contract in this plan.

Non-negotiable V1 rules:

- Every Component 11 output vertex occurrence remains one distinct topological occurrence, even when projected or 3D coordinates are bit-identical.
- Every Component 11 boundary halfedge remains unchanged in identity, direction, endpoints, pair, region ownership, and provenance and is assigned exactly once to one triangle or one explicit residual obligation/patch permitted by the Component 12 contract.
- Every internal diagonal is allocated as a reciprocal halfedge pair before either side can enter a proposed region result. One side may be temporarily assigned to an emitted triangle while the reverse side is owned by the remaining active cell, but no pair is published until both sides have exactly one final incident cell.
- Every ordinary triangle references three distinct Component 11 output-occurrence IDs and three oriented halfedge uses forming one closed cycle.
- Projection is a bounded numerical representation only. Projection equality, distance, tolerance, and nominal angle never create identity, adjacency, cancellation, or welding.
- V1 triangulates one Component 11 positive-area face region consisting of exactly one outer contour and zero or more direct holes. Positive-area islands are separate Component 11 regions. Deferred zero-measure contours remain explicit residual obligations.
- Definite positive-area ears are consumed before cleanup-required zero-measure ears. No uncertain predicate is converted to zero by an epsilon test.
- A local geometric ambiguity that can change positive-area coverage beyond the authorized bounded model fails closed.
- No coordinate is recomputed, snapped, averaged, projected back to 3D, or changed by this component.
- Component 12 may read tolerance and cleanup policy thresholds only to categorize obligations for Component 13. It may not reserve, commit, or report any cleanup displacement or feature-removal cost.
- Use strict portable C++17 and the standard library only. Do not add, vendor, download, optionally load, or invoke an external polygon, triangulation, exact-arithmetic, geometry, graph, hashing, serialization, testing, or concurrency dependency.
- Do not call, adapt, or copy `src/YgorMeshesBoolean{,2,3,4,5}*.{h,cc}`.

A valid empty Component 11 artifact publishes a fully audited empty `triangulated_output_complex<T,I>` and deterministic digest. It is not an error.

## 1. Existing Ygor assessment and mandatory reuse decisions

### 1.1 `YgorMathMonotoneDecomposition` is useful design reference, not a production provider

`Monotone_Decomposition_2` and `Triangulate_Monotone_Decomposition` contain useful in-tree algorithmic ideas: contour nesting, temporary hole bridges, sweep-line partitioning, monotone stack triangulation, deterministic index tie breaks, and tests. They do not satisfy Component 12 and must not be called by the bounded Boolean producer or verifier because the current implementation:

- accepts raw `vec2<T>` loops rather than owner-bound output occurrences, boundary halfedges, bounded coordinates, contour roles, or provenance;
- removes consecutive equal coordinates and closing duplicates;
- repeatedly deletes collinear vertices;
- rejects reused coordinates, zero-length edges, touching contours, and zero-area structures that Component 12 must preserve;
- uses direct coordinate equality and ordinary coordinate ordering as semantic decisions;
- uses bare `orient_sign`, `point_on_*`, and `segments_intersect_*` results without inherited uncertainty or structured evidence;
- computes contour, face, triangle, and coverage areas with `long double`;
- chooses a nominal shortest bridge using `long double` squared length;
- emits a mutable public `fv_surface_mesh<T,I>` rather than paired internal topology;
- silently skips duplicate-index or zero-orientation triangles;
- reverses candidate winding locally without publishing paired-incidence evidence;
- checks coverage with a `long double` epsilon rather than boundary, Euler, noncrossing, and bounded area evidence;
- throws exceptions for expected invalid or difficult geometry; and
- has no owner/version validation, resource transaction, cancellation, precision ledger, deterministic replay, canonical codec, or independent artifact verifier.

Keep the public API source-compatible. Reuse its literature choices, fixture shapes, and non-normative performance baselines. Do not broaden that general-purpose API until it can satisfy its own compatibility requirements. Implement the V1 bounded provider under `src/YgorMeshesBooleanBounded/` with Component 01 and 03 contracts.

Small purely combinatorial helpers may be extracted only when all of the following hold:

- the helper accepts strong IDs or checked local offsets, not coordinates;
- it has no hidden normalization, deletion, winding change, or exception path;
- it is deterministic under complete keys;
- it is tested independently in both old and new callers; and
- extraction does not make the bounded target depend on a translation unit compiled with ordinary Ygor floating flags.

### 1.2 `YgorMathConstrainedDelaunay` and `YgorMathDelaunay` are prohibited providers

The existing Delaunay implementations are substantially incompatible with this component. They deduplicate vertices by coordinate, reject coincident constraint endpoints, reject vertices lying on constraints, discard degenerate triangles, use a lifted 3D convex hull and mutable public mesh, expose no boundary-halfedge identity, and do not preserve repeated-coordinate occurrences or zero-length topological constraints. Constraint recovery and domain filtering are not coupled to Component 03 enclosures, cleanup obligations, pair-at-creation topology, or deterministic replay.

Do not use Delaunay triangulation as a fallback, preprocessing step, verifier, or tie breaker. Do not copy its lifting, coordinate deduplication, pruning, constraint-face reconstruction, or local legalization into Component 12. Its tests may supply ordinary nondegenerate comparison fixtures only.

### 1.3 `YgorMeshesHoles`, remeshing, and public mesh utilities are unsuitable

Do not use `FindBoundaryChains`, `FillBoundaryChainsByZippering`, `EnsureConsistentFaceOrientation`, public mesh remeshing, duplicate-vertex merging, face cleanup, hole filling, BSP output, or mesh-orientation utilities. These routines operate after public mesh materialization, use coordinate proximity or epsilon rules, skip short/duplicate edges, mutate faces, or lack paired-edge and occurrence lineage. Component 11 already supplies exact cycles, contour roles, orientation, and reciprocal cross-face pairs; Component 12 must fail on a contradiction rather than reconstruct or repair them.

`fv_surface_mesh<T,I>` is not an intermediate representation. Do not create one in Component 12. Public indexing and serialization belong to Component 14.

### 1.4 Adaptive predicates are reused only through Component 03

Use Component 03 structured bounded orientation, segment relation, side, containment, projection, residual, area, and exact-nominal tie services. Do not call public bare `orient_sign`, adaptive determinant, in-circle, point-on-segment, segment-intersection, or point-in-polygon APIs directly.

Component 03 owns:

- exact-nominal expansion arithmetic;
- conservative uncertainty enclosures;
- exact-tie versus uncertainty distinction;
- fixed expression grouping and floating-environment qualification;
- canonical operation and predicate evidence;
- bounded projection and support-plane residual checks; and
- precision-ledger references.

Component 12 owns only canonical query formation, query reuse, policy disposition, and triangulation-specific aggregate escalation. Do not create a second interval, exact-sign, or tolerance system.

### 1.5 Existing spatial indexes are not authoritative

Do not call public `rtree<T>`, `octree<T>`, `cells_index<T>`, KD-tree, or raw `index_bbox<T>` as the production overlap or ear-blocker provider. Their insertion history, ordinary floating comparisons, mutable storage, uninflated bounds, `std::any`, exceptions, or heuristic split behavior do not meet this stage's deterministic bounded contract.

For V1, implement a small region-local immutable interval/AABB index over Component 03 finite projected bounds. The index may reuse a generic strict-target flat hierarchy utility already introduced for Component 06 when that utility exposes owner-checked finite-AABB records and deterministic layout without importing triangle-specific semantics. Otherwise implement the region-local provider in Component 12. Spatial indexing only prunes definitely separated candidates; it never proves visibility or topology.

### 1.6 Mandatory predecessor and infrastructure reuse

Reuse, rather than duplicate:

- Component 11 output-occurrence records, authoritative bounded coordinate references, paired boundary edges and halfedges, face regions, cycles, contour roles, zero-measure descriptors, support lineage, reverse maps, and canonical keys;
- Component 10 retained-use orientation, occurrence-separation, multiplicity, and selected-boundary audit references;
- Components 04 and 05 source-facet support frames, source-triangle provenance, semantic face-region identity, and source-edge versus facet-diagonal distinctions;
- Component 08 event/carrier identity and construction lineage where a projected occurrence or repeated-coordinate audit needs it;
- Component 03 bounded values, finite AABBs, projection, determinant/orientation, segment relation, containment, residual, area, interval reduction, exact-nominal tie, precision-ledger, and tolerance-budget read services;
- Component 01 owner tokens, strong IDs, checked integer/count/byte arithmetic, typed outcomes/errors, resource leases, cancellation, deterministic failure arbitration, transactions, immutable publication, canonical bytes, SHA-256, diagnostics, and replay; and
- Component 17 deterministic task execution only when available, while retaining the serial implementation as the semantic reference.

Use `vec2<T>` and `vec3<T>` only as nominal payload carriers inside bounded records or non-authoritative visualization. Bare vector equality, ordering, length, distance, normalization, dot, cross, or angle must not determine a topology-affecting result.

## 2. Exact files, API, versions, and checkpoints

### 2.1 Files

Add under `src/YgorMeshesBooleanBounded/`:

- `OutputTriangulationTypes.h` — closed enums, strong IDs, complete keys, fixed provider constants, checked range types, counters, and failure-detail records.
- `TriangulatedOutputComplex.h` — immutable artifact schema, section views, checked accessors, and narrow Component 13 query views.
- `OutputTriangulation.h/.cc` — typed stage entrypoint, checkpoint orchestration, region task preparation, canonical merge, and publication transaction.
- `TriangulationPreflight.h/.cc` — predecessor validation, exact count bounds, ID/byte/work representability, and resource plan.
- `TriangulationProjection.h/.cc` — support-frame selection, bounded projection, projected occurrence table, frame conditioning, and plane-residual audit.
- `TriangulationRegionModel.h/.cc` — occurrence-preserving contours, local traversal nodes, immutable boundary constraints, active-cell topology, and reverse maps.
- `TriangulationPredicates.h/.cc` — canonical query keys, compute-once region predicate registry, structured dispositions, and aggregate escalation records.
- `TriangulationContours.h/.cc` — contour-role audit, canonical hole processing, complete bridge candidate enumeration, visibility certificates, and temporary disk-ring construction.
- `TriangulationEars.h/.cc` — active-ring generations, ear candidate discovery, blocker indexing, candidate queue, strict progress, and ordinary/cleanup candidate disposition.
- `TriangulationDiagonals.h/.cc` — atomic paired internal diagonals, pending-cell ownership, halfedge assignment, pair finalization, and diagonal reverse maps.
- `TriangulationResiduals.h/.cc` — cleanup-required triangle/cell creation, obligation aggregation, conceptual orientation, and Component 13 handoff records.
- `TriangulationCoverage.h/.cc` — producer boundary conservation, connectivity, Euler, bounded area, crossing, overlap, and missing-pocket checks.
- `TriangulationCodec.h/.cc` — canonical encoding/decoding, section digests, full digest, and replay payloads.
- `TriangulationVerifier.h/.cc` — independently implemented reconstruction, alternative overlap/coverage checks, bounded exhaustive oracle dispatch, and mutation rejection.
- `TriangulationQueries.h` — owner-checked immutable views required by Component 13, Component 15, diagnostics, and tests.

Extend existing bounded-subsystem infrastructure instead of creating parallel registries:

- `ContractVersions.h` for Component 12 provider, schema, key, category, codec, replay, and verifier versions;
- Component 01 stage, checkpoint, strong-ID-domain, resource-kind, error-subcode, diagnostic, and replay registries;
- Component 03 capability declarations only where required bounded planar queries or canonical interval reductions are not already exported;
- the strict bounded Boolean production target and explicit-instantiation lists; and
- the bounded test target and test registration.

Do not place these production translation units in the ordinary `src/CMakeLists.txt` glob target if that target still applies `-ffast-math`. They must compile through the strict Boolean target established by Components 01 and 03, with nearest-even arithmetic, no unauthorized reassociation, no finite-only assumptions, and the frozen contraction policy.

Add under `tests/mesh_boolean_bounded/`:

- `TestTriangulationContracts.cc`;
- `TestTriangulationProjection.cc`;
- `TestTriangulationRegionModel.cc`;
- `TestTriangulationBridges.cc`;
- `TestTriangulationEars.cc`;
- `TestTriangulationOrientationEscalation.cc`;
- `TestTriangulationRepeatedCoordinates.cc`;
- `TestTriangulationResiduals.cc`;
- `TestTriangulationBoundaryConservation.cc`;
- `TestTriangulationCoverage.cc`;
- `TestTriangulationBooleanCorpus.cc`;
- `TestTriangulationMutation.cc`;
- `TestTriangulationProperties.cc`;
- `TestTriangulationAdversarial.cc`;
- `TestTriangulationFuzzReplay.cc`;
- `TestTriangulationResourcesCancellation.cc`;
- `TestTriangulationDeterminismConcurrency.cc`;
- `TestTriangulationStructuralPerformance.cc`;
- `TriangulationFixtures.h/.cc`;
- `TriangulationExactOracle.h/.cc`; and
- `GoldenTriangulationV1.h`.

Keep exhaustive legal-diagonal enumeration, arbitrary-precision integer/rational arithmetic, corrupt-artifact builders, random generators, shrinkers, and golden regeneration tools test-only.

### 2.2 Typed entrypoint

Provide an internal entrypoint conceptually equivalent to:

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

`capabilities` freezes all Component 12 provider/schema versions, supported predecessor versions, verification level, bounded-oracle thresholds, supported scalar/index combinations, internal ID widths, maximum contour/region complexity, and Component 13 compatibility versions. It contains no caller-supplied triangulator, predicate callback, function pointer, allocator with semantic behavior, or external provider selector.

Validate all owner tokens, operation and operand-role mappings, scalar/index descriptors, policy versions, artifact versions, predecessor digests, verification dispositions, strong-ID domains, CSR ranges, coordinate references, support frames, precision-ledger references, and capability compatibility before authoritative allocation.

The stage owns one transaction. Per-region builders are private subtransactions or task-local proposals. Only the final verified immutable artifact handle is published.

### 2.3 Version registry

Add explicit nonzero versions for at least:

- output-triangulation artifact schema;
- support-frame selection and bounded-projection provider;
- projected occurrence and projected bound schemas;
- region-local contour, traversal-node, active-cell, and constraint schemas;
- planar predicate-query key and evidence schemas;
- orientation escalation provider and trace schema;
- bridge candidate, bridge visibility, and selected bridge schemas;
- ear candidate, rejection, deferral, and acceptance schemas;
- active-ring generation and progress schema;
- internal diagonal key, paired-edge, halfedge, and pending-owner schemas;
- triangle record, corner, edge-use, and geometric-category schemas;
- residual patch, degeneracy class, cleanup obligation, and handoff schemas;
- boundary assignment, connectivity, Euler, area, noncrossing, overlap, and coverage certificate schemas;
- region-local spatial-index provider/layout;
- canonical ID/remap and serialization schemas;
- diagnostics and replay schemas; and
- producer and independent-verifier versions.

Zero, unknown, unsupported, or incompatible required versions fail before construction. Include every version in artifact headers, errors, canonical bytes, replay, and verifier checks.

### 2.4 Stable checkpoints

Use these fixed checkpoints:

1. context and capability validation;
2. predecessor owner/version/digest verification;
3. exact count, ID, byte, and work preflight;
4. resource reservation;
5. canonical region task creation;
6. support-frame selection and conditioning audit;
7. bounded projected-occurrence construction;
8. exact contour, boundary-constraint, and local-node construction;
9. contour-role and zero-measure audit;
10. bridge candidate enumeration and compute-once visibility evaluation;
11. selected bridge-pair allocation and disk-ring construction;
12. active-cell, generation, blocker-index, and initial ear-queue construction;
13. orientation escalation and candidate disposition;
14. atomic diagonal-pair allocation and triangle or boundary-only residual acceptance;
15. per-region completion and strict progress audit;
16. per-region boundary, pair, connectivity, Euler, area, crossing, and overlap verification;
17. private region result finalization;
18. canonical global merge;
19. canonical dense ID assignment and reverse-map construction;
20. global Component 11 boundary and occurrence preservation audit;
21. producer invariant checks;
22. canonical codec, section digests, full digest, and replay construction;
23. independent artifact verification;
24. resource reconciliation and final cancellation poll; and
25. transaction commit.

Poll cancellation at every checkpoint and at deterministic work-count intervals inside large projections, contour scans, bridge candidate batches, predicate escalations, active-ring updates, spatial-index queries, overlap checks, verifier passes, sorts, and encodes. Never poll based on wall-clock timing.

## 3. Strong IDs, closed enums, complete keys, and artifact layout

### 3.1 Strong ID domains

Define non-interchangeable strong IDs for at least:

- `triangulation_region_id`;
- `support_frame_record_id`;
- `projected_occurrence_id`;
- `projected_constraint_id`;
- `contour_ref_id`;
- `local_traversal_node_id` for private builders only;
- `active_cell_id` and `active_generation_id` for private builders only;
- `planar_predicate_evidence_id`;
- `orientation_escalation_id`;
- `bridge_candidate_id` for diagnostic/replay tables;
- `selected_bridge_id`;
- `internal_diagonal_id`;
- `triangulation_halfedge_id` for Component 12 internal halfedges only;
- `output_triangle_id`;
- `triangle_corner_ref_id` and `triangle_edge_use_ref_id`;
- `residual_cell_id`;
- `cleanup_obligation_id`;
- `boundary_assignment_id`;
- `region_spatial_node_id`;
- `coverage_certificate_id`;
- `overlap_audit_id`; and
- verifier/replay evidence IDs where a generic evidence ID would permit domain confusion.

Do not alias these IDs to Component 11 IDs, `I`, `size_t`, vector offsets, pointers, hashes, or each other. A record may contain a Component 11 `output_vertex_occurrence_id`, `output_halfedge_id`, `paired_output_edge_id`, `output_face_region_id`, or `face_cycle_id` only in the exact declared predecessor domain. Conversion between IDs and contiguous offsets must use checked owner/domain accessors.

### 3.2 Closed enums

Use explicit nonzero enumerators and reject unknown values for:

- support frame disposition: inherited authoritative, canonical source-facet fallback, invalid;
- projected value disposition: finite bounded, conditioning exceeded, non-finite, invalid;
- contour role: outer, hole, deferred zero-measure, invalid;
- constraint role: Component 11 boundary, selected bridge, generated diagonal, residual-only edge, invalid;
- predicate query kind: orientation, point side, segment relation, endpoint sector, containment, area, separation, overlap, invalid;
- predicate disposition: definite accepted, exact symbolic tie accepted, definite rejected, uncertain retry/escalate, uncertainty exceeds tolerance, invalid;
- orientation escalation step: immediate, farther predecessor, farther successor, paired farther neighbors, maximal-chain turn, contour area, remaining-cell area, source support, exact-lineage tie, exhausted, invalid;
- orientation result: definite positive, positive within cleanup margin, proven zero cleanup, symbolic exact tie cleanup, definite negative, unresolved topology, non-finite, invalid;
- bridge disposition: accepted, endpoint-sector rejected, boundary crossing, diagonal crossing, excluded-domain crossing, uncertain, duplicate, resource rejected, invalid;
- ear disposition: accepted definite, accepted cleanup margin, accepted zero cleanup, accepted symbolic cleanup, reflex, blocked by occurrence, blocked by constraint, blocked by interior corner, duplicate corner, uncertain, stale generation, invalid;
- diagonal state: planned paired, one side assigned and reverse owned by active cell, both triangle sides assigned, invalid;
- triangle geometric category exactly matching the Component 12 specification;
- residual class: zero-area triangle, symbolic-tie triangle, zero-length-edge fan, collinear-chain cell, pinched-equal-coordinate cell, deferred zero-measure contour, unsupported, invalid;
- boundary assignment kind: triangle, residual patch, invalid;
- coverage disposition: verified, empty verified, residual-only verified, mismatch, uncertain, invalid;
- verifier disposition: accepted, rejected contract, rejected topology, rejected geometry, rejected codec, rejected determinism, invalid; and
- all resource, diagnostic, and failure-detail classifications required by Component 01.

Never serialize compiler enum names, RTTI, or implementation-defined underlying values. Use fixed-width explicit values.

### 3.3 Complete canonical keys

Define lexicographic complete keys. At minimum:

```text
triangulation_region_key =
    (Component 11 output_face_region complete key,
     prescribed orientation,
     occurrence/multiplicity sheet,
     support semantic key)

projected_occurrence_key =
    (triangulation_region_key,
     Component 11 output_vertex_occurrence complete key,
     support_frame complete key)

constraint_key =
    (triangulation_region_key,
     role,
     owning Component 11 cycle key or selected-bridge key,
     directed endpoint occurrence keys,
     predecessor halfedge/lineage key)

predicate_query_key =
    (provider and formula versions,
     triangulation_region_key,
     query kind,
     support_frame key,
     normalized ordered operand strong-ID keys,
     orientation/order parity)

bridge_key =
    (triangulation_region_key,
     hole cycle complete key,
     current containing-cell key,
     outer/current endpoint occurrence key,
     hole endpoint occurrence key,
     endpoint traversal-node lineage keys)

ear_candidate_key =
    (triangulation_region_key,
     active-cell complete key,
     candidate category rank,
     current traversal-node lineage key,
     predecessor occurrence key,
     current occurrence key,
     successor occurrence key,
     active generation)

internal_diagonal_key =
    (triangulation_region_key,
     unordered endpoint occurrence complete keys,
     decomposition role,
     creating bridge or ear key,
     side lineage)

triangle_key =
    (triangulation_region_key,
     canonical orientation-preserving rotation of three occurrence keys,
     three edge-use complete keys,
     creating candidate key,
     geometric category)

residual_key =
    (triangulation_region_key,
     residual class,
     canonical boundary-use cycle,
     sorted source obligations and provenance,
     creating candidate/escalation key)
```

The full record key must include any additional lineage required to distinguish two legal topology-distinct records with the same endpoints or nominal geometry. Hashes are lookup accelerators only and never identity. Equal complete keys are a duplicate/invariant failure unless the schema explicitly represents an aggregated member set.

Do not rank bridge or ear candidates by nominal length, area, angle, or quality in V1. Candidate category participates only after a structured bounded classification: definite positive and positive-within-cleanup-margin candidates precede proven-zero cleanup candidates. Within a category, lineage keys decide.

### 3.4 Artifact section layout

`triangulated_output_complex<T,I>` is an immutable, context-owned artifact with canonical contiguous sections in this order:

1. header, owner, operation, scalar/index descriptors, policy versions, provider versions, predecessor digests, and artifact versions;
2. region summaries in `triangulation_region_key` order;
3. support-frame selection and conditioning records;
4. projected occurrence records and Component 11 occurrence reverse maps;
5. projected boundary constraints and contour references;
6. shared planar predicate evidence and orientation escalation traces;
7. selected bridge records and visibility certificates;
8. internal diagonal paired-edge records and reciprocal internal halfedges;
9. canonical ordinary triangle records, corner references, and oriented edge-use references;
10. residual patches, cleanup obligations, and obligation-member ranges;
11. exact boundary-halfedge assignment table;
12. diagonal-to-two-cell assignment table;
13. region-to-triangle/residual-patch/diagonal/contour ranges and reverse maps;
14. boundary-conservation, connectivity, Euler, area, noncrossing, overlap, and coverage certificates;
15. source, retained-use, face-region, cycle, event, carrier, occurrence, and caller provenance maps;
16. precision-ledger references and a zero-cleanup-budget-use certificate;
17. resource and structural statistics;
18. canonical section digests and full artifact digest;
19. diagnostics and deterministic replay metadata; and
20. independent verifier report and verifier digest.

The artifact references Component 11 output occurrences, bounded coordinates, paired boundary edges, and boundary halfedges without changing their IDs or endpoint topology. It owns only Component 12 internal diagonal IDs, internal halfedges, triangles, residual records, and evidence.

Published records may reference only immutable predecessor storage and immutable stage-owned buffers whose lifetime covers Components 13-15. No view may reference active-ring nodes, generation tables, worker-local vectors, candidate queues, mutable indexes, stack objects, caller meshes, or temporary bridge duplicates.

### 3.5 Required record fields

Each support-frame record contains at least:

- frame ID/key and provider version;
- owning Component 11 region;
- selected source-facet/support-frame provenance;
- bounded origin and two in-plane basis directions or equivalent affine projection map;
- orientation parity from projected 2D to prescribed output 3D orientation;
- conditioning and plane-residual bounds;
- precision-ledger references;
- fallback-candidate audit where applicable; and
- record digest.

Each projected occurrence record contains at least:

- projected occurrence ID/key;
- Component 11 output occurrence ID/key;
- authoritative unchanged 3D bounded-point reference;
- bounded nominal 2D coordinates and conservative axis/radial enclosures;
- projection operation/formula evidence;
- frame and precision-ledger references;
- repeated-coordinate and zero-length descriptors inherited from Component 11;
- all region-local contour/traversal uses in canonical order; and
- record digest.

Each selected bridge record contains at least:

- bridge ID/key and owning region/hole/current cell;
- exact endpoint occurrence IDs and endpoint traversal-node lineage;
- the paired internal diagonal ID allocated for the bridge;
- endpoint-sector, containment, noncrossing, and excluded-hole evidence;
- complete rejected-candidate count/range or compact replayable rejection summary;
- the selected candidate's canonical rank;
- temporary ring splice references, which are excluded from the published topology;
- precision/provenance references; and
- record digest.

Each internal diagonal record contains at least:

- diagonal ID/key, role, and creating bridge/ear key;
- two reciprocal internal halfedge IDs in canonical slots;
- exact reversed endpoint occurrence IDs;
- owning Component 11 face region;
- exactly two final incident triangle assignments;
- visibility, noncrossing, and orientation evidence;
- confirmation that it is bookkeeping-only for source-facet semantic reasoning;
- source/region/provenance and precision references; and
- record digest.

Each triangle record contains at least:

- triangle ID/key and owning region;
- three distinct Component 11 output occurrence IDs in prescribed orientation;
- three oriented edge uses, each referencing either one unchanged Component 11 boundary halfedge or one Component 12 internal halfedge;
- three reciprocal-pair references;
- creating ear/cell key and acceptance order;
- projected bounded orientation and optional bounded 3D support-orientation evidence;
- geometric category;
- bounded area, altitude/edge measures required by cleanup policy, and precision references;
- source, retained-use, region, cycle, event, carrier, and caller provenance ranges;
- cleanup obligation reference when category is not definite positive area; and
- record digest.

Each residual patch/obligation contains at least:

- residual and obligation IDs/keys;
- owning region and Component 11 cycle(s);
- complete oriented boundary-use cycle or local patch interface;
- all involved output occurrence IDs, including topology-distinct equal-coordinate occurrences;
- local paired-edge and vertex-fan context;
- conceptual prescribed orientation;
- bounded area, edge-length, separation, support-plane, and uncertainty evidence;
- residual class and why an ordinary triangle was impossible or cleanup-required;
- candidate cleanup actions known to be topologically eligible, without selecting or budgeting one;
- forbidden merges, occurrence separations, multiplicity classes, source-feature restrictions, and topology-change restrictions;
- complete source/event/carrier/retained-use/precision provenance;
- member boundary-halfedge assignments;
- deterministic replay payload; and
- record digest.

## 4. Input validation, preflight, and resource reservation

### 4.1 Predecessor validation

Before projection or allocation, validate:

- all artifacts have the same live Component 01 context owner, operation, operand-role mapping, contact/output/degeneracy policies, scalar descriptor, and index descriptor;
- every required predecessor/provider version is supported and every required predecessor verification disposition is successful;
- complete predecessor digests recompute or match trusted immutable handles;
- every Component 11 output occurrence, paired edge, halfedge, region, cycle, contour relation, zero-measure record, support reference, and reverse map lies in range and has the promised canonical order;
- every Component 11 paired boundary edge has exactly two reciprocal halfedges with exact reversed topological endpoints;
- every boundary halfedge belongs to exactly one Component 11 face-region side and exactly one cycle;
- every positive-area region has exactly one outer contour and zero or more direct holes under V1;
- every positive-area cycle is closed, directed consistently with its contour role and prescribed region orientation, and duplicate-free in directed-halfedge membership;
- deferred zero-measure cycles are explicitly tagged and disjoint in directed-halfedge assignment from positive-area cycles;
- every output occurrence resolves to one finite authoritative Component 03 bounded point and non-shrinking precision-ledger reference;
- support-facet/frame provenance resolves through Components 04/05 and is compatible across each region;
- retained-use orientation, occurrence separation, and multiplicity agree with Components 10 and 11;
- event/carrier references resolve where present; and
- no predecessor record claims cleanup displacement or coordinate modification by Component 12.

A contradiction in a committed predecessor artifact is `internal_invariant_error`. Do not repair an open cycle, infer a missing contour, reverse a region, replace an endpoint, merge equal coordinates, or search for an alternative pair.

### 4.2 Checked upper bounds

For each positive-area Component 11 region define with checked arithmetic:

```text
B = total boundary halfedge uses across outer and hole contours
H = direct hole contour count
R = deferred zero-measure contour count
W = B + 2*H                     // weak-disk traversal nodes after all bridges
T_max = max(1, W - 2)           // ordinary or cleanup-required triangles for a nonempty bridged disk
D_ear_max = max(0, W - 3)       // ear-created diagonals
D_total_max = H + D_ear_max      // bridges plus ear diagonals
IH_max = 2 * D_total_max         // Component 12 internal halfedges
```

For ordinary nondegenerate input, the exact triangle count is `B + 2*H - 2` and the exact total internal-diagonal count is `B + 3*H - 3`. Cleanup-required triangles still count as triangles. Boundary-only residual patches may alter only explicitly versioned zero-measure accounting; they must not own a Component 12 internal diagonal. Require the checked upper bounds and final Euler/accounting certificates.

Preflight globally for:

- one region record per Component 11 positive-area region plus explicitly bounded zero-measure-only region support;
- projected occurrence uses and unique region-occurrence mappings;
- every boundary constraint and contour reference;
- up to `H` selected bridges per positive-area region;
- all internal diagonal pairs and two halfedges per pair;
- all ordinary/cleanup-required triangles, boundary-only residual patches, corner/edge-use references, assignments, and reverse maps;
- compute-once predicate evidence, escalation traces, bridge candidate diagnostics, active-ring generations, and rejection summaries;
- region-local blocker-index nodes and overlap candidate pairs;
- producer and verifier coverage evidence;
- canonical sort/merge/codec/replay workspace;
- diagnostics and shrinkable failure payloads; and
- configured abstract work for complete bridge enumeration, ear validation, predicate escalation, crossing checks, overlap checks, and independent verification.

Check strong-ID representability independently from public index type `I`, byte capacity, and work limits. Component 12 must not assume Component 13 will reduce counts to avoid overflow. Return `index_overflow` for representation failure and `resource_limit` for configured resource/work failure.

### 4.3 Resource plan

Reserve separately through Component 01 for:

- persistent projected records;
- persistent internal diagonals/halfedges;
- persistent triangles and residual obligations;
- persistent evidence, maps, codec, and replay bytes;
- peak per-region active-ring and candidate storage;
- peak bridge enumeration and predicate-cache storage;
- peak spatial-index and overlap-pair storage;
- verifier scratch and exhaustive-oracle scratch where enabled;
- sort/merge workspaces; and
- abstract work units for every topology-affecting query and verification pass.

Reserve before authoritative allocation. Per-region tasks receive fixed leases or checked subleases. Reconcile actual use at deterministic checkpoints. Transfer only verified persistent leases at stage commit. Resource exhaustion never permits a skipped visibility test, truncated candidate set, omitted overlap pair, or partial region artifact.

## 5. Support-frame selection and bounded projection

### 5.1 Frame selection

For each positive-area region, first use the authoritative oriented support frame supplied through Component 04/05/11 when present and compatible. If Component 11 permits deterministic recovery, enumerate candidate source support frames in complete source-facet/triangle lineage order and select the first candidate that Component 03 proves:

- finite and representable;
- nondegenerate with sufficient conditioning;
- orientation-compatible with the region's prescribed output direction;
- planar for every region occurrence within inherited precision and tolerance policy; and
- able to distinguish the region's required positive-area topology without an uncertainty collapse.

Do not use PCA, least-squares fitting, arbitrary coordinate extrema, a nominal largest normal component, or a platform-dependent normalization. Record every rejected candidate's stable reason when diagnostics policy requires it.

If no candidate passes, return `geometric_condition_exceeds_tolerance` with the region, candidate frames, plane residuals, conditioning bounds, and replay data.

### 5.2 Projection operation

Project every region occurrence exactly once through the selected Component 03 affine projection formula. Store:

- nominal projected `T` bits;
- conservative X/Y enclosures including source uncertainty and projection roundoff;
- radial/projected error where provided;
- exact formula/provider versions;
- source bounded-point and precision-ledger references;
- frame parity; and
- conditioning evidence.

Use the same projected record for every contour, bridge, ear, crossing, area, and verifier query. Do not independently reproject a point in another consumer or algebraically rearrange the formula.

Projection must handle finite extreme exponents, large translations, subnormals, and signed zero under Component 03 policy. A non-finite nominal/intermediate, invalid enclosure, nominal outside enclosure, or projection uncertainty that can erase required positive-area topology is a typed geometry failure.

### 5.3 Orientation relation

Freeze the projected positive orientation for a region from:

```text
projected_positive_sign =
    Component 11 prescribed 3D orientation
    combined with selected support-frame parity
```

All bridge sector tests, ear orientations, contour area signs, triangle corner order, and canonical rotations use this relation. Never infer orientation from a provider's emitted winding and reverse it after publication.

### 5.4 Projection audit

After projection, verify:

- one projected record exists for each distinct `(region, output occurrence)` use;
- every contour traversal resolves to the correct projected occurrence without coordinate lookup;
- equal projected coordinates retain separate occurrence IDs and local-node uses;
- zero-nominal-length constraints remain present;
- all plane residuals and projected enclosures are finite and policy-compatible;
- source/event identity and precision lineage are unchanged; and
- no tolerance or cleanup-budget record was created.

## 6. Region-local occurrence-preserving constraint model

### 6.1 Boundary constraints

Create one immutable region-local directed boundary constraint for every Component 11 boundary halfedge. Preserve its exact predecessor ID, pair, direction, endpoints, edge role, cycle, region, source/event/carrier provenance, zero-length descriptor, and occurrence-separation class.

A boundary constraint is never deleted or replaced. Ear clipping may change which cell is incident on its region side, but the final assignment table must refer to the same Component 11 halfedge.

### 6.2 Traversal nodes

Create one private traversal node per directed corner visit in each Component 11 cycle. A traversal node contains:

- owning cycle and contour role;
- previous/current/next boundary-halfedge use;
- current output occurrence ID and projected occurrence reference;
- stable predecessor lineage key;
- active-cell membership and generation; and
- temporary bridge-splice role where applicable.

Several traversal nodes may reference the same output occurrence, and distinct occurrences may have equal projected coordinates. Neither condition permits merging. Temporary bridge duplication creates traversal nodes only; it never creates a new output occurrence or published coordinate.

### 6.3 Contour audit

For each region:

- reconstruct every cycle from predecessor halfedge successor records, not coordinates;
- compare the exact halfedge sequence and canonical rotation with Component 11;
- verify one outer and all direct holes are assigned once;
- verify outer/hole projected orientation through bounded contour-area evidence when definite;
- retain an uncertain or zero-measure contour only when Component 11 explicitly tags it and the Component 12 residual policy supports it;
- reject a definite positive-area self-crossing boundary or definite crossing between nonincident contours;
- fail on unresolved contour topology that can alter positive-area coverage; and
- create exact reverse maps from cycles, boundary halfedges, and occurrences to local records.

Do not recalculate nesting from nominal point-in-polygon tests. Component 11 contour hierarchy is authoritative and is only defensively audited.

### 6.4 Active cells

After hole integration, represent each untriangulated region piece as a private active cell with:

- a circular doubly linked sequence of traversal nodes;
- one oriented edge use between each adjacent node;
- an exact current boundary-use multiset;
- a stable cell lineage key derived from the region and creating split;
- a generation incremented after every local topology change;
- candidate dependency ranges;
- remaining-cell area evidence; and
- a strict finite progress counter.

No active-cell ID or mutable link escapes publication. Stale candidates are detected by cell and node generations.

## 7. Compute-once bounded planar predicate registry

### 7.1 Canonical query formation

Every topology-affecting 2D relation is requested through one region-local registry keyed by `predicate_query_key`. Normalize symmetric operands and record orientation parity so mathematically identical queries map to one key. Required queries include:

- orientation of three projected occurrences/traversal uses;
- point side relative to a directed constraint;
- segment relation with proper, endpoint, overlap, tied, absent, uncertain, and invalid outcomes;
- endpoint-sector membership for a candidate bridge or diagonal;
- point/triangle and point/contour membership;
- bounded contour, cell, and triangle signed area;
- closed AABB overlap and definite separation;
- support-plane residual/orientation checks; and
- aggregate interval reductions used by escalation or coverage.

The first request computes the prescribed Component 03 operation graph and stores immutable evidence. Later requests reuse the same evidence ID. Equivalent tests must not be independently evaluated with another formula or operand order.

### 7.2 Structured result requirements

Each evidence record retains:

- query key, formula/provider versions, and operands;
- nominal result bits where applicable;
- conservative enclosure/margins;
- exact-nominal sign or exact-zero evidence when requested;
- inherited precision and operation-ledger references;
- conditioning and uncertainty cause;
- deterministic policy disposition;
- consumers and replay references; and
- record digest.

A scalar epsilon comparison is prohibited. User tolerance may only participate through the specifically versioned geometric-condition or cleanup-category policy.

### 7.3 Exact ties versus uncertainty

Treat these separately:

- **definite relation:** enclosure proves the required sign/separation;
- **exact representational tie:** exact-nominal expansion proves zero and the frozen symbolic/lineage policy authorizes one bookkeeping disposition;
- **bounded uncertainty:** enclosure overlaps multiple topologically different outcomes;
- **invalid:** non-finite, unsupported, contradictory, or malformed evidence.

An exact tie may use the frozen lineage rule only for topology-preserving bookkeeping explicitly listed in capabilities. Bounded uncertainty that can change occupied positive area does not become an exact tie and must escalate or fail.

## 8. Frozen orientation escalation

### 8.1 Escalation sequence

For an active corner `(p, c, n)`, use this exact V1 sequence, stopping at the first decisive permitted result and recording every step:

1. bounded immediate orientation `(p,c,n)`;
2. bounded orientation using the nearest previous distinct topological occurrence with `(c,n)`;
3. bounded orientation using `(p,c)` with the nearest next distinct topological occurrence;
4. paired farther-neighbor orientations using the nearest distinct occurrences on both sides;
5. bounded aggregate signed turn over the maximal canonical near-collinear chain containing `c`;
6. bounded signed area contribution of that chain within the current active cell;
7. bounded signed area and prescribed orientation of the complete remaining active cell;
8. authoritative source-facet/support orientation and retained-incidence evidence;
9. frozen exact-lineage tie rule, only when exact-nominal zero is proven and the result is topology-preserving; and
10. exhausted/unresolved.

“Nearest” means minimum cyclic step count, then complete traversal-node lineage key. Do not choose by geometric distance.

### 8.2 Result classification

Classify the corner/triangle as:

- `definite_positive_area` when the required sign and accepted area/altitude margins are definitely positive;
- `positive_area_within_cleanup_margin` when orientation is definitely correct but one configured bounded measure requires Component 13 review;
- `zero_area_cleanup_required` when aggregate evidence proves no resolvable positive-area contribution and a cleanup triangle/cell preserves all topology;
- `symbolic_exact_tie_cleanup_required` when exact representational equality plus the frozen policy preserves intended topology;
- invalid inverted when the required sign is definitely negative;
- invalid uncertain positive-area topology when remaining alternatives can change coverage beyond policy; or
- invalid non-finite.

Only the first four may enter a proposed region result. A definite negative ear is rejected as reflex or invalid according to context; a final unavoidable inversion is failure. An unresolved orientation must not be retried without a changed active generation or new aggregate evidence.

### 8.3 Escalation cache and limits

Cache escalation by active-cell generation and corner lineage. Charge every step. Bound maximum farther-neighbor walks by current cell size and aggregate scans by configured work. Record the terminal reason. Repeatedly deferring the same unchanged state is an internal progress failure, not a loop.

## 9. Canonical hole integration

### 9.1 Processing order

Process direct hole contours in Component 11 complete cycle-key order. The current containing active boundary begins as the outer contour plus previously selected bridge splices. Do not order holes by nominal rightmost coordinate, area, or size.

### 9.2 Complete bridge candidate set

For the selected hole, enumerate every pair of:

- one current containing-boundary traversal node; and
- one traversal node of the hole,

in `bridge_key` order. A conservative spatial index may skip a candidate only when Component 03 proves its segment bounds cannot meet the required endpoint/domain conditions or are definitely blocked. It must not omit a potentially admissible pair. Complete enumeration remains the correctness fallback and is charged to work limits.

### 9.3 Bridge admissibility certificate

A bridge candidate is admissible only when all are true:

1. endpoints are existing output occurrences in the same region and are permitted to connect by occurrence-separation/multiplicity policy;
2. the two endpoint traversal nodes are not already adjacent through the same topological constraint;
3. the open bridge lies in the intended positive-area domain under bounded endpoint-sector tests and canonical containment evidence;
4. it has no forbidden proper crossing or positive-length overlap with any nonincident outer, hole, deferred, selected-bridge, or accepted-diagonal constraint;
5. endpoint contact with an incident constraint is exactly the permitted endpoint relation;
6. topology-distinct equal-coordinate contact is accepted only by an explicit exact-tie rule and complete occurrence evidence;
7. the bridge does not enter or fill an excluded hole;
8. adding it keeps the cell decomposition combinatorially valid;
9. both directed sides can be allocated as one reciprocal internal pair; and
10. uncertainty is within the accepted policy.

Use bounded endpoint-sector tests plus noncrossing and domain-containment evidence; no nominal midpoint alone is sufficient. A nominally shortest visible bridge has no preference in V1.

### 9.4 Selection and splice

Select the first admissible candidate in complete key order. Allocate its reciprocal internal diagonal pair privately before modifying the active topology. Splice the hole into the disk traversal with two temporary traversal uses of the bridge, each referencing one opposite internal halfedge. The duplicated traversal endpoints reference existing occurrences; no new output vertex is created.

If no candidate is admissible:

- emit a residual only when the complete hole or local structure is already an explicitly supported zero-measure obligation and no positive-area domain is lost;
- return `geometric_condition_exceeds_tolerance` when uncertainty prevents a safe bridge or alternative legal bridges can change positive-area coverage; or
- return an invariant failure when the committed contour hierarchy contradicts its promised admissible domain.

Never drop, fill, merge, or shrink a hole.

## 10. Bounded indexed ear decomposition

### 10.1 Initial candidate creation

After all holes are bridged, build one active disk cell. For every active traversal node, create or reject one current ear candidate using:

- previous/current/next node generations;
- three output occurrence IDs;
- orientation escalation result;
- proposed closing diagonal `(previous,next)`;
- exact edge-use cycle;
- bounded candidate triangle bounds;
- blocker-index query results;
- excluded-domain and noncrossing evidence; and
- complete candidate key.

A candidate with repeated output occurrence IDs is never a triangle. It may become a supported boundary-only residual candidate only when the entire residual boundary uses Component 11 constraints, owns no Component 12 internal diagonal, and aggregate evidence proves zero measure; otherwise the current triangulation path fails.

### 10.2 Blocker index

Maintain a deterministic region-local immutable-or-rebuilt flat index over active projected occurrence bounds and current constraint bounds. A candidate triangle query returns all records whose conservative bounds are not definitely separated. Sort candidates by complete topological key before exact relation tests.

The index is only an accelerator. Every possible blocker admitted by Component 03 bounds must be returned. Rebuild or patch it at fixed generation thresholds specified by the provider version. Index layout and threshold changes require a provider version change.

### 10.3 Ordinary ear admissibility

An ordinary ear is admissible only when:

- the three occurrence IDs are distinct;
- orientation classification is `definite_positive_area` or `positive_area_within_cleanup_margin`;
- the closing edge is an existing permitted current-cell edge or an admissible new internal diagonal;
- the triangle uses exactly three oriented edge uses forming a cycle in prescribed orientation;
- no other active traversal occurrence that is topologically nonincident lies definitely inside the candidate triangle;
- no current boundary, bridge, or accepted diagonal has a forbidden relation with the candidate interior or closing diagonal;
- no excluded hole interior is included;
- removing the current node leaves a valid active-cell boundary or completes the final cell;
- all predicate evidence is decisive under policy; and
- the action strictly decreases the progress tuple.

A topology-distinct occurrence at the same projected coordinate is still tested and retained. Coordinate equality never causes a blocker to be ignored.

### 10.4 Candidate queue and selection

Maintain candidates in a deterministic ordered vector/heap keyed by `ear_candidate_key`, with explicit generation validation on pop. V1 category priority is:

1. definite positive area;
2. positive area within cleanup margin;
3. proven zero-area cleanup;
4. symbolic exact-tie cleanup.

Within a category use complete lineage only. Stale candidates are discarded and regenerated for affected nodes. Queue insertion order, hash order, worker timing, memory address, and floating quality scores are irrelevant.

Accept positive-area candidates while any valid positive-area candidate exists. Consume a cleanup-required candidate only when no valid positive-area candidate exists in the current generation and its residual/triangle handoff is completely certified. This rule is part of the provider version.

### 10.5 Ear acceptance transaction

For each accepted ear:

1. revalidate cell/node generations and the exact three-edge local topology;
2. reuse or recompute-by-key all required predicate evidence;
3. reserve required diagonal, triangle or boundary-only residual, evidence, map, and replay resources;
4. create the closing internal diagonal pair atomically when the closing edge is not already present;
5. build the ordinary/cleanup-required triangle privately, or build a boundary-only residual patch only under Section 12 eligibility;
6. assign the two existing edge uses and the appropriate closing halfedge side;
7. transfer the reverse closing halfedge to the remaining active cell as a pending owned edge use;
8. verify local pair reciprocity, orientation, boundary assignment uniqueness, and cell closure;
9. remove the ear node/update the active ring and increment affected generations;
10. append the acceptance/progress record;
11. regenerate only affected candidates; and
12. commit the nested private action.

Failure before step 12 leaves the active topology, pair table, candidate generations, assignments, resource counters, diagnostics, and replay unchanged.

### 10.6 Final cell

When exactly three traversal nodes remain, audit them through the same predicate, orientation, assignment, and category rules. Emit one ordinary or cleanup-required triangle if the three occurrence IDs are distinct. If they are not distinct, emit a residual patch only when all three boundary uses are unchanged Component 11 boundary halfedges, no Component 12 internal diagonal is incident, and the complete structure is provably zero measure; otherwise fail with `unsupported_degeneracy`. Do not bypass checks because no new diagonal is required.

### 10.7 Strict progress and termination

Use the lexicographic progress tuple:

```text
(unintegrated_hole_count,
 active_positive_area_cell_count,
 total_active_traversal_node_count,
 unresolved_zero_measure_obligation_count,
 accepted_action_count_remaining_bound)
```

Every selected bridge reduces the first item. Every accepted ear/residual extraction reduces active node count or closes a cell. No operation may leave the tuple unchanged. A deferred candidate is reconsidered only after a dependency generation changes.

Bound every active-ring walk by current node count, every candidate retry by generation changes, and total accepted actions by the checked preflight maximum. If the queue contains no admissible candidate and no supported residual handoff can reduce state, return a typed geometry failure with the complete stalled cell and predicate evidence. Never randomly perturb, silently relax, or loop.

## 11. Atomic internal diagonals and cell materialization

### 11.1 Pair-at-creation

Allocate every selected bridge or ear diagonal as one private record containing two reciprocal internal halfedges. Canonical halfedge slots are derived from complete directed keys, not creation order. Immediately set and validate:

```text
h0.pair = h1.id
h1.pair = h0.id
h0.origin = h1.destination
h0.destination = h1.origin
h1.origin = h0.destination
h1.destination = h0.origin
```

The pair state begins as `planned_paired`. During ear extraction, one halfedge may become incident to the emitted triangle and the reverse halfedge becomes an explicit edge use owned by the remaining active cell. This is `one_side_assigned_and_reverse_owned_by_active_cell`, not a published half-edge. Before region finalization every diagonal must be `both_triangle_sides_assigned`, with exactly two opposite directed triangle uses. A residual patch may not own or terminate a Component 12 internal diagonal.

No downstream artifact can observe an unpaired or one-sided diagonal.

### 11.2 Boundary edge use

A triangle or residual-patch boundary-assignment record referencing a Component 11 boundary halfedge stores that exact predecessor ID. It does not allocate a replacement Component 12 halfedge. Verify the cross-face reciprocal pair remains unchanged and is not consumed on the opposite region side by this region's triangulation.

Every boundary halfedge receives exactly one assignment on its incident region side. Assignment occurs only when the owning triangle or boundary-only residual patch commits.

### 11.3 Triangle construction

Construct corner order from the active-cell oriented boundary. Canonicalize by orientation-preserving rotation only; never reverse a triangle after acceptance. Verify:

- three distinct occurrence IDs;
- edge endpoints match consecutive corners exactly;
- all three edge uses have pair references;
- region ownership and source support agree;
- bounded projected orientation has the required sign/category;
- bounded 3D support orientation agrees where capabilities require it;
- candidate-domain and noncrossing evidence remain valid; and
- provenance ranges are complete.

### 11.4 No source-semantic leakage

Mark every Component 12 diagonal as `internal_triangulation_diagonal`. It must not become an original source edge, Boolean classification barrier, event carrier, symbolic owner, or retained-use boundary in downstream semantic audits. Component 13 may remove or replace it only under cleanup certificates; Components 14/15 treat it as output triangle adjacency.

## 12. Degenerate triangle and residual-patch handoff

### 12.1 Supported V1 residual forms

V1 supports only these explicit forms:

- a three-distinct-occurrence cleanup-required triangle with zero/sub-threshold bounded area and one cleanup obligation;
- a three-distinct-occurrence symbolic exact-tie triangle and one cleanup obligation;
- a zero-length-edge fan obligation attached to one or more cleanup-required triangles with complete paired context;
- a maximal collinear-chain obligation attached to cleanup-required triangles while preserving every chain occurrence;
- a pinched equal-coordinate obligation with distinct occurrence-separation constraints; and
- a boundary-only residual patch for a deferred zero-measure contour supplied by Component 11, provided it owns no Component 12 internal diagonal.

The first five forms remain triangle topology: every Component 12 internal diagonal still has two triangle uses. A non-triangle residual patch is permitted only for a predecessor zero-measure boundary structure that can be represented entirely by Component 11 boundary halfedges and occurrence/fan context. Any other residual form is `unsupported_degeneracy` or `geometric_condition_exceeds_tolerance`; it is not hidden in diagnostics.

### 12.2 Residual eligibility

Emit a residual only when all are true:

- complete positive-area coverage outside the residual remains provable;
- aggregate evidence proves the residual's unresolved geometry is zero-measure or confined within the cleanup handoff policy;
- every involved boundary halfedge has an exact triangle or boundary-only residual assignment, and every involved internal halfedge has an exact triangle assignment;
- conceptual orientation and local fan/interface are unambiguous;
- no occurrence-separation, multiplicity, source-feature, or cross-face pair rule is violated;
- Component 13 has an advertised compatible residual schema/action capability; and
- no cleanup displacement or feature-removal budget is reserved or committed.

If alternative residual interpretations can change positive-area coverage, fail.

### 12.3 Obligation aggregation

Create one obligation per canonical residual key. Multiple local records may aggregate only when their exact member set, owning region, topology interface, and forbidden-merge constraints are recorded. Every Component 11 deferred zero-measure record, every non-definite triangle, and every boundary-only residual patch receives exactly one final obligation membership.

Component 13 must be able to discover candidate cleanup actions directly from the artifact. It must not reconstruct polygon contours, guess conceptual orientation, or infer which equal-coordinate occurrences may merge.

### 12.4 Zero budget use certificate

Publish a certificate showing:

- no output coordinate changed;
- no Component 03 tolerance-budget reservation or commit was made by Component 12;
- no edge, contour, face region, occurrence, or component was deleted;
- added precision-ledger records are projection/predicate/verification uncertainty only; and
- inherited precision was not reduced.

## 13. Crossing, overlap, and local geometric admissibility

### 13.1 Diagonal noncrossing

Before accepting a bridge or ear diagonal, query all non-definitely-separated current constraints. Distinguish:

- its own incident endpoints;
- a shared paired diagonal;
- a shared boundary endpoint;
- a topology-distinct equal-coordinate contact;
- a permitted exact tie;
- collinear/overlapping zero-measure geometry covered by cleanup-required triangles or a permitted boundary-only residual obligation;
- forbidden proper crossing;
- forbidden positive-length overlap; and
- unresolved uncertainty.

Forbidden crossing/overlap rejects the candidate or fails the final region if unavoidable. Uncertainty that can change positive-area topology fails.

### 13.2 Triangle overlap audit

After a region is complete, build a fresh immutable producer overlap index over triangle projected AABBs and a separate zero-measure residual-bound audit. Enumerate every unordered non-definitely-separated cell pair once in complete pair-key order. For each pair:

- exclude the exact permitted shared vertex or reciprocal shared edge relation;
- test all nonincident edge pairs through the compute-once relation registry;
- test contained positive-area vertices/regions where edge relations alone are insufficient;
- distinguish topology-distinct equal-coordinate and zero-measure residual contact;
- reject any forbidden proper crossing or positive-area overlap; and
- fail on unresolved positive-area overlap uncertainty.

A shared region owner does not excuse overlap. Area cancellation is not evidence of nonoverlap.

### 13.3 Missing-pocket checks

In addition to boundary/Euler/area checks, verify no bounded active cell remains unassigned and no contour sector lacks a triangle or residual-patch assignment. At higher verification levels, use deterministic representative probes for each reconstructed planar cell and ensure exactly one accepted triangle covers each positive-area cell and no triangle covers an excluded hole cell.

## 14. Boundary conservation, topology, and coverage certificates

### 14.1 Boundary multiset conservation

For each region, construct the canonical multiset of Component 11 boundary halfedge IDs and compare it exactly with the final triangle/residual-patch assignment multiset. Require:

- every expected halfedge appears once;
- no foreign halfedge appears;
- direction/endpoints/pair/region/cycle/role are unchanged;
- no boundary halfedge is replaced by a coordinate-equal internal diagonal; and
- all reverse maps are bijective.

Perform a global audit after region merge to ensure every Component 11 boundary halfedge side is accounted for exactly once across the artifact.

### 14.2 Internal pair balance

For every internal diagonal require:

- exactly two reciprocal halfedges;
- exact reversed occurrence endpoints;
- exactly two opposite directed final triangle uses in the same Component 11 region;
- no third use and no pending active-cell owner;
- compatible orientation on both incident cells; and
- complete visibility/noncrossing evidence.

### 14.3 Cell closure and connectivity

Verify every triangle cycle and every residual-patch boundary/interface record closes by exact IDs. Build positive-area triangle adjacency solely from reciprocal internal diagonal pairs. Require the positive-area triangulated domain corresponding to each Component 11 region component to be connected. A residual patch may attach only through recorded Component 11 boundary/occurrence context and may not terminate an internal diagonal.

### 14.4 Euler certificate

Recompute unique region occurrence vertices, boundary plus internal paired edges, ordinary/cleanup-required triangles, residual patches, contour count, and connected components from final records. For a connected orientable planar region with `H` direct holes, require the versioned cell-complex relation equivalent to:

```text
V - E + F = 1 - H
```

where `F` counts ordinary and cleanup-required triangles and excludes the exterior and hole interiors. Any boundary-only zero-measure residual correction is explicit, versioned, and independently verified; it never converts an internal diagonal from a two-triangle edge. Record any zero-measure correction term explicitly; never silently alter Euler counts to make them match.

### 14.5 Bounded area certificate

Using Component 03 canonical interval reductions:

- compute oriented outer contour area minus hole areas;
- compute the sum of ordinary triangle oriented areas;
- account for cleanup-margin and zero-measure residual bounds separately;
- combine intervals in canonical record order; and
- require conservative agreement under the frozen coverage policy.

Do not use `long double`, unordered reduction, or area equality alone. The certificate is valid only together with boundary, pair, connectivity, Euler, noncrossing, and overlap evidence.

### 14.6 Orientation certificate

Every definite or cleanup-margin positive triangle must agree with projected frame parity and prescribed 3D output orientation. Every zero-measure residual must retain conceptual orientation and boundary order. A verifier may not repair a reversed cell.

### 14.7 Complete coverage disposition

A region receives `verified` only after all required certificates pass. A region containing residuals receives `residual_only_verified` only when positive-area coverage is complete and every residual is bounded and assigned. An empty artifact receives `empty_verified`. Any mismatch or unresolved positive-area ambiguity prevents publication.

## 15. Canonical merge, dense IDs, codec, and replay

### 15.1 Region-private proposals

Each region task emits a complete private proposal keyed entirely by predecessor complete keys. It may contain temporary local IDs, but no global dense ID. A task proposal includes all projection, predicates, bridges, diagonals, cells, residuals, assignments, certificates, counters, diagnostics, and replay necessary to validate that region independently.

### 15.2 Canonical global merge

After all region tasks succeed:

1. sort proposals by `triangulation_region_key`;
2. select the primary failure by Component 01 deterministic failure key if any task failed;
3. merge shared predecessor references and evidence by complete key with equality validation;
4. sort every owned record table by its complete key;
5. reject duplicate complete keys unless the table has an explicit aggregation rule;
6. assign dense strong IDs only after full sorting;
7. rewrite all local references through checked remap tables;
8. build CSR ranges and reverse maps;
9. perform the global boundary/occurrence/pair audit; and
10. discard all task-local mutable storage before publication.

Worker completion order cannot affect output.

### 15.3 Canonical bytes

Encode fixed-width fields explicitly through Component 01 canonical bytes. Include:

- all provider/schema/policy versions;
- scalar bits through Component 03 canonical scalar encoding;
- strong-ID domains and values;
- complete keys and canonical ranges;
- all evidence and provenance needed for verification;
- structural/resource counters;
- zero-budget-use certificate;
- predecessor section/full digests;
- verifier disposition/digest; and
- replay metadata.

Do not serialize raw structs, padding, pointers, allocator state, locale text, exception messages, unordered iteration, or platform type names.

### 15.4 Replay payload

The replay record must reproduce:

- selected frame and every rejected frame reason required by policy;
- projected record formula references;
- contour and bridge candidate order;
- every predicate query/result/consumer;
- every escalation step;
- active-cell generation changes;
- every accepted/rejected/deferred ear;
- every diagonal pair state transition;
- every triangle and residual-patch boundary assignment;
- coverage/overlap candidate order and result;
- resource/cancellation checkpoints; and
- canonical remap and digest construction.

A replay run under the same versions must produce byte-identical artifact bytes or the same deterministic typed failure.

## 16. Independent verifier

### 16.1 Independence boundaries

`TriangulationVerifier` must not call the producer's:

- bridge chooser;
- active-ring updater;
- ear test or candidate queue;
- diagonal allocator;
- triangle acceptance helper;
- residual aggregator;
- producer overlap-pair traversal; or
- coverage accumulator.

It may use shared immutable schemas, Component 01 checked accessors/canonical bytes/SHA-256, and Component 03 bounded arithmetic/predicate primitives. Shared primitive use does not permit shared grouping or decision control flow.

### 16.2 Verifier reconstruction

The verifier independently:

1. validates header, owners, versions, ranges, enums, reserved fields, and digests;
2. reconstructs Component 11 boundary expectations directly from the predecessor artifact;
3. reconstructs projected coordinates from the selected frame and source bounded points;
4. reconstructs every cell cycle from corner and edge-use records;
5. reconstructs reciprocal internal pairs and exact cell adjacency;
6. rebuilds boundary assignments, region connectivity, and vertex/edge/cell counts;
7. recomputes Euler and orientation relations;
8. recomputes bounded contour and triangle area through an independently grouped canonical reduction;
9. enumerates possible crossings/overlaps with an independent sorted-endpoint sweep or exhaustive all-pairs path below a fixed threshold;
10. verifies holes remain excluded and no positive-area cell is omitted;
11. verifies every residual obligation and Component 13 compatibility;
12. verifies no coordinate moved and no cleanup budget was spent;
13. validates complete-key sort order, dense IDs, reverse maps, codec bytes, and digest; and
14. emits a verifier digest/report.

The verifier must reject a producer artifact even when counts and top-level digest fields were forged consistently by a mutation fixture.

### 16.3 Bounded exhaustive oracle

For configured small regions, dispatch to a test-only independent oracle that:

- uses exact integer/rational projected coordinates for integer/power-of-two fixtures;
- enumerates all legal noncrossing diagonals and bridge sets;
- constructs all legal constrained triangulations or exact planar cells within the threshold;
- verifies the producer's boundary, holes, occupied cells, and residual classifications;
- proves at least one legal triangulation exists for expected-success fixtures; and
- serializes any disagreement for shrinking.

Production code never depends on arbitrary-precision arithmetic or exhaustive enumeration.

### 16.4 Mutation rejection inventory

At minimum, the verifier must reject mutations that:

- remove, duplicate, reverse, reassign, or replace one Component 11 boundary halfedge;
- change a boundary endpoint or cross-face pair;
- merge two repeated-coordinate occurrences;
- omit or fill a hole;
- change contour role or hierarchy;
- create an unpaired, one-sided, three-use, crossed, or wrong-endpoint diagonal;
- reverse a triangle, duplicate it, delete it, or alter one corner;
- introduce positive-area overlap or a missing pocket while preserving total area;
- change a geometric category or orientation evidence;
- omit or misclassify a residual obligation;
- forge a bridge visibility or escalation result;
- claim cleanup budget use is zero after changing a coordinate;
- scramble canonical IDs/order/reverse maps;
- forge resource counters, codec bytes, section digests, full digest, or verifier report; and
- insert unknown enums, versions, nonzero reserved fields, stale owners, or out-of-range IDs.

## 17. Typed failures, diagnostics, and deterministic arbitration

### 17.1 Component 12 subcodes

Allocate a disjoint Component 12 subcode range including at least:

- unsupported triangulation capability/version;
- predecessor owner/version/digest mismatch;
- invalid region/cycle/contour range;
- boundary pair or endpoint contradiction;
- unsupported contour hierarchy;
- support frame unavailable or orientation-incompatible;
- support plane residual exceeded;
- projection non-finite, invalid, or ill-conditioned;
- projected topology uncertainty exceeds tolerance;
- definite unrepresented boundary crossing;
- unresolved contour topology;
- bridge candidate set exhausted;
- bridge endpoint-sector failure;
- bridge crossing/overlap;
- bridge visibility uncertainty;
- active-ring corruption or stale-generation misuse;
- orientation escalation exhausted;
- inverted required triangle;
- ear candidate set stalled;
- closing diagonal inadmissible;
- internal diagonal pair/assignment failure;
- boundary halfedge unassigned or multiply assigned;
- unsupported residual form;
- residual positive-area ambiguity;
- triangle or residual-patch cycle/interface closure failure;
- connectivity or Euler mismatch;
- bounded area mismatch;
- forbidden triangle crossing or positive-area overlap;
- coverage missing pocket;
- hidden coordinate change or cleanup budget use;
- canonicalization/reverse-map failure;
- codec/digest/replay failure;
- independent verifier disagreement;
- ID/count/byte overflow;
- resource limit;
- cancellation; and
- internal progress/invariant failure.

Map them to the stable public categories from Component 01. Expected conditioning/coverage ambiguity is normally `geometric_condition_exceeds_tolerance`; malformed caller policy is `input_contract_error` or `invalid_tolerance`; representability is `index_overflow`; configured capacity is `resource_limit`; predecessor contradiction or producer/verifier disagreement is `internal_invariant_error`.

### 17.2 Failure payload

Every failure includes, where applicable:

- stage/checkpoint/subcode and stable primary failure key;
- context owner and all provider/policy versions;
- Component 11 region, cycles, halfedges, and occurrence complete keys;
- support frame and projected bounded values;
- active cell generation and canonical boundary-use cycle;
- bridge/ear/diagonal/triangle/residual-patch candidate key;
- exact predicate evidence and escalation trace;
- resource counters and cancellation checkpoint;
- predecessor and partial canonical input digests; and
- a bounded deterministic replay payload.

Do not rely on free-form exception text as the only diagnostic.

### 17.3 Failure arbitration

For parallel region tasks, collect all failures after joining workers and choose the primary error by Component 01's full deterministic failure key, beginning with stable checkpoint/subcode and complete region/candidate lineage. Do not choose first worker, first clock time, or first exception. No partial artifact publishes.

## 18. Transactionality, cancellation, and concurrency

### 18.1 Stage transaction

All persistent output is built in private stage-owned storage. Region completion does not publish a visible artifact. Global merge, codec, and independent verification occur before the single stage commit.

### 18.2 Nested action rollback

Bridge selection and ear acceptance use nested private transactions. On failure, restore exactly:

- active-ring links and generations;
- pending diagonal table and pair states;
- triangle/residual-patch tables;
- boundary and internal-halfedge assignments;
- candidate queue and dependency generations;
- predicate consumer ranges;
- resource leases/counters;
- diagnostics and replay append positions; and
- progress counters.

### 18.3 Cancellation

Poll at fixed work intervals. On cancellation:

- stop creating new tasks/actions;
- allow active tasks to reach a deterministic safe point;
- join every worker;
- roll back all nested and stage transactions;
- return all resource leases;
- publish no region, diagonal, triangle, residual, evidence, codec, or replay artifact; and
- return `cancelled` with stable checkpoint and replay context.

### 18.4 Permitted parallelism

V1 may parallelize independent face regions only. Within one region, bridge selection and ear acceptance remain serial according to the canonical provider because each action changes the candidate domain. Producer overlap checks and verifier pair checks may use deterministic fixed partitions with private outputs and canonical merge.

No task mutates a shared artifact, predicate cache, resource counter, diagnostic stream, or ID allocator without deterministic ownership. Thread counts 1, 2, and maximum must produce identical bytes or identical failure.

## 19. Test and qualification plan

### 19.1 Contract and basic known-answer tests

Cover:

- empty artifact;
- one valid triangle;
- convex polygons of sizes 4 through at least 32;
- concave polygons with one and many reflex vertices;
- orthogonal and star-shaped polygons;
- long collinear chains;
- annuli, several sibling holes, and Component 11-compatible nested-region arrangements;
- triangle-only cleanup categories; and
- invalid owners, versions, ranges, pairs, contour roles, support frames, and digests.

Verify exact boundary assignments, triangle orientation, diagonal pairs, expected nondegenerate counts, canonical IDs, and independent coverage.

### 19.2 Existing-provider regression tests

Create fixtures demonstrating why the bounded provider must not call existing monotone/CDT APIs:

- repeated coordinate occurrences that the legacy routines merge/reject;
- zero-length topological constraints;
- collinear vertices that must retain provenance;
- touching outer/hole occurrences;
- cleanup-required zero-area cells;
- large-translation cases where `long double` area/shortest-bridge policy is not the contract; and
- mutable public-mesh output lacking boundary-halfedge identity.

These tests qualify the reuse decision; they do not require changing legacy API behavior.

### 19.3 Boolean-shaped corpus

Use actual or faithfully serialized Component 08-11 artifacts from:

- transverse box/box intersections;
- oblique convex-polytope intersections;
- concave extrusions;
- partial coplanar overlaps;
- equal operands with different source triangulations;
- cavities, islands, and disconnected shells;
- several carrier loops on one source facet;
- intersection curves through original vertices and edges;
- thin corridors and needle-like regions;
- point/edge touching outputs with duplicated occurrences; and
- repeated Boolean chains with inherited precision.

Hand-authored simple polygons alone are not sufficient.

### 19.4 Projection tests

Test authoritative and fallback frames under:

- axis permutations and sign flips;
- exactly representable translations;
- power-of-two scaling;
- extreme exponents and mixed magnitudes;
- subnormal coordinates and signed zero;
- nearly degenerate support triangles;
- plane residuals just below, at, and above allowed conditioning; and
- source-triangle/facet permutation.

Require byte-identical selected frame/projections after documented remapping and correct typed failure when conditioning is insufficient.

### 19.5 Repeated-coordinate and zero-length tests

Include:

- two distinct occurrences with bit-identical projected coordinates;
- several equal-coordinate occurrences separated around one contour;
- zero-nominal-length boundary edges;
- outer/hole point touch represented by distinct occurrences;
- topology-distinct overlap of contour vertices;
- equal-parameter event clusters;
- repeated visits to one occurrence through different traversal nodes where permitted; and
- collinear chains whose middle occurrences have distinct provenance obligations.

Verify termination, no welding/deletion, exact boundary assignment, residual obligations, stable topology, and mutation rejection.

### 19.6 Hole bridge tests

Test:

- several admissible bridges with canonical-lineage selection;
- one narrow but definite bridge;
- endpoint-only contact;
- topology-distinct equal-coordinate endpoint contact;
- a nominally shortest bridge that crosses a constraint;
- a bridge blocked by another hole;
- a bridge whose containment or crossing relation is uncertain beyond tolerance;
- many holes competing for endpoints;
- contour and endpoint permutations;
- no admissible bridge for a positive-area hole; and
- zero-measure hole residual handoff.

Verify holes remain excluded, bridge pairs are reciprocal, candidate order is deterministic, and no quality metric changes topology.

### 19.7 Orientation escalation tests

Construct corners where:

- immediate orientation is definite;
- a farther predecessor resolves the turn;
- a farther successor resolves the turn;
- paired farther neighbors resolve it;
- maximal-chain aggregate proves zero contribution;
- remaining-cell/contour area resolves orientation;
- source-facet evidence resolves an exact representational tie;
- symbolic exact equality is eligible for the frozen lineage rule; and
- no escalation distinguishes positive-area alternatives.

Assert exact escalation step sequence, evidence reuse, category, and failure payload.

### 19.8 Ear and progress tests

Test:

- deterministic ear sequence under ring rotation and reversed candidate insertion;
- stale candidate invalidation;
- candidate blocked by an interior occurrence;
- candidate blocked by a boundary/diagonal crossing;
- cleanup-margin ear;
- provable zero-area ear when no positive ear exists;
- symbolic-tie residual;
- a stalled active ring that must fail;
- adversarial few-valid-ear polygons; and
- bounded retries/progress counters.

Instrument accepted, rejected, deferred, stale, and regenerated candidates.

### 19.9 Boundary, topology, and coverage tests

Independently verify:

- exact boundary-halfedge multiset equality;
- unchanged endpoints/directions/cross-face pairs;
- internal diagonal two-use balance;
- triangle and residual-patch closure/interface validity;
- cell adjacency connectivity;
- Euler characteristic for zero, one, and many holes;
- bounded oriented-area agreement;
- no forbidden crossings or positive-area overlaps;
- no omitted positive-area cells; and
- complete residual assignment.

Use exact rational/integer-coordinate oracles for bounded low-complexity fixtures. Include cases where overlapping and missing triangles preserve total area to prove area alone cannot pass.

### 19.10 Tolerance-boundary and adversarial numerical tests

Create features with width, altitude, edge length, and area:

- clearly above tolerance;
- just above the machine/precision floor;
- just above, at, and just below cleanup-category thresholds;
- just below, at, and just above caller tolerance; and
- under large translation, power-of-two scale, adjacent-float, subnormal, and signed-zero variants.

Verify definite, cleanup-margin, zero/tie residual, and failure categories are separated without a universal epsilon.

### 19.11 Metamorphic and determinism tests

Apply:

- vertex, halfedge, cycle, contour, region, source-facet, shell, and component permutations;
- cycle rotation and alternative valid start halfedge;
- operand exchange with operation remapping;
- alternative valid source triangulations/facet subdivisions;
- axis permutation and orientation-corrected sign flip;
- exactly representable translation and power-of-two scaling with precision scaling;
- projection-candidate permutation;
- reversed bridge/ear insertion order;
- hash-collision injection;
- thread counts 1, 2, and maximum; and
- forced task delays.

For fixed V1 versions, support frames, selected bridges, ear sequence, internal diagonals, triangle IDs/categories, residual obligations, diagnostics, replay, and digest must be byte-identical after documented remapping.

### 19.12 Mutation tests

Implement every mutation listed in Section 16.4 plus resource, owner, version, range, reserved-field, precision-reference, and replay mutations. Correct counts and recompute superficial digests in mutation builders so the independent verifier must examine semantics. Required mutation survival count is zero.

### 19.13 Fuzzing and shrinking

Generate valid Component 11 polygonal complexes with controlled:

- region/corner count;
- reflex count;
- hole count;
- contour nesting represented through separate regions;
- repeated-coordinate count;
- zero-length edge count;
- collinear-chain length;
- corridor width;
- event/carrier/source provenance;
- coordinate exponent and ULP perturbation;
- triangle/residual-patch category boundary; and
- resource limits/cancellation checkpoints.

For small cases compare against exhaustive legal bridge/diagonal/cell oracles. Serialize and deterministically shrink every crash, hang, boundary loss, wrong pair, overlap, missing pocket, nondeterminism, verifier disagreement, or unexpected typed failure while preserving the failure and complete predecessor artifacts.

### 19.14 Resource, cancellation, and concurrency tests

For projected records, traversal nodes, bridge candidates, predicate evidence, active candidates, spatial nodes, diagonals, halfedges, triangles, residuals, assignments, verification work, temporary bytes, and persistent bytes, test limit-minus-one, limit, and limit-plus-one.

Cancel during every stable checkpoint and inside long bridge scans, escalation walks, ear batches, diagonal allocation, overlap enumeration, coverage reduction, codec, and verifier. Confirm workers join, reservations return, no pending pair/cell remains visible, and no partial artifact publishes.

### 19.15 Structural performance gates

Record and assert:

- projected occurrence count;
- bridge candidate and visibility-test counts;
- predicate cache hit/miss counts by kind;
- maximum escalation depth;
- active candidate accepted/rejected/deferred/stale counts;
- blocker-index nodes and candidate pairs;
- internal diagonal/triangle/residual-patch counts;
- overlap candidate and exact relation counts;
- verifier candidate counts;
- peak temporary/persistent bytes; and
- abstract work units.

Large ordinary simple polygons with no holes and well-separated vertices must not perform exhaustive all-diagonal enumeration or all-triangle overlap tests. Their blocker/overlap candidate counts must track actual bound overlap within versioned structural thresholds. Adversarial repeated-coordinate or many-hole cases may consume superlinear configured work and return `resource_limit`, but must never skip verification.

### 19.16 Sanitizer and platform matrix

Run supported `float`/`double` and internal/public index-width combinations under:

- GCC and Clang strict C++17 targets;
- debug and optimized strict floating builds;
- ASan/UBSan;
- TSan for deterministic region concurrency;
- supported 32-bit/64-bit index capacity boundaries; and
- supported architectures/floating-environment qualification from Components 01/03.

No valid fixture may expect `internal_invariant_error`.

## 20. Implementation sequence and gates

Implement in this order. Do not integrate the next step until the stated gate passes.

1. **Schemas, versions, API, and compile target.** Add closed enums, IDs, artifact skeleton, capability validation, strict-target integration, empty-artifact path, codec placeholders, and contract tests. Gate: invalid versions/owners/ranges fail deterministically and empty artifact verifies/round-trips.
2. **Predecessor validation and preflight.** Implement all Component 11/10/04/05/08/03 audits, checked formulas, resource plan, and failure payloads. Gate: mutation fixtures for every predecessor contradiction fail before projection.
3. **Projection.** Implement authoritative/fallback frame selection, compute-once projected occurrences, conditioning, and plane-residual audits. Gate: exact-oracle/metamorphic projection suite passes across scalar types and extreme scales.
4. **Region model and contour audit.** Implement immutable constraints, traversal nodes, contour roles, reverse maps, and active-cell schema. Gate: repeated-coordinate and zero-length fixtures preserve exact topology without triangulation.
5. **Predicate registry and escalation.** Implement canonical query normalization, Component 03 adapters, result cache, exact-tie distinction, and frozen escalation. Gate: known-answer and mutation suites prove no duplicate inconsistent evaluation and all escalation paths.
6. **Hole integration.** Implement complete bridge enumeration, visibility certificates, pair allocation, splice, progress, and rollback. Gate: annulus/multi-hole/exact-tie/resource/cancellation tests and independent hole-domain checks pass.
7. **Ear decomposition.** Implement blocker index, generation-safe candidate queue, admissibility, positive-before-cleanup ordering, strict progress, and final-cell handling. Gate: convex/concave/Boolean corpus, stalled-ring, and structural performance tests pass.
8. **Diagonal and triangle materialization.** Implement atomic pairs, pending active-cell ownership, boundary assignments, triangle categories, provenance, and local invariants. Gate: pair/boundary/orientation mutation survival is zero.
9. **Residual handoff.** Implement every supported residual class, obligations, compatibility checks, conceptual orientation, and zero-budget certificate. Gate: Component 13-facing contract fixtures require no contour reconstruction or guessed merge eligibility.
10. **Producer coverage.** Implement noncrossing/overlap index, boundary, pair, connectivity, Euler, area, orientation, and missing-pocket certificates. Gate: exact-oracle fixtures and area-cancelling overlap/missing mutations are rejected.
11. **Canonical merge, codec, and replay.** Implement sorted global merge, dense remap, reverse maps, canonical bytes/digests, diagnostics, and replay. Gate: byte-identical outputs/failures under all permutations and thread schedules.
12. **Independent verifier.** Implement independent reconstruction, alternative pair enumeration, exhaustive threshold path, codec verification, and full mutation inventory. Gate: zero required mutation survivors and producer/verifier agreement across corpus/fuzz.
13. **Resource, cancellation, fuzz, and performance qualification.** Complete limit boundaries, cancellation injection, deterministic shrinking, sanitizer/platform matrix, and structural gates. Gate: all Component 12 definition-of-done items pass before Component 13 integration.

Keep implementation commits small enough that each gate can be reviewed and bisected, but do not expose an unverified intermediate artifact as a supported downstream contract.

## 21. Definition of done

The Component 12 implementation plan is fulfilled only when all of the following are true:

- every accepted Component 11 positive-area region is completely and deterministically triangulated, with only explicitly supported bounded residual patches/obligations remaining;
- every Component 11 output occurrence and authoritative coordinate is unchanged;
- every Component 11 boundary halfedge is preserved and assigned exactly once on its incident region side;
- every internal diagonal is born as a reciprocal pair and has exactly two opposite final triangle uses;
- every ordinary triangle has three distinct occurrence IDs, a closed three-edge cycle, prescribed orientation, category, provenance, and evidence;
- holes and Component 11 contour topology are preserved exactly;
- repeated coordinates, zero-length topological edges, collinear chains, and weak equal-coordinate touches are handled without coordinate welding or deletion;
- uncertain orientation follows the frozen escalation sequence and positive-area ambiguity fails closed;
- every residual degeneracy is explicit, bounded, budget-free, and directly consumable by Component 13;
- no coordinate movement, cleanup action, feature removal, Boolean reclassification, occurrence repartition, or cross-face pair change occurs;
- boundary, pair, connectivity, Euler, bounded area, orientation, noncrossing, overlap, and missing-pocket checks all pass;
- the independent verifier reconstructs and accepts every success and rejects every required mutation;
- all providers terminate or return a typed deterministic failure under bounded work;
- cancellation and resource failure leave no partial artifact, pending diagonal, leaked lease, or worker;
- serial and parallel runs produce byte-identical artifact/replay bytes or the same deterministic failure;
- exact-oracle, Boolean corpus, property, metamorphic, adversarial, fuzz/shrink, mutation, resource, cancellation, sanitizer, platform, and structural performance suites pass;
- production and normative-test code is strict portable C++17 with no external dependency; and
- Component 13 can consume `triangulated_output_complex<T,I>` without reconstructing polygon contours, inferring adjacency from coordinates, or guessing cleanup obligations.

Only after this plan fully represents every Component 12 requirement should `tracker.md` mark component 12 complete.
