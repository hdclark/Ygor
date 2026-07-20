# Plan 13: Budgeted Cleanup and Topological Simplification

## 0. Scope and fixed V1 design

Implement **only Component 13** from `component_13_budgeted_cleanup_topological_simplification.md`. Consume the verified immutable artifacts from Components 01-12 and publish one immutable `cleaned_triangle_manifold<T>` for Component 14. Do not repeat Boolean classification, choose intersection ownership, reconstruct Component 11 face cycles as a substitute for predecessor data, change the operation truth table, assemble `fv_surface_mesh<T,I>`, merge public polygons, perform final public verification, or publish an ordinary Boolean success.

Freeze V1 as the following provider set:

```text
mutable_complex_provider:         append_only_generation_checked_cleanup_complex_v1
obligation_provider:              explicit_member_dependency_graph_v1
candidate_provider:               obligation_seeded_complete_local_actions_v1
scheduler_provider:               serial_global_minimum_phased_scheduler_v1
patch_provider:                   closed_star_interface_patch_v1
link_provider:                    reconstructed_closed_surface_link_v1
split_provider:                   exact_oriented_fan_sector_partition_v1
coordinate_provider:              existing_coordinate_first_bounded_segment_v1
retriangulation_provider:         interface_preserving_bounded_local_cell_v1
intersection_provider:            append_only_rank_morton_triangle_forest_v1
feature_cost_provider:            certified_piecewise_affine_patch_deviation_v1
budget_provider:                  component03_outward_sum_per_lineage_v1
topology_effect_provider:         rebuilt_component_link_euler_semantics_v1
certificate_provider:             replayable_closed_patch_delta_v1
canonicalization_provider:        complete_lineage_action_key_remap_v1
verification_provider:            independent_replay_rebuild_and_all_pairs_v1
```

The executable serial implementation is the semantic reference. Candidate discovery and proposal evaluation may be parallelized over immutable snapshots, but V1 commits exactly one globally smallest currently eligible action at a time. Worker count, task delay, allocation order, hash collisions, and speculative completion order must not change the accepted action sequence, replacement coordinates, topology, budget records, failure, diagnostics, canonical bytes, or digest.

V1 implements the conservative cleanup policy required by the component:

- exact topology and immutable lineage establish action eligibility before geometry or tolerance is considered;
- cleanup is obligation-driven, not an opportunistic mesh-quality pass;
- a valid thin or poor-aspect-ratio feature remains when it satisfies the final contract;
- zero-motion and topology-preserving actions are preferred over moving or topology-changing actions;
- vertex splitting or occurrence duplication is permitted only to recover the required one-closed-fan occurrence structure or to enable an otherwise valid composite cleanup action;
- local component merge, component split, handle removal/creation, cavity removal/creation, and all other genus-changing edits are unsupported and rejected in V1;
- an entire closed connected component may be removed only under the explicitly enabled Component 01/03 component-removal policy and the certificate in this plan;
- no action may create adjacency from coordinate proximity, exact coordinate equality, a scalar epsilon, or a spatial-index result;
- every committed action is a nested private transaction and leaves a valid paired cleanup cell complex;
- no unresolved residual cell, cleanup-required triangle, zero-length prohibited edge, zero-area final triangle, or cleanup obligation remains on success;
- every changed bounded coordinate, residual, orientation result, spatial bound, precision entry, and tolerance-budget record is recomputed through Component 03;
- every proposed moved or retriangulated patch is checked against all potentially interacting alive nonadjacent geometry, including the required swept envelope;
- all production and normative-test code is strict portable C++17 and uses only Ygor and the C++17 standard library; and
- do not call, adapt, copy, or derive implementation from `src/YgorMeshesBoolean{,2,3,4,5}*.{h,cc}`.

A valid empty Component 12 artifact publishes an audited empty `cleaned_triangle_manifold<T>` with zero actions and zero cleanup cost. If all output is removed through explicitly authorized whole-component cleanup, publish an empty cleaned artifact whose action log and topology report distinguish that result from an originally empty artifact.

Mark Component 13 complete in `tracker.md` only after every instruction and qualification gate in Section 25 is represented and the complete plan has been committed.

## 1. Existing Ygor assessment and mandatory reuse decisions

### 1.1 `mesh_remesher<T,I>` is not a Component 13 provider

`src/YgorMeshesRemeshing.h/.cc` implements a useful general-purpose iterative remesher over mutable public `fv_surface_mesh<T,I>`. It splits long edges, collapses short edges, flips for valence, and performs tangential smoothing. Preserve that API and its existing users, but do not call it from Component 13.

It is incompatible with the bounded Boolean cleanup contract because it:

- mutates public vertex and face arrays in place rather than a private paired internal complex;
- discovers edges from face-index pairs and does not preserve Component 11/12 halfedge, occurrence, carrier, retained-use, event, or source lineage;
- chooses collapse eligibility from nominal edge length and a local normal test rather than reconstructing the exact closed-surface link condition;
- uses midpoint averaging, target-length ratios, raw `vec3` distance/cross/dot/normalization, and fixed constants such as `1e-10`;
- prefers boundary vertices and lower indices without canonical Boolean lineage;
- can delete the two shared faces and globally replace an index without proving that duplicate edges, three-use edges, bow-ties, point-touch connections, or coincident-sheet welds are absent;
- flips edges for valence and smooths geometry for quality rather than to discharge a named Boolean cleanup obligation;
- has no bounded coordinate enclosure, inherited precision, per-lineage cumulative displacement, feature-removal metric, Component 03 reservation, or output-precision update;
- has no conservative swept-envelope or nonadjacent-triangle intersection test;
- throws for expected conditions and has no stage transaction, nested rollback, cancellation, resource accounting, deterministic failure arbitration, action certificate, codec, replay, or independent verifier; and
- operates on a representation that Component 14, not Component 13, is responsible for materializing.

Its fixture shapes, ordinary nondegenerate performance measurements, and some non-authoritative local mutation ideas may be reused in tests. Do not extract its collapse, flip, face-rewrite, normal, length, or smoothing routines. A future shared helper may be extracted only if it is purely combinatorial, accepts strong owner-bound IDs, has no coordinate or public-mesh dependency, is independently tested, and does not weaken either caller's contract.

### 1.2 Public `fv_surface_mesh<T,I>` cleanup methods are prohibited

`fv_surface_mesh<T,I>` exposes `merge_duplicate_vertices`, `remove_degenerate_faces`, `remove_disconnected_vertices`, `simplify_inner_triangles`, `convert_to_triangles`, and an optional `involved_faces` index. These are convenient public mesh utilities, not bounded Boolean cleanup services.

Do not use them in Component 13 because they:

- operate after loss of paired-edge and vertex-occurrence identity;
- may use coordinate-distance thresholds or public index equality;
- cannot preserve topology-distinct equal-coordinate occurrences;
- do not carry cleanup obligations, source/event/carrier lineage, retained-use multiplicity, or topology-separation constraints;
- do not reserve or commit Component 03 budget;
- do not publish exact link, patch-interface, orientation, intersection, topology-effect, or before/after certificates; and
- can silently remove or merge entities that the Boolean result must retain.

`fv_surface_mesh<T,I>` remains the Component 14 output representation. Do not construct a temporary public mesh for cleanup, verification, or candidate generation.

### 1.3 Loop subdivision, hole filling, orientation, and repair utilities are unsuitable

`YgorMeshesRefinement` performs in-place Loop subdivision and interpolation. `YgorMeshesHoles`, mesh orientation helpers, BSP output cleanup, tetrahedralization cleanup, and public hole filling or zippering operate on public topology, use geometric heuristics, or repair incomplete meshes after the fact. Component 13 must preserve and transform the exact paired internal complex supplied by Component 12; it must fail on a predecessor contradiction rather than infer a boundary, fill an unpaired hole, reverse a patch, or rebuild adjacency from coordinates.

Keep these APIs source-compatible. They may supply non-authoritative fixture generation or comparison data only.

### 1.4 Existing spatial indexes are accelerators with incompatible contracts

Do not use public `rtree<T>`, `octree<T>`, `cells_index<T>`, KD-tree helpers, or raw `index_bbox<T>` as the authoritative cleanup interaction index. Their mutable insertion behavior, raw floating comparisons, uninflated bounds, heuristic splitting, `std::any`, exception paths, or iteration order do not establish conservative Component 03 evidence or deterministic replay.

Reuse the generic finite-AABB and rank-Morton primitives introduced by Components 03 and 06 only through owner-checked, semantics-free interfaces. Component 13 owns alive/dead generation filtering, adjacency exclusion, proposed/swept-patch query formation, and exact cleanup relation disposition.

### 1.5 Existing adaptive predicates are consumed only through Component 03

Use Component 03 bounded points, finite intervals, exact-nominal expansion signs, orientation, residual, distance, projection, segment/triangle relation, AABB, tolerance-budget, and precision-ledger services. Do not call bare `orient_sign`, `point_on_*`, `segments_intersect_*`, `vec3::distance`, `vec3::length`, `vec3::unit`, `plane<T>`, or an ad hoc epsilon directly.

Component 13 owns canonical query formation, topology-aware exclusions, action-policy disposition, and action certificates. It must not create a second interval package, exact arithmetic implementation, machine-floor rule, tolerance ledger, or coordinate-equality policy.

### 1.6 Mandatory predecessor and infrastructure reuse

Reuse, rather than duplicate:

- Component 12 triangle, residual-cell, cleanup-obligation, boundary-assignment, internal-diagonal, halfedge, geometric-category, support, precision, provenance, canonical-key, digest, and verifier records;
- Component 11 output vertex occurrences, paired boundary halfedges, face regions, cycles, contour roles, occurrence separation, and cross-face pair lineage;
- Component 10 retained-use orientation, multiplicity, selected-boundary, coincident ownership, and topology-separation records;
- Components 08, 05, and 04 event, carrier, source edge/facet/triangle, semantic support, and source-feature provenance;
- Component 03 bounded values, exact-nominal evidence, finite AABBs, distance and residual bounds, orientation and intersection primitives, precision ledger, tolerance-budget proposals/reservations/commits, and output-precision aggregation;
- Component 01 owner tokens, strong IDs, checked arithmetic, closed enums, stage/checkpoint/subcode registries, typed outcomes, resource leases, cancellation, deterministic failure arbitration, transactions, immutable publication, canonical bytes, SHA-256, diagnostics, and replay; and
- Component 17 deterministic task execution when available, while retaining the serial scheduler and verifier as normative semantic references.

Use `vec3<T>` only as a nominal payload carrier inside Component 03 bounded records or for non-authoritative test rendering. Coordinate bits may participate in a complete canonical record key only under the frozen Component 03 signed-zero policy; they never establish topology.

## 2. Exact files, API, versions, and checkpoints

### 2.1 Production files

Add under `src/YgorMeshesBooleanBounded/`:

- `CleanupTypes.h` — closed enums, strong IDs, complete keys, fixed V1 constants, action/result classifications, counters, and failure details.
- `CleanedTriangleManifold.h` — immutable artifact schema, checked section views, and narrow Component 14/15 query surfaces.
- `MeshCleanup.h/.cc` — typed stage entrypoint, checkpoint orchestration, serial semantic reference, canonical merge, and publication transaction.
- `CleanupPreflight.h/.cc` — predecessor audits, exact count/ID/byte/work bounds, capability compatibility, and resource plan.
- `CleanupMutableComplex.h/.cc` — append-only stage-private vertex, paired-edge, halfedge, triangle, and residual-cell slots; alive flags; generations; incidence; tombstones; snapshots; and rollback deltas.
- `CleanupTopology.h/.cc` — exact edge links, vertex links, fan sectors, connected components, patch boundaries, interface bijections, manifold reconstruction, and topology-effect primitives.
- `CleanupObligations.h/.cc` — Component 12 obligation import, member graph, absorption rules, dispositions, progress categories, and completion audit.
- `CleanupCandidates.h/.cc` — obligation-seeded discovery, complete local action enumeration, generation dependencies, preliminary eligibility records, and rejection evidence.
- `CleanupScheduler.h/.cc` — phased complete keys, stale-candidate handling, deterministic global-minimum selection, failed-candidate suppression, and progress enforcement.
- `CleanupPatch.h/.cc` — closed affected-patch collection, exterior interface extraction, immutable before snapshots, private replacement proposals, exact interface maps, and atomic splice.
- `CleanupSplit.h/.cc` — exact oriented fan-sector partitioning, occurrence duplication, zero-motion split certificates, and split-plus-action composites.
- `CleanupCollapse.h/.cc` — link-condition evaluation, quotient topology, duplicate-edge splice planning, representative proposals, and collapse patch construction.
- `CleanupRetriangulation.h/.cc` — edge swaps, zero-area fold replacement, collinear-chain replacement, boundary-only residual absorption, constrained local cell enumeration, and paired diagonal creation.
- `CleanupCoordinates.h/.cc` — deterministic existing-coordinate and bounded-segment representative enumeration, Component 03 coordinate proposals, support residuals, and lineage displacement maps.
- `CleanupIntersectionRelations.h/.cc` — canonical proposed-triangle, edge, swept-envelope, and external-triangle queries using Component 03 primitives.
- `CleanupIntersectionIndex.h/.cc` — append-only immutable rank-Morton triangle forest, alive-generation filtering, complete conservative queries, and structural verification hooks.
- `CleanupBudget.h/.cc` — patch correspondence, displacement and feature-removal bounds, Component 03 proposal/reservation/commit adapters, and rollback integration.
- `CleanupTopologyEffects.h/.cc` — before/after component reconstruction, Euler/genus calculation after manifold validation, semantic component-role evidence, and topology-change reports.
- `CleanupCertificates.h/.cc` — deterministic action, eligibility, patch, budget, orientation, intersection, obligation, topology-effect, and before/after certificate construction.
- `CleanupCanonicalization.h/.cc` — final alive-entity sorting, dense strong-ID remap, reverse maps, action-log remap, and complete-key validation.
- `CleanupCodec.h/.cc` — canonical artifact/action encoding, section digests, full digest, and replay payload.
- `CleanupVerifier.h/.cc` — independently implemented intake reconstruction, sequential action replay, budget/topology/intersection checks, final rebuild, codec verification, and mutation rejection.
- `CleanupQueries.h` — owner-checked immutable views required by Components 14-15, diagnostics, replay, and tests.

Extend existing bounded-subsystem infrastructure instead of creating parallel registries:

- `ContractVersions.h` for Component 13 artifact, mutable-slot, obligation, candidate, scheduler, patch, link, split, collapse, coordinate, retriangulation, intersection, feature-cost, topology-effect, certificate, canonicalization, codec, replay, and verifier versions;
- Component 01 stage, checkpoint, strong-ID-domain, resource-kind, error-subcode, diagnostic, and replay registries;
- Component 03 capability declarations only where a required bounded distance, swept bound, patch correspondence, budget class, or precision aggregation view is not already exported;
- the strict bounded Boolean target and explicit-instantiation lists; and
- the bounded test target and CTest registration.

Do not compile these production translation units in an ordinary target that permits fast-math, reassociation, finite-only assumptions, an unauthorized rounding mode, or contraction contrary to Component 03.

### 2.2 Test files

Add under `tests/mesh_boolean_bounded/`:

- `TestCleanupContracts.cc`;
- `TestCleanupMutableComplex.cc`;
- `TestCleanupLinksFans.cc`;
- `TestCleanupObligations.cc`;
- `TestCleanupCandidatesScheduler.cc`;
- `TestCleanupEdgeCollapse.cc`;
- `TestCleanupVertexSplit.cc`;
- `TestCleanupZeroArea.cc`;
- `TestCleanupZeroLength.cc`;
- `TestCleanupRetriangulation.cc`;
- `TestCleanupCollinearChains.cc`;
- `TestCleanupThinFeatures.cc`;
- `TestCleanupComponentRemoval.cc`;
- `TestCleanupCoordinatesBudget.cc`;
- `TestCleanupIntersections.cc`;
- `TestCleanupTopologyEffects.cc`;
- `TestCleanupCertificatesReplay.cc`;
- `TestCleanupCanonicalizationCodec.cc`;
- `TestCleanupVerifierMutation.cc`;
- `TestCleanupBooleanCorpus.cc`;
- `TestCleanupProperties.cc`;
- `TestCleanupAdversarial.cc`;
- `TestCleanupFuzzReplay.cc`;
- `TestCleanupResourcesCancellation.cc`;
- `TestCleanupDeterminismConcurrency.cc`;
- `TestCleanupStructuralPerformance.cc`;
- `CleanupFixtures.h/.cc`;
- `CleanupExactTopologyOracle.h/.cc`;
- `CleanupExactGeometryOracle.h/.cc`; and
- `GoldenCleanupV1.h`.

Keep arbitrary-precision integer/rational arithmetic, exhaustive legal-collapse/retriangulation enumeration, corrupt-artifact builders, random valid-complex generators, shrinkers, and golden regeneration tools test-only.

### 2.3 Typed entrypoint

Provide an internal entrypoint conceptually equivalent to:

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

`capabilities` freezes every Component 13 provider/schema version, supported predecessor versions, action classes, default-policy restrictions, patch-size and work limits, independent-verifier thresholds, scalar/index combinations, Component 03 budget capabilities, and Component 14 compatibility versions. It contains no caller callback, arbitrary cost function, external simplifier, allocator with semantic behavior, or runtime-selected unversioned heuristic.

Validate owners, operation, operand role, policies, scalar/index descriptors, predecessor versions and digests, strong-ID domains, precision snapshots, budget service owner, supported residual schemas, and capability compatibility before authoritative allocation.

The stage owns one transaction. Every action owns a nested private transaction. Only the final independently verified immutable artifact handle is published.

### 2.4 Stable checkpoints

Use these fixed checkpoints:

1. context, cleanup policy, and capability validation;
2. predecessor owner/version/digest verification;
3. Component 12 triangle/residual/obligation coverage audit;
4. exact count, ID, byte, and work preflight;
5. persistent and peak resource reservation;
6. mutable cleanup complex construction;
7. initial pair, cell-cycle, fan, occurrence, and precision audit;
8. cleanup-obligation graph import;
9. initial defect scan and obligation reconciliation;
10. initial append-only interaction-index construction;
11. candidate discovery and generation binding;
12. deterministic candidate ordering;
13. selected candidate stale-state revalidation;
14. closed affected-patch and exterior-interface collection;
15. exact topological eligibility and topology-effect preclassification;
16. coordinate/topology/retriangulation proposal construction;
17. displacement, feature-removal, precision, clearance, and swept-bound calculation;
18. Component 01 resource and Component 03 tolerance-budget reservation;
19. private replacement-patch construction;
20. replacement pair, cycle, fan, interface, orientation, and self-intersection verification;
21. proposed-patch external-interaction and side-plausibility verification;
22. action certificate and before/after digest construction;
23. atomic patch splice and actual budget/precision commit;
24. obligation transition and candidate invalidation/regeneration;
25. strict progress and termination audit;
26. final manifold, obligation, budget, precision, and topology-effect producer verification;
27. canonical alive-entity remap and reverse-map construction;
28. canonical codec, section digests, full digest, and replay construction;
29. independent sequential action replay and final artifact verification;
30. idempotence verification at required verification levels;
31. resource reconciliation and final cancellation poll; and
32. transaction commit.

Poll cancellation at every checkpoint and at deterministic work-count intervals inside large star/link walks, component traversals, candidate batches, local-cell enumeration, bounded predicate batches, interaction-index queries, external pair tests, canonical sorts, replay, codec, and verifier passes. Never poll from wall-clock time.

## 3. Strong IDs, closed enums, complete keys, and artifact layout

### 3.1 Strong ID domains

Define non-interchangeable strong IDs for at least:

- `cleanup_vertex_slot_id`;
- `cleanup_paired_edge_slot_id`;
- `cleanup_halfedge_slot_id`;
- `cleanup_triangle_slot_id`;
- `cleanup_residual_cell_slot_id`;
- `cleanup_generation_id`;
- `cleanup_obligation_id`;
- `cleanup_candidate_id`;
- `cleanup_patch_id`;
- `cleanup_interface_halfedge_id`;
- `cleanup_fan_sector_id`;
- `cleanup_coordinate_proposal_id`;
- `cleanup_patch_correspondence_id`;
- `cleanup_interaction_query_id`;
- `cleanup_action_id`;
- `cleanup_action_certificate_id`;
- `cleanup_topology_effect_id`;
- `cleanup_component_snapshot_id`;
- `cleanup_budget_evidence_id`;
- `cleanup_precision_update_id`;
- `cleanup_spatial_block_id`;
- `cleanup_spatial_node_id`;
- final `cleaned_vertex_occurrence_id`;
- final `cleaned_paired_edge_id`;
- final `cleaned_halfedge_id`;
- final `cleaned_triangle_id`; and
- verifier/replay evidence IDs where generic IDs would permit domain confusion.

Stage-private slot IDs are append-only and never reused. Removal sets an alive flag false and advances affected generations. Final cleaned IDs are assigned only after success from complete canonical keys. Neither slot IDs nor final IDs alias Component 11/12 IDs, public index `I`, `size_t`, pointers, hashes, or each other.

### 3.2 Closed enums

Use explicit fixed-width nonzero enumerators and reject unknown values for:

- mutable cell kind: triangle, boundary-only residual, invalid;
- obligation class: zero-area triangle, symbolic-tie triangle, prohibited zero-length edge, collinear-chain cell, pinched equal-coordinate patch, deferred zero-measure contour, sub-threshold sliver requiring cleanup, component-removal review, verifier-discovered defect, aggregate, invalid;
- obligation disposition: pending, proven final-valid, discharged by action, absorbed with member evidence, removed with component, rejected budget, rejected topology, rejected geometry, failed resource, failed cancellation, failed invariant, invalid;
- candidate action class: prove-valid, edge swap, local retriangulation, fan split, split-and-collapse, endpoint collapse, constructed-point collapse, collinear-chain replacement, zero-area fold replacement, boundary-residual absorption, whole-component removal, invalid;
- action motion class: zero coordinate change, existing-coordinate lineage movement, bounded constructed-coordinate movement, feature removal, component removal, invalid;
- topological eligibility: eligible, stale, link failure, interface failure, occurrence-separation failure, source-feature restriction, duplicate-edge failure, fan failure, component/genus restriction, unsupported action, invalid;
- coordinate proposal kind: retain canonical endpoint, retain other endpoint, reuse compatible source/event coordinate, bounded segment midpoint, invalid;
- patch topology kind: closed star, two-triangle disk, multi-triangle disk, fan-sector composite, closed component, residual cluster, invalid;
- patch correspondence kind: identity, exact retriangulation coverage, piecewise affine quotient, whole-component deletion, invalid;
- intersection relation: exact adjacency, permitted shared vertex, permitted reciprocal shared edge, topology-distinct coordinate contact, definite separation, definite forbidden intersection, permitted local overlap, uncertainty blocks action, invalid;
- topology effect: counts only, occurrence-separation repair, component removal, component merge, component split, handle removal, handle creation, cavity removal, cavity creation, unsupported, invalid;
- candidate disposition: accepted, stale, duplicate, superseded, ineligible topology, over budget, invalid orientation, external intersection, unresolved geometry, no progress, resource rejected, invalid;
- action result: committed, rejected and cached, rolled back, invalid;
- final cleanup status: verified clean, verified empty, failed outstanding obligations, invalid; and
- verifier disposition: accepted, rejected contract, rejected action replay, rejected topology, rejected geometry, rejected budget, rejected codec, rejected determinism, invalid.

Compiler enum names, RTTI, implementation-defined values, and free-form strings are not serialized as the authority.

### 3.3 Complete canonical keys

Define lexicographic complete keys. At minimum:

```text
obligation_key =
    (obligation class and severity rank,
     Component 12 obligation/residual/triangle complete key,
     sorted exact member entity lineage keys,
     occurrence-separation and source-feature restriction digest)

candidate_key =
    (scheduler phase,
     highest obligation severity discharged,
     motion class rank,
     action class rank,
     sorted obligation keys,
     affected patch interface key,
     primary feature lineage key,
     complete action-specific topology key,
     dependency slot generations,
     provider versions)

patch_key =
    (sorted owned cell lineage keys,
     canonical oriented exterior-interface cycles,
     sorted affected vertex/edge lineage keys,
     dependency generations)

coordinate_proposal_key =
    (action candidate key,
     proposal kind rank,
     canonical contributor coordinate key,
     ordered affected original lineage keys,
     Component 03 operation/formula key)

interaction_query_key =
    (action candidate key,
     proposed or swept entity role,
     canonical proposed triangle/edge key,
     canonical external alive triangle lineage key,
     generation snapshot,
     relation formula version)

action_key =
    (monotonic commit sequence,
     selected candidate complete key,
     before patch digest,
     replacement proposal complete key)

final cleaned entity key =
    (entity kind,
     surviving/creating action lineage,
     complete predecessor lineage multiset,
     coordinate bits where applicable,
     exact oriented final incidence key,
     occurrence multiplicity/separation key)
```

A complete key must include extra lineage whenever two legal topology-distinct records could otherwise compare equal. Hashes accelerate lookup only. Equal complete keys are an invariant failure unless a schema explicitly aggregates a sorted member set.

Numerical cost may affect scheduling only through a structured Component 03 result that definitely orders two disjoint bound intervals. If cost intervals overlap or tie, the key uses canonical lineage and provider rank; it never compares nominal floats as a hidden tie-break.

### 3.4 Scheduler phases and fixed action preference

Freeze V1 scheduler phases in this order:

1. predecessor/representation contradiction detection — fail, never repair;
2. obligations that can be independently proven final-valid with no mutation;
3. zero-motion edge swap or exact-coverage local retriangulation;
4. zero-motion fan split or occurrence duplication;
5. zero-motion split-plus-retriangulation or split-plus-collapse composite;
6. endpoint-retaining collapse using an already existing coordinate;
7. compatible source/event-coordinate collapse;
8. bounded segment-coordinate collapse;
9. collinear-chain or zero-area multi-cell composite replacement;
10. explicitly authorized whole-component removal.

Within one phase, rank first by the highest-severity obligation discharged, then by number of obligations discharged only when the exact member sets differ without changing semantics, then by complete candidate key. Do not prioritize nominal shortest edge, smallest area, best aspect ratio, largest valence improvement, smallest floating cost, or discovery order.

A phase does not globally forbid a later class when an earlier class is ineligible. It controls deterministic selection among currently eligible proposals. A candidate that only improves quality and discharges no obligation is not generated.

### 3.5 Immutable artifact section layout

`cleaned_triangle_manifold<T>` contains canonical contiguous sections in this order:

1. header, owner, operation, scalar descriptor, cleanup policy, provider/schema versions, supported Component 14 versions, and predecessor digests;
2. final cleaned vertex-occurrence records;
3. final paired-edge and reciprocal halfedge records;
4. final oriented triangle records;
5. reconstructed vertex-link cycles and connected-component records;
6. final bounded coordinates, spatial bounds, precision-ledger references, and output-precision candidate;
7. predecessor-to-final and final-to-predecessor provenance maps;
8. Component 12 obligation import and final disposition records;
9. canonical committed action log;
10. per-action eligibility, before-patch, proposal, correspondence, budget, orientation, intersection, topology-effect, and after-patch certificates;
11. per-original-lineage cumulative displacement records;
12. feature-removal and component-removal records;
13. topology-change and component semantic-role reports;
14. no-new-intersection and local side-plausibility evidence;
15. resource, structural, candidate, and progress statistics;
16. canonical section digests and full artifact digest;
17. deterministic diagnostics and replay metadata; and
18. independent verifier report and verifier digest.

Published records may reference immutable predecessor artifacts and immutable committed Component 03 ledger/budget records whose lifetime covers Components 14-15. No view may reference mutable slots, dead records without a published action mapping, candidate queues, task-local arrays, temporary link sets, reservation tokens, rollback logs, or stack storage.

### 3.6 Required final entity records

Each final cleaned vertex record contains at least:

- final ID/key;
- finite authoritative bounded coordinate and exact nominal `T` bits;
- current precision-ledger entry;
- all predecessor Component 11 occurrence, Component 12 use, source vertex, event, carrier, retained-use, and caller lineage;
- occurrence-separation, multiplicity, and coincident-sheet class;
- cumulative displacement per original contributing lineage;
- incident halfedge range and canonical closed-link cycle;
- creating or last-changing cleanup action;
- component membership; and
- record digest.

Each paired-edge record contains at least:

- final ID/key and two reciprocal halfedge IDs;
- exact reversed endpoint occurrence IDs;
- exactly two incident triangle IDs;
- surviving source/boundary/internal-diagonal roles and provenance;
- collapse, splice, retriangulation, or creation lineage;
- bounded length and precision evidence;
- no-prohibited-zero-length disposition; and
- record digest.

Each triangle record contains at least:

- final ID/key;
- three distinct vertex occurrence IDs in prescribed outward order;
- three oriented halfedge IDs forming one closed cycle;
- owning connected component;
- definite accepted bounded orientation and area/altitude evidence;
- finite conservative AABB;
- source, event, carrier, retained-use, face-region, cycle, Component 12 triangle, and cleanup lineage;
- creating or last-changing action;
- local side-plausibility and external-intersection evidence references; and
- record digest.

## 4. Input validation, preflight, and resource reservation

### 4.1 Cross-artifact validation

Before constructing mutable state, validate:

- all artifacts have the same live Component 01 owner, operation, operand-role map, contact/output/cleanup policies, scalar/index descriptors, and deterministic policy;
- every required provider/schema version is supported and nonzero;
- predecessor verification dispositions are successful;
- predecessor full and required section digests recompute or match trusted immutable handles;
- Component 12 triangle, edge-use, diagonal, residual-cell, obligation, assignment, reverse-map, and precision ranges are valid and canonical;
- every Component 11 boundary halfedge is assigned exactly once on each incident region side by Component 12;
- every Component 12 internal diagonal has two reciprocal halfedges and exactly two triangle uses;
- every triangle cycle and residual-cell boundary/interface closes by exact IDs;
- every output occurrence resolves to one finite Component 03 bounded point and immutable precision entry;
- every non-definite triangle and residual cell belongs to exactly one cleanup obligation;
- every cleanup obligation's members, prohibited merges, source-feature restrictions, conceptual orientation, and advertised action capabilities are complete;
- Component 10 orientation, multiplicity, occurrence separation, and coincident ownership agree with Components 11-12;
- source/event/carrier references resolve through Components 04/05/08;
- Component 12's zero-cleanup-budget certificate is valid; and
- no mutable caller mesh or stale transaction owner is referenced.

A contradiction in a committed predecessor artifact is `internal_invariant_error`. Do not repair a missing pair, missing obligation, wrong occurrence partition, reversed retained surface, stale precision reference, open residual interface, or forged digest through cleanup.

### 4.2 Cleanup policy validation

Before mutable import, validate the frozen cleanup policy as one internally consistent versioned object. It must explicitly define:

- enabled action classes and coordinate providers;
- finite nonnegative caller tolerance and all action-specific limits;
- the Component 03 cumulative displacement combination rule, which must be outward-rounded per-lineage sum for V1;
- whether short/zero-edge collapse, zero-area retriangulation, collinear-chain replacement, fan splitting, and whole-component removal are enabled;
- protected source/event/carrier/retained-use feature classes;
- required orientation, support residual, clearance, swept-envelope, and side-probe thresholds;
- maximum patch boundary, fan valence, local triangulation, edit, candidate, regeneration, component, interaction, work, and byte counts;
- deterministic action priority and provider versions;
- verification, certificate, diagnostic, and replay detail levels; and
- the exact topology effects authorized.

Reject zero/unknown versions, missing fields, non-finite or negative limits, thresholds below the qualified machine/precision floor, a movement rule incompatible with Component 03, or internally contradictory enablement before mutation. V1 rejects any policy that authorizes component merge/split, handle or cavity creation/removal, or another genus-changing local action. A policy may enable whole-component removal only with the Component 03 component-removal budget class and semantic-role evidence required by Section 17.

Tolerance and policy thresholds select named bounded dispositions only. They are never installed as a universal equality, weld, duplicate, link, or adjacency epsilon.

### 4.3 Initial cleanup complex model

The Component 12 artifact may contain both triangles and supported boundary-only residual cells. Initialize a private **paired cleanup cell complex**, not a triangle-only mesh:

- every Component 12 output occurrence becomes one live cleanup vertex slot;
- every Component 11 boundary pair and Component 12 internal diagonal becomes one live paired-edge slot with two halfedge slots;
- every Component 12 triangle becomes one live triangular cell with its exact edge uses;
- every boundary-only residual becomes one live residual-cell slot with its exact oriented boundary and interface;
- halfedges retain reciprocal pairs even when one or both incident cells are residual cells;
- exact vertex links are reconstructed over incident cell sectors;
- all lineage, role, orientation, obligation, and precision references are copied without coordinate lookup; and
- no coordinate, adjacency, pair, cell, or occurrence is normalized during import.

Intermediate private state may retain residual cells while obligations remain. Every committed action must preserve a valid paired oriented cleanup cell complex. Success requires zero residual cells and only valid triangles.

### 4.4 Checked upper bounds

Use checked Component 01 arithmetic to derive conservative upper bounds for:

- initial vertices, paired edges, halfedges, triangles, and residual cells;
- append-only slots created by each enabled action;
- maximum vertex duplicates from fan partitioning;
- maximum replacement edges, halfedges, and triangles for each local patch size;
- candidate records, dependency-generation vectors, and rejection cache entries;
- obligation nodes, member edges, transitions, and absorbed-member ranges;
- patch snapshots, interface cycles, correspondence cells, and rollback deltas;
- coordinate proposals, Component 03 operation records, budget reservations, commits, and precision entries;
- interaction-index primitive records, immutable blocks, nodes, query candidates, and exact relation evidence;
- action logs, full certificates, topology-effect snapshots, and provenance maps;
- canonical remap/reverse-map/codec/replay storage;
- producer and independent-verifier work and scratch; and
- configured edit, candidate-regeneration, patch, fan, component, intersection, byte, and abstract-work limits.

Strong internal ID capacity, public `I` capacity needed by Component 14, `std::size_t` capacity, and configured resource capacity are distinct checks. Component 13 must not assume later canonicalization or component removal will reduce counts enough to avoid overflow.

### 4.5 Resource plan

Reserve separately through Component 01 for:

- persistent final topology and provenance;
- persistent action, obligation, budget, topology-change, and verification evidence;
- peak mutable cleanup slots and incidence storage;
- peak candidate queue, dependency, and rejection-cache storage;
- peak patch snapshot, replacement, and rollback storage;
- peak coordinate and precision proposal storage;
- active Component 03 reservation records;
- interaction-index blocks, query results, and relation evidence;
- component/link/fan reconstruction scratch;
- canonical sorting/remap/codec/replay workspace;
- independent verifier and bounded exhaustive-oracle scratch; and
- abstract work units for every topology, geometry, sort, index, replay, and verification operation.

Reserve persistent maxima or checked growth budgets before authoritative construction. Nested actions obtain subleases before private allocation. Resource failure cannot truncate candidates, omit a patch member, skip an external interaction, cap a certificate, drop provenance, or publish current private state.

## 5. Append-only generation-checked mutable cleanup complex

### 5.1 Slot storage

Use contiguous append-only slot arrays. A slot contains:

- owner and strong slot ID;
- alive flag;
- monotonically increasing generation;
- complete creation and current lineage keys;
- exact incidence references;
- predecessor and cleanup provenance;
- current bounded geometry references where applicable; and
- last-changing action or initial-import marker.

Removed slots become tombstones and are never reused. New slots append. This prevents stale handles from accidentally naming a different entity and makes action replay independent of allocator reuse. Resource preflight and edit limits bound append-only growth.

### 5.2 Halfedge and cell invariants

For every committed private state:

- each live paired edge owns exactly two live reciprocal halfedges;
- reciprocal halfedges have exact reversed endpoint slot IDs;
- each live halfedge has exactly one live incident cell;
- a triangular cell owns exactly three directed halfedges in one closed cycle;
- a residual cell owns one or more exact oriented boundary cycles permitted by its schema;
- every live edge has exactly two incident cell-side uses;
- every live vertex occurrence has one closed oriented incident-cell fan, except that a candidate composite action may represent conceptual split sectors only inside its uncommitted proposal;
- no cell references a dead slot;
- no active candidate depends on a generation that changed; and
- all incidence updates occur through a verified patch splice.

Do not use a mutable public `involved_faces` index. Maintain exact owner-bound incidence CSR-like vectors or sorted adjacency tables whose mutations are part of the action delta and whose correctness is independently reconstructible.

### 5.3 Generation rules

Increment the generation of every live entity whose:

- incidence changes;
- coordinate or precision reference changes;
- role or obligation membership changes;
- component membership may change;
- candidate eligibility evidence changes; or
- external-interaction neighborhood changes.

A candidate stores the complete sorted `(slot ID, generation)` dependency set. A mismatch makes it stale; stale candidates are discarded and regenerated only if their originating obligation remains pending.

### 5.4 Action delta and rollback

A nested action delta records:

- alive-flag changes;
- appended slot range starts;
- prior generations;
- old incidence and role records;
- obligation graph changes;
- candidate invalidation/rejection-cache changes;
- interaction-index append positions;
- Component 01 sublease state;
- Component 03 reservation token and pending precision records;
- diagnostic/replay append positions; and
- progress counters.

No authoritative mutable state is altered until the replacement proposal and certificate have passed all precommit checks. The final splice applies one bounded delta. If any check or commit fails, restore exact pre-action state, discard appended slots, release reservations, and truncate private logs.

## 6. Obligation import, reconciliation, and completion

### 6.1 One authoritative obligation graph

Import every Component 12 obligation as one node with its exact member set and predecessor key. Add a producer-discovered node only when the initial or post-action verifier finds a final-contract defect not already covered. A discovered node must name the exact cells/edges/vertices, class, evidence, and source obligation relationship; it cannot hide a producer omission.

Represent aggregation with explicit directed membership edges:

- an obligation may absorb another only when the aggregate records the complete sorted transitive member set;
- no member entity may silently disappear when removed as a side effect;
- every old obligation has exactly one final disposition; and
- every replacement defect has a new obligation of strictly lower progress rank or causes action rejection.

### 6.2 Initial reconciliation scan

Independently scan the imported complex for:

- non-definite or zero/sub-threshold triangle orientation/area;
- repeated triangle corners;
- prohibited bounded zero-length edges;
- residual cells;
- disconnected or pinched vertex links;
- duplicate paired-edge endpoint patterns that threaten a legal action;
- collinear-chain descriptors;
- sliver/narrow-feature records marked as requiring final cleanup;
- incomplete obligation coverage; and
- component-removal review records.

A predecessor defect that violates Component 12's promised combinatorial contract is an invariant failure. A represented geometry defect becomes or joins an obligation.

### 6.3 Obligation completion

An obligation is complete only when an independently reconstructed after-state proves:

- every original member is removed, mapped, or retained as final-valid;
- no replacement entity inherits the same or higher defect;
- all affected boundary, pair, occurrence, multiplicity, source-feature, and provenance constraints remain represented;
- all displacement, feature-removal, precision, and topology-effect costs are committed and referenced;
- the local patch passes pair, cycle, link, orientation, self-intersection, external-intersection, and side-plausibility checks; and
- the action certificate names the exact disposition.

A `prove-valid` disposition is permitted only when a stronger Component 03 bounded check establishes the final threshold without topology or coordinate change. It publishes the stronger evidence and does not mutate topology.

## 7. Candidate discovery and deterministic scheduling

### 7.1 Obligation-seeded discovery

Generate candidates only from:

- one pending obligation;
- a canonical connected aggregate of overlapping obligations whose affected patches share live entities; or
- a verifier-discovered defect obligation.

For each seed, enumerate every V1 action class that could legally discharge it under the frozen policy and local topology. Candidate discovery may use spatial bounds to find possible external conflicts, but action identity and patch ownership are determined by exact topology and lineage first.

Do not generate arbitrary short-edge collapses, valence flips, smoothing moves, or component removals unrelated to an obligation or explicit policy review.

### 7.2 Preliminary candidate record

Before expensive proposal construction, record:

- candidate key and provider versions;
- seed and aggregate obligations;
- action and motion classes;
- exact primary feature IDs;
- complete dependency generations;
- minimum closed patch domain;
- known occurrence/source-feature/topology restrictions;
- preliminary coordinate/provider possibilities;
- required Component 01 resource classes;
- required Component 03 budget classes;
- expected progress reduction; and
- deterministic preliminary rejection reason, if any.

### 7.3 Complete local action enumeration

For a fixed seed and generation, enumerate all action variants required by V1:

- both directed endpoint-retention choices for a collapse, canonicalized by endpoint keys;
- every compatible existing source/event coordinate contributor;
- the one versioned bounded segment construction when enabled;
- every exact fan-sector partition consistent with oriented link connectivity;
- every eligible edge swap diagonal;
- every local retriangulation admitted by the patch-size/provider limits;
- every valid composite split-plus-collapse or split-plus-retriangulation partition;
- every whole-component removal candidate for an eligible component; and
- a prove-valid candidate when stronger evidence is available.

Enumeration may stop after finding the globally smallest accepted candidate only when all unexamined variants have keys strictly greater and cannot change failure arbitration or required diagnostics. Otherwise record complete deterministic rejection summaries.

### 7.4 Failed-candidate cache

Cache a rejected candidate by:

```text
(candidate semantic key,
 complete dependency generations,
 policy/provider versions,
 rejection evidence digest)
```

Do not regenerate it until at least one dependency generation, policy version, or evidence source changes. This prevents numerical retry loops and action oscillation.

### 7.5 Serial semantic scheduler

The serial reference repeatedly:

1. obtains the smallest current candidate key;
2. checks all stored dependency generations;
3. discards/regenerates stale candidates;
4. constructs and verifies the full proposal;
5. commits it if accepted;
6. records a deterministic rejection if not accepted;
7. regenerates only obligations and neighborhoods affected by the result; and
8. checks the global progress measure.

Parallel evaluation may prepare immutable proposal evidence for a canonical prefix of candidates. The commit decision is still the serial global-minimum decision. A later candidate cannot commit while an earlier key remains unresolved.

## 8. Closed affected patches and exact interfaces

### 8.1 Patch closure

Collect the smallest versioned closed patch sufficient to prove all action consequences, then expand it when any check requires more context. Include:

- every removed, merged, split, duplicated, or moved vertex;
- complete incident cells and halfedges of those vertices;
- all cells whose orientation, precision, spatial bound, or provenance changes;
- every paired edge that may be removed, duplicated, spliced, or gain a new endpoint;
- complete before links for all vertices whose incidence changes;
- complete residual-cell and obligation members touched by the action;
- the oriented exterior interface halfedges separating changed from unchanged cells;
- all occurrence-separation, multiplicity, coincident-sheet, source-feature, retained-use, and carrier constraints crossing the interface;
- neighboring cells needed for side-plausibility evidence; and
- proposed/swept spatial bounds needed to query external geometry.

Patch collection follows incidence and exact lineage, never coordinate radius.

### 8.2 Interface representation

Represent each exterior interface as one or more canonical oriented cycles of unchanged halfedges. Record:

- exact halfedge and endpoint IDs;
- unchanged reciprocal exterior incidence;
- owning components and support lineage;
- cycle orientation;
- attachment sector at every boundary vertex; and
- a canonical cycle rotation.

The replacement must provide a bijection to every unchanged interface use with the same endpoint occurrence or an explicitly certified old-to-new occurrence mapping. No interface halfedge may be omitted, duplicated, reversed, or replaced by a coordinate-equal edge.

### 8.3 Patch topology classes

V1 accepts:

- a two-triangle disk for an edge swap;
- a closed one- or two-star collapse patch;
- a single topological disk for local retriangulation;
- several disks connected only through a composite fan split with each disk separately certified;
- a complete closed connected component for removal; or
- a canonical residual cluster plus enough adjacent cells to produce one of the above.

A patch with an unresolvable non-disk interface, hidden handle, ambiguous repeated boundary occurrence, or required unsupported topology change is ineligible. Do not heuristically cut it.

## 9. Exact link condition, fan reconstruction, and vertex splitting

### 9.1 Independent link reconstruction

Build vertex and edge links from live halfedge/cell incidence for each candidate. Do not trust cached valence or neighbor counts.

For a live vertex occurrence, the link is the cyclic sequence of opposite directed edge sectors around that exact occurrence. For a paired edge, the link contains the two opposite vertices/cell sectors incident to that edge. Preserve distinct entries even when their coordinates or predecessor source IDs coincide.

### 9.2 Collapse link condition

An ordinary edge collapse is topologically eligible only when simulation of the exact quotient proves all of the following:

- the edge has exactly two opposite triangle uses and no residual-cell use unless the composite action explicitly absorbs that residual;
- the intersection of the two endpoint link member sets is exactly the edge link, after applying only an explicitly certified fan split;
- the two ordinary incident triangles are exactly the cells removed by the ordinary quotient;
- duplicate endpoint-neighbor edges created by the quotient are precisely the pairs expected to splice after those incident triangles disappear;
- no resulting edge has fewer or more than two uses;
- no resulting vertex link is disconnected, self-identified, repeated, or non-cyclic;
- no two previously topology-separated fans, point-touch components, edge-touch components, or coincident-sheet occurrences are joined;
- no prohibited source/result boundary or carrier role is erased;
- the affected component remains an oriented closed two-manifold; and
- the action's topology-effect class is permitted.

The implementation may optimize this test, but the certificate and independent verifier must reconstruct the full before/after links and quotient member sets. A short or zero-length edge alone is never eligibility evidence.

### 9.3 Fan split and occurrence duplication

A fan split is eligible only when exact oriented sector connectivity after the proposed removal/replacement partitions one coordinate occurrence into two or more required closed fan cycles.

For each new occurrence:

- assign one complete nonempty cyclic sector set;
- duplicate the authoritative bounded coordinate and precision reference without movement;
- copy all source/event/retained-use lineage and record copy multiplicity;
- preserve or refine occurrence-separation and coincident-sheet classes;
- reassign each incident halfedge and cell exactly once;
- prove every affected edge remains reciprocal and two-use;
- prove the split does not geometrically open a gap; and
- publish old-to-new and new-to-old sector maps.

A split may commit alone only when it directly discharges a represented pinched/non-manifold occurrence obligation and produces valid closed fans. Otherwise include it in one atomic split-plus-collapse or split-plus-retriangulation action; no non-manifold conceptual intermediate state is committed.

Do not split a valid connected fan for aspect ratio, valence, or scheduler convenience.

### 9.4 Topology-preserving split classification

A split that separates sectors which were required to be distinct final occurrences but were represented in one cleanup cell vertex is classified `occurrence_separation_repair`, not a geometric component split. The certificate must show the unchanged geometric support, exact sector partition, and predecessor separation requirement. Any split that changes actual connected-component or genus semantics is unsupported in V1.

## 10. Coordinate proposals and lineage displacement

### 10.1 Fixed proposal order

For a topologically eligible collapse or move, enumerate coordinate proposals in this exact order:

1. retain the endpoint with the lexicographically smaller complete endpoint lineage key;
2. retain the other endpoint;
3. reuse each compatible existing canonical source-vertex or shared event coordinate in complete contributor-key order;
4. use one Component 03 bounded segment-midpoint construction with fixed formula grouping, only when the cleanup policy permits coordinate construction.

An earlier proposal is not automatically accepted; every proposal must pass budgets, support residuals, orientation, clearance, and intersection checks. The candidate key records the proposal rank. Do not use centroid, least-squares, smoothing, quadric error, unconstrained average, normalized interpolation, random perturbation, or quality optimization.

### 10.2 Compatibility of an existing coordinate

An existing coordinate is compatible only when:

- it belongs to the union of contributing vertex lineages or a shared canonical source/event support explicitly referenced by all contributors;
- reusing it does not merge prohibited occurrence classes;
- every affected original lineage receives a conservative displacement bound to that coordinate;
- required source planes, carriers, selected boundary neighborhoods, and local support envelopes remain within policy;
- all incident replacement triangles retain definite orientation; and
- no external interaction or side-plausibility check is blocked.

Bit-identical coordinates remain separate proposals when their topological lineage differs; the resulting topology mapping, not coordinate bits, decides whether they are equivalent.

### 10.3 Bounded segment construction

The V1 constructed point is the Component 03 prescribed midpoint operation over the edge endpoints, with explicit rounding and enclosure. It is considered only after all existing-coordinate proposals fail and only for an exact edge-collapse action. Record:

- operation/formula ID;
- endpoint bounded points and precision parents;
- nominal bits and enclosure;
- support/carrier residuals;
- conditioning;
- inherited and construction uncertainty;
- per-lineage displacement;
- affected triangle orientation margins;
- available budget; and
- deterministic proposal key.

If midpoint conditioning, residual, displacement, or output precision exceeds tolerance, reject the proposal. Do not try arbitrary interpolation parameters in V1.

### 10.4 Per-lineage displacement

For every original geometric lineage contributing to a moved/merged vertex, compute:

- the current bounded location;
- the proposed bounded location;
- a conservative upper bound on actual displacement including both enclosures;
- already committed cumulative displacement;
- outward-rounded new cumulative sum;
- lineage-specific tolerance/policy limit;
- remaining margin; and
- the corresponding Component 03 proposal entry.

When several current vertices share an original lineage, charge the conservative maximum current-to-proposed displacement for that action and then add it to the lineage's prior cumulative total according to the Component 03 V1 rule. Never charge only the kept endpoint, only the largest action in the stage, or a nominal vector that cancels prior movement.

## 11. Edge collapse materialization

### 11.1 Quotient plan

After topology and coordinate eligibility, build a private quotient plan that explicitly lists:

- endpoint slots and the exact paired edge being collapsed;
- the two incident cells removed;
- all endpoint incident sectors;
- replacement vertex occurrence and lineage union;
- every old edge mapped to a surviving, spliced, or removed edge;
- every halfedge endpoint rewrite;
- every surviving triangle corner rewrite;
- every duplicate neighbor-edge pair that must be spliced;
- all removed and replacement obligations;
- exterior interface mapping; and
- affected component and topology-effect records.

Do not perform a global index substitution.

### 11.2 Duplicate-edge splice

For each expected pair of edges from the two endpoints to one edge-link vertex:

- remove the halfedge sides belonging to the two disappearing incident triangles;
- pair the two surviving opposite sides into one replacement paired edge;
- prove exact reversed endpoints after endpoint mapping;
- transfer both surviving incident triangles;
- preserve the union of edge provenance and distinguish source-feature restrictions;
- reject if more or fewer than the expected two survivor sides exist; and
- reject if another live edge with the replacement endpoints exists outside this exact quotient relation.

This splice is how ordinary triangle edge collapse preserves two-use edges. It is never a coordinate-based edge merge.

### 11.3 Triangle rewrite

For every surviving affected triangle:

- replace only the exact endpoint occurrence named by the quotient;
- preserve orientation-preserving corner order;
- require three distinct replacement occurrence IDs;
- rebuild its three halfedge uses and pairs;
- recompute bounded orientation, area/altitude, support residual, AABB, and precision;
- reject any definite inversion, unresolved final orientation, or new zero-area triangle; and
- update full provenance and action lineage.

### 11.4 Composite collapse

A split-plus-collapse action contains one conceptual fan partition followed by one or more quotient operations inside the same private proposal. The complete final patch must pass all checks. The certificate exposes each conceptual step, but no partial split or quotient is visible.

## 12. Zero-area triangles, zero-length edges, and residual cells

### 12.1 Zero-area triangle actions

For each zero-area or cleanup-required triangle, enumerate in fixed action order:

- prove final-valid under a stronger bounded threshold check;
- swap an eligible internal diagonal;
- retriangulate a closed local disk with unchanged coordinates and exact coverage;
- split a required fan then retriangulate;
- collapse each topologically eligible edge through all coordinate proposals;
- remove a certified paired zero-area fold through local disk replacement;
- replace a maximal collinear chain; or
- remove its entire component when component removal is enabled and certified.

Deleting the triangle record alone is prohibited. Every action accounts for all triangle edges, reciprocal pairs, adjacent cells, vertex sectors, obligations, and provenance.

### 12.2 Zero-length edge classification

Before generating a collapse, classify the exact paired edge as:

- ordinary collapsible edge;
- topology-separated point-touch connector;
- topology-separated edge-touch or coincident-sheet occurrence;
- edge requiring endpoint fan split;
- one of several coordinate-coincident edge occurrences;
- zero-length edge on a valid thin positive-area feature;
- protected source/carrier/result-boundary edge;
- edge incident to a residual cluster; or
- edge in a closed degenerate component.

A valid thin-feature edge may remain only when the final edge/triangle geometry is accepted and no obligation requires removal. An edge that cannot satisfy the final contract and has no permitted certified action causes typed cleanup failure.

### 12.3 Boundary-only residual absorption

For a Component 12 boundary-only residual cell:

1. collect the complete connected residual cluster through exact paired edges and shared obligated sectors;
2. include adjacent triangle cells necessary to obtain a closed patch;
3. reconstruct all exterior interface cycles;
4. determine whether the cluster is a local disk, several disks separable by an exact fan split, or an entire closed component;
5. enumerate zero-motion local replacements using existing occurrences;
6. if necessary, enumerate permitted split/collapse composites;
7. otherwise consider certified whole-component removal; and
8. fail if no supported action eliminates every residual cell.

A residual cell never survives publication. It may not terminate a final edge, be serialized as a public face, or disappear without an obligation disposition.

### 12.4 Paired zero-area fold

A two-cell fold may be removed only when:

- the two cells and shared edge set are exactly identified;
- their removal exposes two interface chains that have an exact orientation-reversing bijection;
- reconnecting the interface creates reciprocal two-use edges and closed vertex fans;
- no source-feature, retained-use, occurrence-separation, or coincident-sheet constraint forbids it;
- bounded geometry proves the fold is zero/sub-tolerance under policy;
- the replacement does not cross external geometry; and
- patch correspondence and budget evidence cover the removed fold.

Do not infer fold cancellation from opposite nominal normals alone.

## 13. Edge swaps, local retriangulation, and collinear chains

### 13.1 Edge swap

An edge swap is considered only for a two-triangle disk whose shared edge role is `internal_triangulation_diagonal` or another explicitly versioned swappable role. Require:

- four exact boundary halfedges forming one oriented disk interface;
- distinct replacement diagonal endpoint occurrences;
- no existing unrelated edge with the replacement endpoints;
- a reciprocal replacement diagonal allocated before either triangle;
- two replacement triangles with definite accepted outward orientation;
- exact interface preservation;
- no forbidden self or external intersection;
- no occurrence, source-feature, selected-boundary, component, or genus change;
- no coordinate movement or budget use except precision-operation records; and
- a strict reduction in the owning obligation's defect category.

Quality or valence improvement alone is not an action. Record the old diagonal as removed and the new pair as action-created.

### 13.2 Local disk retriangulation provider

V1 retriangulates only an exact topological disk with a canonical oriented exterior interface. It uses existing vertex occurrences unless combined with an explicitly certified collapse/split coordinate proposal.

Build a local constraint model from exact interface and retained interior feature restrictions. Reuse Component 03 bounded predicates and the Component 12 occurrence-preserving triangulation primitives through a narrow local-cell adapter where their contracts match. Do not call Component 12's stage producer, mutate its artifact, or reuse its producer decisions as verification.

The provider:

- enumerates candidate internal diagonals in complete endpoint-lineage order;
- rejects a diagonal on definite crossing, protected-feature violation, occurrence conflict, or unresolved positive-area geometry;
- creates every accepted diagonal as a reciprocal pair;
- enumerates legal triangulations by canonical backtracking for boundary size at or below the versioned exhaustive production limit;
- uses the versioned deterministic ear provider for larger disks, with complete blocker queries and no quality ranking;
- accepts only a triangulation whose triangles all have definite final orientation and whose exact boundary, connectivity, Euler, noncrossing, and bounded coverage certificates pass; and
- chooses the lexicographically smallest accepted replacement encoding.

Freeze the V1 exhaustive production limit at 10 distinct boundary occurrences. Changing it, the larger-disk provider, diagonal order, or selected-minimum encoding requires a provider version change. Work is always bounded by Component 01 limits.

### 13.3 Exact-coverage zero-motion retriangulation

A zero-motion retriangulation has zero displacement and zero feature-removal cost only when an independent bounded coverage certificate proves the old and new patch represent the same supported cell set:

- identical exterior interface;
- identical protected internal constraints that must survive;
- compatible source/support regions;
- no positive-area old/new overlap mismatch;
- equal occupied local cells under bounded orientation/containment evidence; and
- no residual region omitted or added.

Area equality alone is insufficient. If exact local coverage cannot be established, the action requires a certified piecewise-affine patch correspondence and budget, or it is rejected.

### 13.4 Collinear-chain replacement

Identify a maximal chain from exact edge/cell topology and Component 12 obligation membership. Simplification is eligible only when:

- removed intermediate occurrences have no incident sectors outside the closed patch;
- every protected source/carrier/result-boundary role either survives exactly or policy explicitly permits its removal;
- occurrence separation and coincident multiplicity remain valid;
- the replacement interface and triangulation have definite orientation;
- Component 03 bounded distance/turn evidence supports the chain classification;
- no new edge or triangle has a forbidden external interaction;
- a correspondence maps every removed local cell to replacement cells; and
- all displacement and feature-removal costs are reserved.

Do not use a global epsilon or nominal point-to-line distance. A chain action must strictly reduce its chain obligation and cannot recreate the same intermediate occurrence under another ID.

## 14. Patch correspondence and feature-removal cost

### 14.1 V1 cost classes

Keep separate:

- coordinate construction uncertainty;
- per-original-lineage displacement;
- topology-preserving exact-coverage retriangulation;
- local feature-removal/deformation;
- whole-component removal; and
- topology-change-specific authorization.

Do not charge uncertainty as movement and do not hide movement inside output precision.

### 14.2 Piecewise-affine correspondence

For a local action that changes patch geometry or topology but retains the component, publish a piecewise-affine correspondence:

- partition every old triangle into canonical subtriangles if needed;
- map each old subtriangle's three corners to exact points/occurrences on one replacement triangle or replacement edge/vertex;
- record reciprocal coverage from replacement cells to old support where policy requires two-sided deviation;
- use Component 03 bounded interpolation and distance operations;
- bound displacement over each affine subtriangle by the outward-rounded maximum of its mapped corner displacement bounds; and
- take canonical outward-rounded maxima over all cells.

Because the norm of an affine displacement field over a triangle is convex, the maximum is bounded by the maximum at its vertices; the verifier must reconstruct the exact mapping and bound. If a valid correspondence cannot be constructed, reject the local action rather than estimate from AABBs or sampled points.

### 14.3 Feature-removal bound

For collapse, chain removal, or fold removal, compute:

- maximum correspondence displacement over the old patch;
- maximum distance of every removed protected feature to its mapped replacement support;
- swept-envelope bound;
- local support/carrier residual;
- any policy-specific removed feature diameter; and
- cumulative per-lineage displacement separately.

The committed feature-removal cost is the maximum required metric under the frozen policy, while sequential lineage displacement remains an outward-rounded sum. A nominal edge length, triangle count, box diagonal without uncertainty, or sampled Hausdorff estimate is not sufficient.

### 14.4 Whole-component size bound

For component removal, choose the component's canonical smallest vertex-lineage key as representative. Compute a conservative upper bound on distance from every bounded component vertex to the representative and include every edge/triangle enclosure through convexity. Also compute the component finite AABB and its conservative diameter as an independent upper bound. Commit the larger bound required by the policy.

Do not use nominal volume, face count, or a non-conservative bounding box.

## 15. Budget reservation, commit, and precision recomputation

### 15.1 Reservation order

Before building mutable replacement slots, form the complete Component 03 proposal containing:

- action and replay identity;
- ordered original lineage IDs;
- worst-case displacement per lineage;
- feature-removal bound;
- optional whole-component removal request;
- patch correspondence and topology-authorization evidence;
- support/swept deviation;
- current cumulative costs;
- stage/action transaction owners; and
- canonical commit key.

Reserve Component 01 entity/byte/work resources and Component 03 tolerance budget before any authoritative mutation. Reservation failure rejects the candidate or terminates the stage with the precise typed failure; it never partially applies topology.

### 15.2 Actual commit

After the replacement patch passes every check, compute actual conservative costs from the accepted proposal. Require:

- finite nonnegative actual costs;
- actual costs no greater than reservation;
- every affected original lineage present exactly once;
- actual replacement bounded points inside recorded enclosures;
- feature/component removal no greater than authorization;
- successful topology splice;
- canonical action commit order; and
- matching precision entries.

Atomically commit the topology delta and budget records under one action transaction. If either commit cannot complete, roll back both.

### 15.3 Precision updates

For every changed or created entity, create new Component 03 records for:

- bounded vertex point;
- inherited source/event precision;
- cleanup construction uncertainty;
- cumulative displacement;
- edge length;
- triangle orientation and area/altitude;
- support and carrier residuals;
- finite AABB;
- swept bound where retained in evidence; and
- aggregate output-precision candidate.

Unchanged entities may retain immutable prior records. Changed entities may not retain stale bounds merely because nominal coordinates did not move.

Require:

```text
output_precision >= every retained inherited precision
output_precision >= every retained construction uncertainty
output_precision >= every cleanup construction uncertainty
output_precision >= every original lineage cumulative displacement
output_precision <= caller tolerance for ordinary success eligibility
```

No active Component 03 reservation may remain at action or stage commit.

### 15.4 Stage aggregate budget and precision report

Build the stage report from committed Component 03 records rather than mutable counters. Include at least:

- maximum realized displacement in any one accepted point proposal;
- maximum cumulative displacement over every original lineage;
- maximum local feature-removal bound;
- maximum whole-component removal bound;
- reserved and committed totals by budget class where the policy defines totals;
- action and lineage attaining each maximum;
- remaining tolerance margin for every limiting class and the global minimum margin;
- largest inherited precision, cleanup construction uncertainty, and support residual;
- final aggregate output precision;
- counts of zero-motion, moving, feature-removing, and component-removing actions; and
- proof that every reservation was committed or released.

The maximum individual action displacement must not substitute for cumulative lineage displacement. Component 14 receives this immutable report unchanged except for canonical public assembly.

## 16. Conservative no-new-intersection and clearance checks

### 16.1 Append-only triangle forest

Implement the interaction index as an append-only forest of immutable Component 06-style rank-Morton AABB blocks:

- index every initial Component 12 triangle by its conservative Component 03 AABB;
- append every replacement triangle generation exactly once;
- retain tombstoned primitives in index blocks but filter them by authoritative alive slot and generation after query;
- maintain immutable power-of-two primitive blocks;
- when two adjacent blocks have the same size, merge their canonical primitive records and rebuild one deterministic block;
- keep one sorted unindexed tail smaller than the next block threshold;
- query every block and the tail;
- sort returned primitive references by complete alive triangle key; and
- permit false positives but no false negatives.

Freeze leaf capacity, rank key, block merge rule, and traversal order by provider version. The index never establishes adjacency or intersection; it only returns non-definitely-separated candidates.

### 16.2 Proposed and swept query bounds

For every moved vertex, new edge, or replacement triangle, construct:

- proposed finite AABB;
- old-to-new swept AABB as the conservative hull of old and proposed bounded positions plus correspondence uncertainty;
- patch aggregate bound; and
- required policy clearance inflation.

Query all of them. A candidate may be excluded only by Component 03 definite separation or exact topology after query.

### 16.3 Exact topology exclusions

Classify a queried external triangle as adjacent only when exact final proposed topology proves:

- same triangle;
- reciprocal shared edge;
- permitted shared vertex occurrence; or
- another explicitly versioned local replacement relation.

Coordinate-equal but topology-distinct vertices, edges, sheets, and components are nonadjacent and must be tested.

### 16.4 Relation disposition

For every potentially interacting pair:

- evaluate canonical bounded edge-edge, vertex-triangle, triangle-triangle, and swept-envelope relations required by the case;
- reuse one evidence record per canonical query;
- distinguish exact adjacency, permitted local overlap, topology-distinct contact, definite separation, definite forbidden intersection, and unresolved uncertainty;
- reject a definite forbidden intersection;
- reject unresolved uncertainty when it can create an invalid embedding or exceed authorized tolerance; and
- record every tested external feature and disposition in the action certificate.

The proposed patch also receives exhaustive local self-pair tests after exact adjacency exclusions.

### 16.5 Clearance and side plausibility

Where cleanup policy requires clearance, prove the lower bound is at least the required margin after precision and movement inflation. Preserve local Boolean boundary plausibility by verifying:

- replacement orientation remains outward;
- replacement support lies inside the authorized old-patch neighborhood;
- no opposite-operand/source carrier is crossed outside its uncertainty;
- topology-separated contacts remain separate;
- no suppressed coincident sheet becomes exposed; and
- deterministic local side probes retain Component 10's selected occupancy where required.

Component 13 does not reclassify the Boolean. A contradiction with retained occupancy rejects the action.

## 17. Whole-component removal and topology effects

### 17.1 Component reconstruction

Reconstruct live connected components from reciprocal edge adjacency before any candidate that may alter component membership. Do not trust mutable labels alone. Record exact vertex, edge, cell, obligation, and provenance member sets.

### 17.2 Removal eligibility

A whole-component removal is eligible only when:

- policy explicitly enables component removal;
- the candidate owns the complete closed connected component;
- every cell is included and the before component is an oriented manifold cleanup cell complex;
- all obligations and provenance owned by the component are included;
- the component is topologically separate from all retained components even where coordinates touch;
- its conservative V1 size bound is within Component 03 component-removal budget;
- Component 10 retained-surface and shell semantics permit removal under the cleanup policy;
- no unmatched coincident ownership, carrier, or source-feature obligation remains;
- component/cavity semantic role is definite from orientation, retained-use lineage, and required side probes; and
- removal produces no residual edge, vertex, candidate, or reservation.

Removal is one all-or-nothing action. It tombstones every component cell, edge, halfedge, and vertex exclusively owned by the component and completes every owned obligation with explicit mappings.

### 17.3 Genus and component-count verification

After constructing every proposal, independently validate the replacement manifold before computing Euler/genus:

- reconstruct before and after connected components;
- compute exact `V-E+F` per closed oriented triangle component;
- require even `2-chi` and derive `g = (2-chi)/2`;
- match unaffected components through exact exterior anchors and lineage;
- classify component count and genus changes; and
- compare with action policy.

V1 permits count/slot changes with unchanged component/genus, occurrence-separation repair, and explicit whole-component removal. It rejects component merge/split and every handle/cavity/genus change. Euler delta alone is not eligibility evidence.

### 17.4 Cavity and occupied-volume interpretation

Topology alone does not identify whether a removed shell bounded an occupied island or cavity. Derive semantic role from Component 10 retained orientation, source shell lineage, and deterministic side evidence. If component-removal semantics are ambiguous, reject the action. Report the before/after component role and occupied-volume interpretation in the success wrapper input for Component 14.

## 18. Atomic action transaction and certificate

### 18.1 Required action sequence

Every candidate action executes these phases exactly:

1. revalidate candidate and dependency generations;
2. collect and canonicalize the closed patch and exterior interface;
3. reconstruct exact before links/components;
4. verify action-specific topology eligibility;
5. enumerate deterministic coordinate/topology proposals;
6. build patch correspondence and conservative costs;
7. reserve all Component 01 resources and Component 03 budget;
8. construct replacement slots and incidence privately;
9. reconstruct replacement pairs, cycles, links, components, and topology effect;
10. recompute all affected bounded geometry and precision;
11. verify orientation, coverage, self-intersection, external interaction, clearance, and side plausibility;
12. verify obligation transitions and strict progress;
13. construct the complete action certificate and before/after digests;
14. atomically splice the patch and commit actual budgets/precision;
15. update the interaction index;
16. update obligations and candidate generations; and
17. append the committed action record.

No intermediate non-manifold patch or active reservation is visible outside the nested transaction.

### 18.2 Certificate contents

Every committed action certificate contains at least:

- action ID/key, class, motion class, provider versions, and scheduler phase;
- candidate key, all dependency generations, and selection rank;
- seed/aggregate obligations and exact dispositions;
- complete before patch cells, vertices, pairs, halfedges, links, components, bounded coordinates, and digest;
- canonical exterior interface cycles;
- exact topological eligibility evidence;
- coordinate proposals considered and deterministic rejection reasons;
- selected coordinate/topology proposal;
- complete replacement slots and incidence;
- old-to-new and new-to-old entity/provenance mappings;
- fan partitions, quotient maps, edge splices, and retriangulation diagonals as applicable;
- patch correspondence and feature-removal proof;
- per-lineage displacement and cumulative totals;
- budget reservation and actual commit records;
- new precision entries;
- all affected orientation, residual, length, area, and AABB evidence;
- self/external intersection and clearance query records;
- local side-plausibility evidence;
- before/after component, Euler, genus, cavity/semantic role, and topology-effect records;
- resource counters and cancellation checkpoint;
- strict progress before/after tuple;
- deterministic reason codes; and
- after patch and cumulative state digests.

Certificates may reference shared immutable tables, but a fresh verifier must replay the action without calling producer candidate selection, link eligibility, mutator, budget accumulator, interaction-index traversal, or topology-effect control flow.

## 19. Progress, termination, and idempotence

### 19.1 Monotonic progress tuple

Freeze this lexicographic global progress tuple:

```text
(
  counts of pending obligations by descending severity class,
  boundary-only residual cell count,
  non-final triangle count,
  prohibited zero-length edge count,
  pinched/nonclosed occurrence count,
  unresolved collinear-chain count,
  pending component-removal review count,
  current nonreturning scheduler phase,
  remaining checked action-capacity
)
```

A `prove-valid` disposition reduces one pending obligation count. Every committed action must strictly reduce the tuple before the final capacity field, or advance a scheduler phase that can never decrease. The remaining action-capacity field is a guard, not permission for a non-progressing action.

A split that increases slot count must reduce a higher-priority pinch/obligation count in the same atomic action. An edge swap must reduce geometric category or obligation severity. A new obligation may be created only at a strictly lower severity and with complete member evidence.

### 19.2 Anti-oscillation

Prevent:

- swap-back of a diagonal under the same or equivalent obligation state;
- collapse/split cycles;
- repeated vertex movement without obligation reduction;
- regeneration of unchanged rejected candidates;
- retriangulation between equivalent encodings;
- component removal reconsideration after a definitive policy rejection; and
- schedule-dependent choice among equal proposals.

Store semantic before/after patch digests and rejected candidate keys. A proposed action whose after semantic digest matches an earlier state in the same obligation lineage is rejected as no progress.

### 19.3 Stalled cleanup

When pending obligations remain and no eligible candidate can reduce the progress tuple, stop. Choose the deterministic primary outstanding obligation/failure by Component 01 failure key and return `cleanup_budget_exceeded`, `result_geometry_not_validated`, `geometric_condition_exceeds_tolerance`, or another precise category. Never iterate until floating arithmetic changes by accident.

### 19.4 Idempotence

At required verification levels, rerun candidate discovery and final-validity scans over the successful immutable artifact through a read-only cleanup adapter. Require:

- zero pending obligations;
- no eligible action under the same policy;
- no coordinate or precision change;
- zero additional budget;
- no topology change;
- no action entry except optional no-op stage evidence; and
- identical canonical topology, geometry, report, and digest inputs.

Do not increase precision merely by revalidation.

## 20. Final canonicalization, codec, and replay

### 20.1 Canonical alive-entity remap

After no obligations remain:

1. collect all live triangle cells, paired edges, halfedges, and vertex occurrences;
2. reject any live residual cell or dead-reference incidence;
3. derive complete final entity keys from predecessor/action lineage, final incidence, coordinate bits, and separation classes;
4. sort each entity table by complete key;
5. reject duplicate keys unless explicit multiplicity rank distinguishes legal copies;
6. assign dense final strong IDs;
7. rewrite every incidence and provenance reference through checked maps;
8. reconstruct link cycles and components in final IDs;
9. build old slot/predecessor to final and final to predecessor maps; and
10. discard mutable slot/candidate/index storage before publication.

Because all action choices and creation lineages are deterministic, final remap does not use transient slot IDs. Component 14 will perform public graph canonicalization independently; Component 13 still guarantees deterministic internal bytes.

### 20.2 Final producer checks

Reconstruct and require:

- finite coordinates and valid precision references;
- three distinct corners and definite accepted outward orientation per triangle;
- reciprocal reversed halfedge pairs;
- exactly two uses per undirected edge;
- one closed triangle fan per vertex occurrence;
- no residual cells or pending obligations;
- no prohibited zero-length edge or zero-area triangle;
- complete predecessor/action mappings;
- cumulative budget and output precision within policy;
- only permitted topology effects;
- no known forbidden nonadjacent intersection;
- canonical sort order, dense IDs, reverse maps, and counts; and
- exact readiness for Component 14 without welding, cleanup, reorientation, or repair.

### 20.3 Canonical encoding

Use Component 01 `CanonicalBytes`. Encode fixed-width fields and Component 03 scalar bits explicitly. Include:

- all schema/provider/policy versions;
- owner, operation, scalar descriptor, and predecessor digests;
- complete final topology and bounded geometry;
- every obligation disposition;
- action sequence and certificates;
- budget, precision, feature-removal, and topology-effect records;
- provenance and reverse maps;
- structural/resource/progress statistics;
- producer and verifier reports; and
- replay metadata.

Do not serialize raw structs, padding, pointers, allocator state, locale text, exception messages, unordered iteration, or compiler type names.

### 20.4 Replay payload

Replay records enough information to reproduce:

- initial Component 12 import and defect reconciliation;
- every candidate generation and complete key;
- every candidate selected, rejected, or made stale;
- every patch/interface collection;
- exact link/fan/component reconstruction;
- coordinate proposal order and bounded evidence;
- resource and budget reservation/rollback/commit;
- replacement topology and correspondence;
- intersection candidate order and relation results;
- obligation transitions and progress;
- interaction-index block evolution;
- final canonical remap;
- codec and digest construction; and
- deterministic failure arbitration.

A replay under the same versions produces byte-identical artifact bytes or the same typed failure and primary key.

## 21. Independent verifier

### 21.1 Independence boundaries

`CleanupVerifier` must not call the producer's:

- candidate discovery or scheduler;
- patch collector;
- link-condition predicate;
- fan partitioner;
- collapse quotient builder;
- edge-splice mutator;
- retriangulation chooser;
- coordinate chooser;
- patch correspondence builder;
- budget accumulator;
- interaction-index traversal;
- topology-effect classifier;
- obligation completion helper; or
- final canonicalization control flow.

It may use shared immutable schemas, Component 01 checked accessors/canonical bytes/SHA-256, and Component 03 bounded arithmetic and budget-record primitives. Shared low-level primitives do not permit shared grouping or acceptance logic.

### 21.2 Sequential action replay

The verifier:

1. validates headers, owners, versions, ranges, enums, reserved fields, and predecessor digests;
2. independently reconstructs the initial cleanup cell complex from Component 12;
3. independently imports and audits every obligation;
4. for each committed action in order:
   - checks dependencies against current replay generations;
   - reconstructs the before patch from current state;
   - compares it with the certificate;
   - independently rebuilds links, fans, components, and exterior interface;
   - validates action-specific topology eligibility;
   - reconstructs selected coordinate and Component 03 evidence;
   - recomputes displacement, cumulative costs, and feature-removal bounds;
   - checks reservation and actual commit records;
   - constructs the certified replacement from declarative mappings;
   - validates pairs, cycles, links, orientation, correspondence, topology effect, and obligation transition;
   - enumerates external interactions through an independent sorted-AABB sweep or exhaustive path;
   - applies the replacement to verifier-owned state; and
   - recomputes after digest and progress;
5. reconstructs final topology from action-replayed state;
6. compares final canonical records and reverse maps;
7. exhaustively checks final pair, triangle, link, obligation, budget, precision, and topology reports;
8. independently enumerates forbidden final triangle intersections;
9. verifies codec bytes, section/full digests, replay, and resource summaries; and
10. emits a verifier report/digest.

A producer artifact with forged counts and recomputed superficial digests must still be rejected.

### 21.3 Independent final intersection search

The verifier does not trust producer candidate pairs. Use:

- exhaustive all-pairs below a frozen triangle threshold; and
- an independently implemented deterministic sorted-axis sweep above it, using Component 03 conservative AABBs and complete pair keys.

Exclude adjacency only after reconstructing exact final topology. Test every non-definitely-separated nonadjacent pair through independently grouped Component 03 relation queries. Unresolved forbidden geometry rejects the artifact.

### 21.4 Bounded exhaustive oracle

For configured small cleanup complexes, dispatch to test-only exact oracles that:

- use exact integer/rational coordinates for integer and power-of-two fixtures;
- enumerate all legal edge collapses satisfying exact link conditions;
- enumerate legal fan partitions and local retriangulations;
- compute exact component/Euler/genus changes;
- compare production action eligibility and final success/failure;
- verify budget bounds contain exact costs; and
- serialize disagreements for shrinking.

Production code never depends on arbitrary-precision arithmetic or exhaustive global enumeration.

### 21.5 Required mutation rejection

At minimum reject mutations that:

- omit or add one patch member;
- forge an interface bijection;
- collapse an edge failing the link condition;
- omit a required duplicate-edge splice;
- create a three-use or one-use edge;
- create a bow-tie/disconnected vertex link;
- merge point-touching, edge-touching, or coincident-sheet occurrences;
- alter a fan partition;
- change a replacement coordinate or formula;
- omit one original displacement lineage;
- reduce cumulative displacement;
- commit actual cost above reservation;
- change feature/component-removal bounds;
- retain stale precision after a coordinate/topology change;
- reverse or collapse a replacement triangle;
- omit an affected boundary triangle orientation check;
- hide a self or external intersection;
- omit swept-envelope evidence;
- drop or falsely complete an obligation;
- misclassify component/genus/cavity effect;
- reorder actions without matching generations;
- forge progress;
- retain a residual cell;
- scramble final IDs/reverse maps;
- alter provenance;
- forge resource counts, canonical bytes, digests, replay, or verifier report; or
- insert unknown versions/enums, nonzero reserved fields, stale owners, or out-of-range IDs.

Required mutation survival count is zero.

## 22. Typed failures, diagnostics, and deterministic arbitration

### 22.1 Component 13 subcodes

Allocate a disjoint Component 13 subcode range including at least:

- unsupported cleanup capability or policy version;
- predecessor owner/version/digest mismatch;
- invalid triangle/residual/obligation coverage;
- zero-budget certificate contradiction;
- mutable-complex import failure;
- pair, cell-cycle, or fan contradiction;
- ID/count/byte overflow;
- resource limit;
- stale candidate;
- candidate enumeration incomplete;
- patch closure or interface failure;
- edge link-condition failure;
- fan partition failure;
- occurrence-separation or coincident-sheet conflict;
- protected source/carrier feature conflict;
- duplicate/three-use edge after proposal;
- unsupported topology effect;
- coordinate proposal exhausted;
- coordinate construction ill-conditioned;
- displacement budget exceeded;
- feature-removal budget exceeded;
- component-removal budget or policy failure;
- support/carrier residual exceeded;
- replacement orientation invalid or unresolved;
- local coverage or correspondence failure;
- self-intersection;
- external forbidden intersection;
- external interaction uncertainty;
- side-plausibility contradiction;
- obligation transition failure;
- no monotonic progress;
- cleanup candidate set exhausted;
- active tolerance reservation at commit;
- precision update incomplete;
- final residual or obligation remains;
- final manifold reconstruction failure;
- canonicalization/reverse-map failure;
- codec/digest/replay failure;
- independent verifier disagreement;
- idempotence failure;
- cancellation; and
- internal invariant/progress failure.

Map expected geometry difficulty to `cleanup_budget_exceeded`, `result_geometry_not_validated`, or `geometric_condition_exceeds_tolerance`; invalid caller policy to `input_contract_error`/`invalid_tolerance`; representability to `index_overflow`; configured capacity to `resource_limit`; cancellation to `cancelled`; and predecessor contradiction or producer/verifier disagreement to `internal_invariant_error`.

The inability of V1 to find an eligible action for a faithfully represented degeneracy is not automatically an internal invariant failure.

### 22.2 Failure payload

Every failure includes, where applicable:

- stage, checkpoint, subcode, and stable primary failure key;
- context owner and all relevant policy/provider versions;
- predecessor artifact and section digests;
- obligation, candidate, patch, interface, entity, component, and lineage complete keys;
- dependency generations;
- bounded coordinates, orientation, residual, distance, and interaction evidence;
- coordinate proposals and rejection reasons;
- displacement, feature-removal, reservation, committed cost, and remaining budget;
- topology links/fans/components and effect evidence;
- resource counters and cancellation checkpoint;
- progress tuple; and
- bounded deterministic replay payload.

Do not use free-form exception text as the only diagnostic.

### 22.3 Failure arbitration

For parallel candidate evaluation, join all workers and choose the primary failure by Component 01's full deterministic key. Candidate rejection normally permits the next candidate; stage failure is selected from the canonical outstanding-obligation/fatal-failure domain after the scheduler proves no action can progress. Never choose first worker, first clock time, first exception, or lowest transient slot.

## 23. Transactionality, cancellation, and concurrency

### 23.1 Stage transaction

All mutable topology, action logs, budget records, precision updates, interaction indexes, final records, codec bytes, and verifier evidence remain private until the single stage commit. Nested committed actions are visible only inside the private stage state.

On any stage failure, discard the entire cleaned state and publish no partial action log or topology to Component 14.

### 23.2 Cancellation

Poll at fixed work intervals. On cancellation:

- stop issuing new candidate evaluations;
- bring active evaluations to deterministic safe points;
- join every worker;
- roll back the current nested action;
- release every Component 03 reservation and Component 01 sublease;
- discard all private mutable and final state;
- publish no cleaned artifact, action certificate, partial codec, or replay artifact; and
- return `cancelled` with stable checkpoint and replay context.

### 23.3 Permitted parallelism

V1 may parallelize:

- initial independent defect scans;
- immutable candidate proposal evaluation over a canonical prefix;
- Component 03 bounded query batches;
- interaction-index block builds;
- external pair relation evaluation with private outputs;
- independent component checks; and
- verifier pair checks.

The serial global-minimum scheduler and one-action-at-a-time commit remain authoritative. Provably independent batch commits are deferred to a future scheduler version; do not add them as an unversioned optimization.

No worker mutates shared topology, obligation state, budget ledger, precision ledger, candidate queue, diagnostic stream, or ID allocator without deterministic ownership and canonical merge.

## 24. Test and qualification plan

### 24.1 Contract and intake tests

Cover:

- empty Component 12 artifact;
- one clean tetrahedral shell requiring no actions;
- clean multiple components;
- every supported Component 12 triangle/residual category;
- mixed triangle and boundary-only residual cells;
- invalid owners, versions, scalar/index descriptors, policies, ranges, pairs, cycles, assignments, obligations, precision references, and digests;
- hidden Component 12 coordinate movement or budget use; and
- unsupported genus-changing policy.

Verify contradictions fail before mutation and clean inputs produce zero-action artifacts.

### 24.2 Existing-provider regression tests

Create fixtures demonstrating why public remeshing/cleanup APIs are not called:

- a short edge whose public remesher collapse creates a three-use edge;
- point-touching components with equal coordinates;
- coincident sheets with duplicate coordinates;
- a midpoint collapse exceeding cumulative lineage tolerance;
- a zero-area triangle whose record deletion leaves a hole;
- a valence-improving flip that does not discharge an obligation;
- a simplification that removes protected source/carrier lineage; and
- a move that creates a remote intersection.

These qualify the reuse decision; do not require changing legacy API behavior.

### 24.3 Mutable complex, links, and fans

Test exact reconstruction for:

- ordinary closed vertex fans;
- high-valence event vertices;
- duplicate-coordinate distinct occurrences;
- point- and edge-touching separate components;
- coincident sheets;
- residual-cell sectors;
- malformed one-way pair;
- mismatched endpoints;
- one-/three-use edge;
- missing or repeated fan sector;
- disconnected link with correct superficial valence; and
- stale generation handles.

Compare cached incidence against independent reconstruction after every debug action.

### 24.4 Edge-collapse topology tests

Include:

- ordinary legal collapse;
- failure of link-set intersection;
- extra common neighbor creating a three-use edge;
- bow-tie result;
- point-touch component merge;
- edge-touch component merge;
- coincident-sheet merge;
- protected source-feature removal;
- legal endpoint-retaining collapse;
- legal source/event-coordinate collapse;
- legal bounded midpoint collapse;
- collapse requiring duplicate-edge splices;
- legal split-plus-collapse;
- high-valence collapse; and
- quotient with repeated coordinate but distinct topology.

Verify exact eligibility, replacement incidence, links, mappings, and rejection subcodes.

### 24.5 Vertex split tests

Include:

- bow-tie sector partition into two closed fans;
- point-touch output separation without coordinate movement;
- preservation of coincident sheets;
- split before zero-length collapse;
- high-valence event-sector partition;
- invalid split of a valid connected fan;
- ambiguous partition rejection; and
- cyclic traversal and slot-order permutations.

Verify zero displacement, complete lineage multiplicity, exact sector coverage, and canonical keys.

### 24.6 Zero-area and residual tests

Include:

- isolated cleanup-required triangle adjacent to valid triangles;
- paired zero-area fold;
- collinear fan;
- distinct topological corners at one coordinate;
- degenerate carrier endpoint patch;
- residual cell absorbed into an adjacent disk;
- residual cluster requiring split;
- residual cluster that is an entire removable component;
- zero-motion retriangulation;
- movement just within and just beyond tolerance; and
- unsupported non-disk residual interface.

Deleting a triangle or residual alone must be caught by mutation verification.

### 24.7 Zero-length and thin-feature tests

Include:

- ordinary zero-length collapsible edge;
- separate point-touching fans;
- one of several coincident edge occurrences;
- protected source/carrier edge;
- endpoint split requirement;
- valid short edge that remains;
- valid thin corridor;
- sub-tolerance spike requiring cleanup;
- thin handle whose removal changes genus;
- tiny cavity; and
- thresholds immediately below, at, and above policy limits.

Verify topology-first behavior and default rejection of genus change.

### 24.8 Retriangulation and collinear-chain tests

Include:

- legal edge swap with same interface;
- swap introducing duplicate edge;
- swap-back oscillation attempt;
- concave local disk;
- disk with repeated-coordinate occurrences;
- exact-coverage alternative triangulation;
- area-equal but overlap/missing-pocket replacement;
- collinear interior chain;
- protected boundary chain;
- chain spanning incompatible supports;
- larger-than-exhaustive-limit deterministic ear path; and
- stalled local cell.

Verify pair-at-creation, boundary conservation, coverage, orientation, progress, and no quality-only actions.

### 24.9 Coordinate, budget, and precision tests

For every proposal provider, test:

- both endpoint choices;
- compatible and incompatible source/event coordinates;
- bounded midpoint;
- equal nominal costs with lineage tie;
- overlapping cost intervals;
- sequential movement through several actions;
- several current vertices sharing one original lineage;
- large translation and mixed exponents;
- subnormals and signed zero;
- construction conditioning near failure;
- reservation rollback after later topology/intersection failure;
- actual cost equal to, below, and illegally above reservation;
- stale precision mutation; and
- repeated Boolean input precision already near tolerance.

Test limit minus one representable unit, exact limit, and limit plus one for displacement, feature removal, component removal, and aggregate output precision.

### 24.10 Patch correspondence tests

Cover:

- identity correspondence;
- exact-coverage zero-motion retriangulation;
- affine edge collapse mapping;
- chain removal mapping;
- fold removal mapping;
- correspondence with required triangle subdivision;
- missing old cell;
- duplicate new coverage;
- incorrect corner mapping;
- an interior displacement larger than claimed due to a corrupted non-affine map; and
- component-removal size bound.

Compare bounded costs against exact rational fixtures.

### 24.11 No-new-intersection tests

Construct proposals where:

- replacement patch is definitely clear;
- moved vertex crosses a remote triangle;
- new diagonal crosses remote geometry;
- swept envelope contacts a topology-distinct coincident sheet;
- proposed patch self-intersects;
- coordinate-equal nonadjacent features are incorrectly excluded by a mutation;
- uncertainty blocks clearance;
- adjacency is valid only by exact reciprocal edge;
- dead indexed triangles are correctly ignored;
- newly appended triangles are found in every forest-block state; and
- extreme scales stress conservative bounds.

Compare producer index results with exhaustive alive-triangle pairs for bounded fixtures. Inject false-negative index mutations and require verifier rejection.

### 24.12 Component-removal and topology-effect tests

Include:

- tiny disconnected tetrahedron below, at, and above threshold;
- several point-touching tiny components;
- duplicate coincident components;
- policy disabled/enabled;
- component with unmatched coincident ownership;
- cavity shell;
- occupied island shell;
- ambiguous semantic role;
- attempted component merge;
- attempted component split;
- thin handle removal;
- before/after Euler records with forged genus; and
- removal of all components.

Verify all-or-nothing deletion, exact member sets, conservative size, semantic report, and empty-result distinction.

### 24.13 Obligation and progress tests

Cover:

- one action discharging several obligations;
- explicit obligation absorption;
- side-effect removal with complete mapping;
- lower-severity replacement obligation;
- illegal same-severity regeneration;
- stale candidate regeneration;
- unchanged rejected-candidate suppression;
- swap oscillation;
- split/collapse oscillation;
- semantic-state cycle;
- no-candidate stall; and
- successful idempotent rerun.

Assert the exact progress tuple before/after every commit.

### 24.14 Action replay and mutation tests

For every action class:

- serialize the before state and certificate;
- replay in a fresh verifier state;
- reproduce topology, bounded geometry, budget, obligations, topology effect, and digest;
- reject stale generation or predecessor digest; and
- apply every mutation in Section 21.5 with corrected superficial counts/digests.

Required mutation survivor count is zero.

### 24.15 Boolean-shaped corpus

Use actual or faithfully serialized Component 08-12 artifacts from:

- transverse and oblique box/polytope intersections;
- concave extrusions;
- partial coplanar overlaps;
- equal operands with different source triangulations;
- cavities, islands, and disconnected shells;
- intersection curves through source vertices/edges;
- carrier clusters and high-valence events;
- thin corridors, slivers, and near-collinear fans;
- point/edge contacts with duplicated occurrences;
- tiny disconnected result components; and
- repeated Boolean chains with inherited precision.

Hand-authored local patches alone are insufficient.

### 24.16 Metamorphic and determinism tests

Apply:

- source, event, retained-use, Component 11, and Component 12 entity permutations;
- alternative legal Component 12 triangulations;
- operand exchange with operation remapping;
- axis permutation;
- orientation-corrected sign flip;
- exactly representable translation;
- power-of-two scale with precision/tolerance scaling;
- candidate insertion reversal;
- link traversal start rotation;
- hash-collision injection;
- interaction-index block partition permutations permitted only as test inputs to canonical rebuild;
- thread counts 1, 2, and maximum;
- forced task delays; and
- repeated cleanup.

For fixed versions, action sequence, proposals, final topology, coordinates, budgets, reports, diagnostics, replay, and digest are byte-identical after documented remapping.

### 24.17 Fuzzing and shrinking

Generate valid paired cleanup complexes with controlled:

- triangle/residual count;
- obligation classes;
- vertex valence;
- zero/short-edge count;
- repeated-coordinate and occurrence multiplicity;
- collinear-chain length;
- sliver quality;
- patch boundary size;
- thin handle/cavity/component size;
- coordinate exponent and ULP perturbation;
- cleanup budget;
- action limit;
- interaction density; and
- resource/cancellation limits.

For small cases compare topology eligibility and final outcomes against exact exhaustive oracles. Serialize and deterministically shrink every crash, hang, non-manifold action, missed interaction, budget undercount, obligation leak, progress cycle, nondeterminism, verifier disagreement, or unexpected typed failure while preserving complete predecessors and failure.

### 24.18 Resource, cancellation, and concurrency tests

For every resource class, test limit-minus-one, limit, and limit-plus-one. Cancel during every stable checkpoint and inside:

- initial scans;
- link/fan/component walks;
- candidate enumeration;
- patch collection;
- local triangulation;
- coordinate construction;
- budget reservation;
- replacement build;
- interaction-index query/build;
- exact relation batches;
- certificate construction;
- action splice;
- obligation regeneration;
- canonicalization;
- codec; and
- verifier replay.

Confirm all workers join, reservations release, private state rolls back, and no partial artifact publishes.

### 24.19 Structural performance gates

Record and assert:

- live/dead/appended slot counts;
- obligations by class and severity;
- candidates discovered, evaluated, stale, rejected, cached, regenerated, and committed;
- link/fan members inspected;
- patch/interface sizes;
- coordinate proposals;
- local triangulation states;
- Component 03 operations and reservations;
- interaction-index blocks/nodes/candidates;
- exact external relation tests;
- action count by class;
- cumulative progress changes;
- verifier pair candidates;
- peak temporary/persistent bytes; and
- abstract work units.

Ordinary isolated defects must remain local and output-sensitive. The append-only forest may return dead false positives, but versioned structural thresholds must prevent ordinary cases from degenerating to repeated global all-pairs work. Pathological cases may return `resource_limit`; they may not skip verification or relax correctness.

### 24.20 Platform and sanitizer matrix

Run supported `float`/`double` and internal/public index-width combinations under:

- GCC and Clang strict C++17 builds;
- debug and optimized strict-floating targets;
- ASan/UBSan;
- TSan for permitted deterministic parallel evaluation;
- supported 32-bit/64-bit public index boundaries; and
- every Component 01/03 qualified architecture/floating environment.

No valid represented degeneracy fixture should expect `internal_invariant_error` merely because cleanup is difficult.

## 25. Implementation sequence and gates

Implement in this order. Do not integrate the next step until the stated gate passes.

1. **Schemas, versions, API, and strict target.** Add IDs, enums, keys, capability validation, artifact skeleton, empty path, codec placeholders, and contract tests. Gate: invalid owners/versions/ranges fail deterministically and empty artifacts verify.
2. **Predecessor intake and preflight.** Implement Component 12/11/10/08/05/04/03 audits, checked bounds, and resource plan. Gate: every predecessor mutation fails before mutable import.
3. **Mutable cleanup complex.** Implement append-only slots, cells, pairs, incidence, generations, tombstones, snapshots, and rollback. Gate: imported clean/residual fixtures reconstruct exactly and stale handles are rejected.
4. **Independent topology primitives.** Implement producer link/fan/component/interface reconstruction and test-only independent oracle comparisons. Gate: all malformed-link and touch/coincident fixtures classify exactly.
5. **Obligation graph and progress.** Implement import, reconciliation, aggregation, dispositions, defect scan, and progress tuple. Gate: no member can disappear and every transition is monotonic.
6. **Candidate discovery and serial scheduler.** Implement complete V1 variants, complete keys, stale handling, rejection cache, and global-minimum loop. Gate: insertion/traversal/thread permutations select identical candidates.
7. **Closed patch and transaction machinery.** Implement patch closure, interfaces, private replacement, resource subleases, deltas, and rollback. Gate: failure injection at every nested phase restores byte-identical private state.
8. **Fan split provider.** Implement oriented sector partition, occurrence duplication, mappings, and composite proposal support. Gate: split corpus and mutation survival zero.
9. **Collapse provider.** Implement link condition, coordinate variants, quotient, duplicate-edge splices, triangle rewrites, and exact topology certificates. Gate: legal/illegal collapse oracle suite and no non-manifold survivors.
10. **Retriangulation and residual absorption.** Implement swaps, local disks, exact coverage, collinear chains, folds, residual clusters, and paired diagonals. Gate: boundary/coverage/oscillation/exhaustive fixtures pass.
11. **Patch correspondence, budget, and precision.** Implement affine mappings, feature/component metrics, Component 03 reservation/commit adapters, cumulative lineage sums, and precision refresh. Gate: exact-oracle and tolerance-boundary suites pass with rollback.
12. **Interaction forest and geometry checks.** Implement append-only blocks, proposed/swept queries, topology exclusions, self/external relations, clearance, and side plausibility. Gate: exhaustive pair comparison has zero false negatives and all intersection mutations fail.
13. **Component removal and topology effects.** Implement full component snapshots, conservative size, semantic role, Euler/genus reports, and V1 policy rejection. Gate: component/cavity/handle corpus passes.
14. **Atomic action certificates and replay.** Integrate all evidence, action splice, obligation regeneration, progress, and deterministic diagnostics. Gate: every action replays from declarative records.
15. **Final canonicalization, codec, and producer verification.** Implement alive remap, final topology/geometry checks, reverse maps, canonical bytes/digests, and idempotence adapter. Gate: byte stability and Component 14 intake fixtures pass.
16. **Independent verifier.** Implement separate sequential replay, topology/budget/intersection reconstruction, alternate pair enumeration, codec checks, and mutation inventory. Gate: zero required mutation survivors.
17. **Qualification.** Complete Boolean corpus, exact oracle, fuzz/shrink, metamorphic, resource, cancellation, concurrency, sanitizer, platform, and structural performance suites. Gate: every definition-of-done item below passes before Component 14 implementation begins.

Keep implementation commits reviewable and bisectable, but never expose a partially implemented or verifier-skipped cleaned artifact as a supported downstream contract.

## 26. Definition of done

The Component 13 implementation plan is fulfilled only when all of the following are true:

- every Component 12 cleanup obligation and residual cell is explicitly discharged, proven final-valid, absorbed with complete member evidence, or causes a precise typed stage failure;
- every committed action begins from exact topology/lineage and preserves a valid oriented paired cleanup cell complex atomically;
- every successful final entity is a triangle-manifold entity: reciprocal pairs, exactly two edge uses, three distinct triangle corners, and one closed fan per vertex occurrence;
- no prohibited zero-area triangle, zero-length edge, residual cell, pinched occurrence, or cleanup obligation remains;
- no unrelated, point-touching, edge-touching, or coincident-sheet feature is welded from coordinate proximity or equality;
- every edge collapse satisfies independently reconstructible exact link and quotient conditions;
- every required fan split has complete oriented sector membership, zero hidden movement, and preserved lineage;
- every retriangulation preserves its exact interface, protected constraints, orientation, and bounded coverage;
- every coordinate proposal is deterministic, bounded, support-compatible, and charged to every original lineage;
- cumulative same-lineage displacement is an outward-rounded sum and every feature/component removal is conservatively bounded within policy;
- all changed geometry has fresh Component 03 precision, residual, orientation, length, and AABB evidence;
- every moved or retriangulated patch passes conservative self, swept, external-intersection, clearance, and side-plausibility checks;
- valid thin features are retained unless a named obligation and authorized certificate require removal;
- whole-component removal is explicit, all-or-nothing, semantically permitted, within budget, and fully reported;
- V1 performs no component merge/split, handle/cavity/genus change, or unsupported topology change;
- every obligation transition and action strictly reduces the frozen progress measure, cleanup terminates, and successful cleanup is idempotent;
- every action has a complete replayable before/after, topology, correspondence, budget, precision, intersection, obligation, and topology-effect certificate;
- final canonical records, reverse maps, reports, diagnostics, replay, and digests are independent of transient IDs, traversal, allocation, worker count, and schedule;
- the independent verifier replays every action, reconstructs final topology and geometry, performs an independent forbidden-intersection search, and rejects every required mutation;
- resource exhaustion and cancellation join workers, release reservations, roll back all private state, and publish nothing;
- exact-oracle, known-answer, Boolean-corpus, property, metamorphic, adversarial, fuzz/shrink, mutation, resource, cancellation, concurrency, sanitizer, platform, idempotence, and structural-performance suites pass;
- all production and normative-test code is strict portable C++17 with no external dependency; and
- Component 14 can consume `cleaned_triangle_manifold<T>` directly without further cleanup, welding, topology repair, reorientation, coordinate change, or obligation interpretation.
