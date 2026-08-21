# Component 7 implementation plan: exact local facet refinement

## 1. Scope and outcome

Implement the operation-independent stage that consumes Component 2's validated source facets and Component 6's immutable `symbolic_complex<T, I>`, constructs the complete exact planar arrangement induced on every source facet, and publishes one immutable `refined_facet_patches<T, I>` bundle. Every semantic source/intersection/overlap constraint must be represented by canonical atomic edges; every positive-area cell inside a source facet must become an oriented patch; and the patches must exactly subdivide, but never move or reclassify, the source facet.

The completed component must provide:

- one `local_refinement` for every validated source facet, including a one-patch identity refinement for an untouched facet;
- exact projection through each facet's frozen deterministic chart and no Cartesian `T` construction;
- source boundaries rebuilt solely from Component 6's shared edge split sequences;
- complete per-facet collection, normalization, and atomization of intersection and coplanar-overlap constraints;
- a canonical planar DCEL-like graph with vertices, atomic edges, halfedges, twins, vertex-star order, cycles, cells, and complete provenance;
- exact extraction of all and only positive-area cells inside the source polygon, including cells with multiple boundary orbits, holes, slits, and dangling-edge spikes;
- deterministic decomposition of every non-Jordan cell into simple positive-area patches using exact, non-quality-driven cuts, with polygon-with-holes cells retained directly when no cut is needed;
- exact constraint coverage, source-boundary equality, area, boundary-cancellation, incidence, and Euler certificates;
- a controlled, globally visible reconciliation path for a constraint crossing missing from the symbolic registry;
- deterministic IDs, encoding, diagnostics, accounting, cancellation, replay, independent verification, and transactional publication.

Component 7 does not stitch different source facets, infer seam sectors, merge coincident sheets across facets, classify operand occupancy, evaluate a Boolean operation, realize `T` coordinates, or simplify geometry. Those are Components 8-12. It must preserve distinct coincident source-sheet and constraint provenance even when several labels occupy one local atomic edge.

All implementation must be self-contained in Ygor, portable C++17, and free of external dependencies. Do not include or call `src/YgorMeshesBoolean{,2,3,4,5}.{h,cc}`.

## 2. Existing Ygor assessment and reuse boundary

### 2.1 Reuse and adapt

- Consume Component 1's `boolean_context`, owner tokens, strong IDs, accounting containers, checked arithmetic, deterministic executor, canonical encoder, diagnostics, trace, replay binding, verifier registry, artifact transactions, cancellation, and typed failures. Use the expanded `verification_environment_view` required by Components 2 and 6.
- Consume Component 2's immutable exact source rings, edge uses/twins, undirected-edge directions, support plane and right-handed chart, source orientation/occupied side, shell/operand provenance, and exact source area. Never rescan the borrowed `fv_surface_mesh`.
- Consume Component 3's exact chart projection, equality, segment relation, point/segment and point/polygon-with-holes classification, parameter extraction, `orient2d`, angular order, signed area, exact witnesses, diagonal visibility, construction storage, and fallible batch pre-ranking. Standard sorting must inspect only immutable ranks and fixed fields.
- Consume Component 6's symbolic points, canonical carriers/atomic intervals, source-edge split sequences and reversed edge-use views, normalized incidence, raw interval/region mappings, ownership/orientation multiplicities, and construction/provenance records. Retain strong ownership of the exact symbolic and validated artifacts.
- Adapt the half-plane/orientation vertex-star ordering, directed-halfedge left-face walk, and canonical cycle rotation patterns from `src/YgorMathMonotoneDecomposition.cc`. Replace floating coordinates, mutable raw indices, `long double` area, approximate containment, collinear cleanup, and position-based ties.
- Adapt canonical undirected-edge aggregation, directed-use comparison, and Euler accounting patterns from `src/YgorMeshesVerification.*` and `src/YgorMeshesHoles.*`. These are structural patterns only; their triangle-only, epsilon-welding, and mutable repair behavior is prohibited.
- Adapt the source-edge-to-all-incident-facets data-flow pattern from `fv_surface_mesh::slice_with_planes`, but use Component 6's one authoritative symbolic split sequence rather than constructing rounded intersections per facet.
- Use `YLOGDEBUG`, `YLOGINFO`, or `YLOGWARN` only to forward optional summaries after deterministic records are frozen. Never use `YLOGERR` for an expected or invariant failure.

No current Ygor facility implements an exact provenance-rich planar arrangement over stable symbolic identities. Components 1-6 are planned production prerequisites and must be implemented and passing before Component 7 production integration.

### 2.2 Reject as production behavior

- Do not call `Constrained_Delaunay_Triangulation_2`. It rejects duplicate, crossing, and T-junction constraints; uses `vec2<T>`, `atan2`, `long double`, rounded lifting, quality legalization, raw indices, and discards constraint provenance.
- Do not call `Monotone_Decomposition_2` or `Triangulate_Monotone_Decomposition`. They remove collinear vertices, reverse rings, require loops not to touch, use rounded areas/midpoints, create position-selected bridges, and emit detached coordinate/index structures.
- Do not call generic `YgorMath` point/segment/polygon functions, `YgorMeshesAdaptivePredicates`, mesh slicing, Loop subdivision, hole welding, or orientation repair. Component 3 is the only geometric authority.
- Do not use `vec2<T>`, `vec3<T>`, `long double`, epsilon, snapping, welding, rounded coordinate equality, `atan2`, normalized floating directions, quality scores, Delaunay/incircle optimization, or random perturbation for any semantic choice.
- Do not erase collinear registry vertices, repeated constraint ownership, zero-area incidence cycles, source-edge split points, or a derivation merely because positive-area output is unchanged.
- Do not use pointers, hash values, worker/insertion order, temporary indices, allocator order, or source ring start as canonical identity.
- Do not create a facet-private symbolic point when two constraints cross. Publication must use one globally registered `symbolic_vertex_id` and the successor Component 6 artifact that owns it.

## 3. Files, namespace, and integration

Add:

- `src/YgorMeshesBooleanLocalRefinement.h`: closed schemas, owner-bound local references, immutable arrangement/patch/certificate records, read-only accessors, artifact constants, reconciliation request schema, and stage entry point.
- `src/YgorMeshesBooleanLocalRefinement.cc`: facet constraint collection, exact normalization, crossing audit, DCEL construction, cell extraction/decomposition, deterministic merge/encoding, coordinator, verifier adapter, and four `<T, I>` explicit instantiations.
- Modify `src/YgorMeshesBooleanContract.{h,cc}` and `src/YgorMeshesBooleanTransaction.h`: add successor-generation IDs/limits/accounting/replay fields and the owner-checked, stale-safe compare-and-publish transaction operation.
- Modify `src/YgorMeshesBooleanSymbolicRegistry.{h,cc}`: add the owner-checked reconciliation request consumer, immutable successor-generation construction, and compare-and-publish integration anticipated by Component 6's handoff contract.
- `tests/Test_MeshesBooleanLocalRefinement.cc`: focused contract, arrangement, degeneracy, coverage, failure, mutation, and round-trip tests.
- `tests/Test_MeshesBooleanLocalRefinementProperties.cc`: generated permutation, insertion-order, differential, metamorphic, sharding, and schedule tests.
- `tests/MeshBooleanLocalRefinementFixtures.h`: synthetic validated/symbolic artifacts, exact planar constraint builders, independent exhaustive arrangement oracle, bit-pattern fixtures, deterministic PRNG, mutation helpers, and replay records.
- Modify `tests/Test_MeshesBooleanContract.cc`: cover generation limits, latest/prior reads, strong prior lifetime, successful compare-and-publish, stale writers, concurrent successors, cancellation, accounting, replay, and rollback.
- Modify `tests/Test_MeshesBooleanSymbolicRegistry{,Properties}.cc` and its fixtures: cover accepted/rejected reconciliation requests, successor generations, interval/source-edge re-splitting, stale compare-and-publish, deterministic request permutation, rollback, and verifier mutation.

Use namespace `ygor::mesh_boolean`. Keep provisional segment occurrences, sweep/exhaustive crossing scratch, rank tables, adjacency worklists, worker shards, and verifier implementation private to the `.cc` or tests. Public templates remain thin and use explicit instantiation for `float`/`double` with `std::uint32_t`/`std::uint64_t`.

Add the `.cc` to Component 1's explicit strict-arithmetic source list in `src/CMakeLists.txt`; effective fast-math and contraction must be disabled and compile-time guarded. Add both tests to the in-tree authoritative CTest integration and `tests/compile.sh`, register `MeshBooleanLocalRefinement.Unit` and `.Properties`, and label them `mesh_boolean;component7`. Do not depend on `tests2` network-fetched doctest. Add authoritative GCC/Clang Debug/Release, ASan/UBSan malformed-artifact/rollback, and TSan facet-shard/reconciliation/cancellation/publication runs.

Before coding, reconcile these prerequisite interfaces instead of adding parallel facilities:

1. Component 1 must provide artifact-owned strong local vertex-occurrence, shared/local atomic-edge, halfedge, boundary-walk, face, and patch references, or explicitly approve equally owner-safe typed dense references scoped by `facet_id`; only `local_patch_id` is currently named. Add corresponding `feature_ref`, limits, canonical encoding, and diagnostics only for references that cross artifact/stage boundaries.
2. Resource policy must account separately or through documented aggregate limits for local point/segment occurrences, shared/local atomic edges, halfedges, boundary walks, faces, patches, certificate entries, rank scratch, and reconciliation requests. `local_patches` alone is insufficient to bound arrangement construction.
3. Component 3 must expose exact batch APIs for projected-point ranks, segment relations/parameters, direction rays, boundary-walk area, polygon-with-holes membership, plane-complement witnesses, and deterministic diagonal visibility. Fallible arithmetic cannot occur inside an STL comparator.
4. Component 6 must expose a deterministic per-facet constraint view containing all incident zero-dimensional points, atomic intervals, overlap-region boundary cycles, complete labels, and mappings, plus the verified reconciliation coordinator specified in Section 8. Component 7 must not globally rescan raw events to recover omitted per-facet semantics.
5. Component 1's artifact store must support the Section 8 owner-checked compare-and-publish of an immutable successor generation while retaining prior generations for existing strong holders. Amend transaction, dependency-generation, slot-read, accounting, replay, and stale-writer tests; do not overwrite or mutate an existing published artifact in place.

## 4. Stage and ownership contract

Expose a coordinator equivalent to:

```cpp
template<class T, class I>
status_or<std::shared_ptr<const published_artifact<refined_facet_patches<T, I>>>>
refine_source_facets(boolean_context<T, I>& context);
```

The coordinator must:

1. Require the exact published `symbolic_complex<T, I>` from `artifact_slot::symbolic_complex`, its strongly retained `validated_operands<T, I>`, matching owner/setup/kernel policy, and the registered Component 7 verifier specification.
2. Reject copied or replacement dependencies even if bytes/digests match. A valid successor symbolic artifact produced by Section 8 is accepted only after restarting against its new strong identity and generation.
3. Validate all dense IDs, source roles, facet incidences, curve endpoints, region cycles, edge-use views, construction owners, and per-facet ranges before geometric work.
4. Run the global crossing-completeness audit before opening the publication transaction. If it returns new reconciliation requests, invoke Section 8 and restart the entire stage; never mix refinements from two symbolic generations.
5. Open one `local_refinement` transaction targeting `artifact_slot::refined_facet_patches`. All per-facet drafts, IDs, stores, encodings, reports, and diagnostics remain private until every facet succeeds.
6. Produce refinements in canonical `(operand role, facet_id)` order. Geometry is operation-independent; union/intersection/difference/xor over the same upstream artifacts yield identical local semantics.
7. Normalize, assign final IDs, encode, independently verify, and publish atomically only after a final cancellation check and matching certificate.
8. Return `resource_limit` for declared bytes/work/entity/exact-number/trace/reconciliation limits, allocation failure, or cancellation; `internal_invariant_error` for malformed dependencies, contradictory constraints, repeated non-progressing reconciliation, producer/verifier mismatch, or arrangement defects.

The published artifact retains typed strong handles to the exact symbolic complex, validated operands, and escaped exact construction storage. It contains no borrowed mesh pointer, provisional index, mutable adjacency map, worker shard, rank scratch, rounded coordinate, or facet-private construction.

## 5. Public artifact schema

### 5.1 Closed kinds and local references

Freeze numeric enums for constraint source (`source_boundary`, `intersection`, `overlap_boundary`), point-incidence kind, atomic-edge role, local edge semantic kind, halfedge direction relation, boundary-orbit area class (`positive`, `negative`, `zero_area`), face extent (`bounded`, `unbounded`), cell role, decomposition kind, reconciliation reason, and certificate invariant. Reject unknown values during decode. Area class belongs to each walk; boundedness belongs only to its owning face, so an unbounded face may own negative and zero-area walks simultaneously.

All facet-local references carry or are checked against the artifact owner and owning `facet_id`. IDs are dense only after all semantic records are canonicalized. A local vertex occurrence references exactly one Component 6 `symbolic_vertex_id`; Component 7 never interns a new geometric point. The top-level bundle additionally assigns `shared_atomic_edge_id` values to semantic, non-artificial exact 3D segments so adjacent facets refer to one identity rather than merely rediscovering equal endpoint pairs.

### 5.2 Constraints and atomic edges

Define normalized immutable records equivalent to:

```cpp
struct local_constraint_label {
    local_constraint_source source_kind;
    feature_ref source;
    optional<symbolic_curve_id> curve;
    optional<raw_event_id> overlap_region;
    orientation_parity direction;
    exact_multiplicity multiplicity;
    accounting_vector<raw_derivation_ref> derivations;
};

struct local_atomic_edge {
    local_atomic_edge_ref id;
    optional<shared_atomic_edge_id> shared_semantic_edge;
    local_vertex_ref lower;
    local_vertex_ref upper;
    optional<symbolic_curve_id> canonical_interval;
    accounting_vector<local_constraint_label> labels;
    bool source_boundary;
    bool artificial;
};
```

Use concrete prior-component types. Endpoint order is the exact projected lexicographic/canonical direction order, not halfedge direction. Semantic edges must have two distinct symbolic endpoints and positive exact length. Their bundle-level shared identity key is the exact unordered pair of endpoint `symbolic_vertex_id`s after proving the straight 3D segment is the same; a Component 6 atomic interval or `(undirected_edge_id, consecutive split positions)` is retained as validating identity/provenance, not as a competing identity. Every semantic edge has one shared ID across all facet-local occurrences. `artificial` is false for every source/intersection/overlap constraint atom; decomposition diagonals are facet-local atoms with no shared semantic ID, curve, or source multiplicity and have an explicit exact visibility certificate.

Several coincident occurrences normalize to one geometric atomic edge while their sorted labels and exact multiplicities are retained. Local geometric edge identity is exact endpoint-pair equality after atomization; carrier and canonical-interval IDs are labels and proof obligations, not identity separators. Same/opposite directions are payload and may cancel only in a later semantic stage; Component 7 never drops them. An atomic edge may simultaneously be a source boundary and an overlap/intersection constraint.

Publish a `local_point_incidence` for every Component 6 symbolic point incident to the facet, including isolated point contacts and point-only carriers that are not endpoints of a positive-length atom. It stores the local vertex occurrence, exact facet location (`source_vertex`, `source_edge_interior`, or `facet_interior`), all source/event/carrier incidence, derivations/evidence, and whether the point is used by an arrangement edge. An isolated point does not split a two-dimensional cell or become a fictitious zero-length edge, but it remains queryable by Component 8 and fully verified.

### 5.3 DCEL, cycles, cells, and patches

For each geometric atomic edge publish exactly two directed halfedges with origin/destination, twin, next, previous, owning edge, left cell/boundary orbit, direction relative to edge canonical order, and complete label view. Publish each vertex's outgoing halfedges in strict exact counterclockwise order, with collinear same-ray occurrences grouped by proven equality and represented by one geometric edge carrying multiplicity.

Publish the permutation orbits of `next` as canonical boundary walks, not assumed-simple polygon cycles. A walk may repeat vertices and may traverse the two sides of a dangling bridge/slit as a spike while still having nonzero total area. Each walk stores its exact signed doubled area, owning face, role, and containment facts. A face may own several walks from disconnected graph components; a zero-area walk is a boundary component/incidence orbit assigned to a face, never an additional face solely because its area is zero. Publish the single unbounded face and every bounded face explicitly.

For a face whose boundary is one simple outer walk plus simple, disjoint hole walks, publish that polygon-with-holes directly as one `local_patch`. A face with a weakly-simple spike, doubled slit, isolated non-Jordan boundary component, repeated vertex, or other non-polygon boundary must be deterministically cut into simple polygon-with-holes patches so every semantic edge remains patch-boundary incidence. The frozen schema-v1 policy treats the semantic arrangement plus accepted artificial cuts as one planar straight-line graph and repeats this augmentation:

- enumerate every unordered pair of existing registered local vertices not already joined; atomize a candidate at every registered symbolic point in its open interior, and retain it only if every resulting open segment lies in the parent face, does not cross or overlap an existing semantic/artificial edge, and exact endpoint-cone/visibility tests permit insertion;
- rank valid candidates by their complete sequence of canonical symbolic endpoint pairs, never length or quality; accept only the least candidate and recompute stars, walks, faces, and candidates after that one insertion rather than accepting a stale simultaneous batch;
- continue until every positive-area child face has one simple outer walk and zero or more simple, pairwise-disjoint hole walks, with no repeated vertex, bridge spike, slit, or disconnected non-hole boundary component;
- use the strictly increasing set of noncrossing endpoint pairs as the termination measure; only finitely many atomized pairs over the fixed registered vertex set exist, and maximal planar augmentation of this valid straight-line graph must have Jordan bounded faces, although collinear boundary subdivisions remain;
- if a non-Jordan positive-area face remains when no valid candidate exists, return `internal_invariant_error` with its complete walk/visibility evidence; never emit a weak patch or invent a Steiner point;
- records paired zero-width bridge occurrences only in decomposition topology, not as semantic arrangement edges, and never counts the two sides as positive-area geometry;
- emits positive-area pieces with exact source orientation;
- marks every introduced edge artificial and preserves a parent-cell mapping;
- requires every original semantic edge side to occur on the resulting patch boundaries with its original shared identity and label view;
- is invariant under cycle rotation, ring reversal through the documented orientation mapping, and worker order.

Each patch stores source facet/operand/shell, source orientation and occupied-side relation, outer/hole walks or decomposed boundary, exact signed area, all boundary atomic-edge/halfedge uses, and its parent face. Refinement does not assign occupancy labels for the other operand.

### 5.4 Maps and coverage certificate

Each `local_refinement` publishes:

- a complete source-edge-use boundary map to the exact directed chain from Component 6's split sequence;
- a point-incidence map for every zero-dimensional facet event and a constraint-occurrence map for every positive-length occurrence to a nonempty ordered chain of local atomic edges with parity and multiplicity;
- a bundle-level shared-semantic-edge store and mappings from every facet-local semantic edge occurrence to exactly one shared identity;
- symbolic vertex and curve incidence ranges;
- local vertex, point-incidence, atomic-edge, halfedge, boundary-walk, face/cell, and patch stores;
- parent-cell/decomposition mappings and artificial-edge records;
- one `local_coverage_certificate`.

The certificate stores exact source area, sum of positive-area patch areas, a normalized oriented boundary sum, normalized point/segment constraint coverage counts, graph component count `C`, `V` including isolated vertices, geometric `E`, directed `H=2E`, actual face count `F` including exactly one unbounded face, bounded/source-domain face counts, boundary-walk counts by sign/zero area, hole counts, and decomposition-cut facts. It explicitly proves the planar embedding identity `V - E + F = 1 + C` and the derived bounded-face identity `F_bounded = E - V + C`; zero-area walks are not added to `F`. Store facts and exact values, not merely booleans. The mandatory verifier recomputes them.

The top-level bundle contains a dense canonical facet range for every source facet, dependency bindings, deterministic semantic statistics, and a combined certificate digest. Empty operands produce a valid empty bundle; an untouched facet produces its source boundary, one positive-area cell, and one identity patch.

## 6. Per-facet constraint construction

For each facet in canonical order:

1. Load the validated exact ring, plane/chart, edge uses, orientation, exact area, and Component 6 per-facet constraint view. Prove every referenced symbolic point lies exactly in the support plane before projection.
2. Project each symbolic point through the frozen right-handed chart. Projection is an exact coordinate selection/permutation; preserve the 3D symbolic identity and construction handle.
3. Replace every source edge use by consecutive pairs from its Component 6 split-sequence view. Require exact endpoint mapping, direction parity, strict parameter order, and equality with the reverse chain used by the twin facet.
4. Add every zero-dimensional symbolic point incident to the facet, every canonical interval incident to it, and every mapped outer/hole edge of every overlap region. Require point location/provenance and endpoint, carrier, facet/plane incidence, region-cycle direction, ownership, and multiplicity consistency. Retain isolated points even when no segment uses them.
5. Split every occurrence at every registered symbolic point proven to lie on its closed segment. Use canonical carrier/source-edge parameter order; no point may be inferred by rounded projection.
6. Compare potentially intersecting occurrences with Component 3's exact segment relation. Conservative exact/rational bounds may reduce pairs but exhaustive mode checks all pairs. Classify disjoint, shared endpoint, T-junction, proper crossing, equal interval, and partial collinear overlap.
7. Send every non-endpoint crossing lacking a registered endpoint through Section 8. Collinear overlap endpoints must already be symbolic; a missing endpoint also requests reconciliation. Any exact crossing mapped to a different symbolic point or equal parameter with different point is an invariant error.
8. Gather all registered breakpoints on each canonical support segment, pre-rank parameters, and emit maximal positive open atoms between consecutive points. Merge geometric atoms by exact endpoint-pair equality after exact support/coverage proof and union normalized carrier/interval labels and multiplicities; do not merge across any registered vertex or provenance transition required by a mapping.
9. Verify every input occurrence maps to a gap-free, correctly directed atomic chain, every atom is covered by at least one non-artificial occurrence, and no two distinct geometric atoms have overlapping open interiors.

Source boundary orientation follows the source facet ring. Interior constraints contribute both halfedge directions but do not by themselves toggle whether a local cell belongs to the source facet; the source polygon domain does.

## 7. Arrangement formation and patch extraction

### 7.1 Embedded graph

Materialize one local vertex for every point incidence and every endpoint used by an atom; isolated vertices participate in `V`/`C` but have an empty star until an artificial cut legitimately uses them. Pre-rank every outgoing exact ray with Component 3's half-plane plus `orient2d` angular order. Equal rays must be collinear and same-direction; after atomization, nearer endpoints cannot hide an unsplit farther edge. Reject duplicate geometric atoms that survived normalization.

Create paired halfedges. At destination `v`, set `next(h)` to the outgoing halfedge immediately clockwise from `twin(h)` in `v`'s CCW star so each walk keeps its face on the left; derive `previous` as the inverse permutation. Require twin involution, next/previous involution, complete star membership, and one visit of every halfedge across all cycles.

Walk all boundary orbits with checked guards bounded by halfedge count. Compute exact signed area while retaining repeated vertices/edges. Preserve zero-area walks as boundary/incidence components with complete halfedges and labels; never reinterpret them as slivers or additional faces. Positive and negative walks are not identified as bounded/exterior from sign alone when components, holes, slits, or spikes are present.

### 7.2 Cell/domain extraction

Construct an exact left-side open witness for every directed boundary-walk occurrence at a nondegenerate edge and use exact point classification plus connectivity across all graph edges to identify face equivalence. Two boundary orbits belong to the same face exactly when their certified left-side witnesses are in the same connected component of the plane complement; establish this with a deterministic vertical/canonical-ray decomposition over all atoms, not containment alone. This labels every orbit with one face, groups disconnected/nested boundary components correctly, and identifies the unique unbounded face. Classify one checked open witness per resulting face against the validated source polygon; boundary classification is an invariant defect.

Then:

- retain as source-domain cells exactly the open regions classified inside the source facet;
- classify each simple negative boundary component as a hole of its owning face only after face equivalence and exact least-container proof; do not classify a spike or zero-area orbit as a hole;
- keep exterior and outside-source cells in the DCEL for verification but emit no patch for them;
- do not infer inclusion by odd/even constraint multiplicity, because intersection curves subdivide without changing source-facet membership;
- prove every bounded face receives exactly one inside/outside source-domain classification and that every boundary orbit belongs to exactly one face.

The validated source facet is simple and has no holes in schema v1, but arrangement cells can have holes due to closed internal constraints. If Component 2 later admits facet holes, use its explicit outer/hole domain directly without changing this stage's cell semantics.

### 7.3 Canonical IDs and exact coverage

Canonicalize boundary walks by orientation-aware minimum rotation over semantic halfedge keys, preserving repeated entries. Rank bundle-level shared semantic edges first by their exact endpoint pair; rank local vertices by symbolic ID/projected exact rank; edges by shared identity or artificial endpoint/certificate key; halfedges by edge and direction; walks by role/normalized sequence; faces by canonical witness and complete walk set; and patches by parent face/decomposition key. Assign global `local_patch_id`s in facet-key then local-patch-key order. Other artifact-local IDs are assigned from their complete canonical keys.

Before a facet draft can succeed, prove:

1. Every source boundary atom appears once in the directed source chain and the chain exactly equals Component 6's edge-use views around the validated ring.
2. Every semantic constraint occurrence is covered by its published chain with exact direction, labels, multiplicity, and no uncovered or extra positive-length segment.
3. Atom open interiors are disjoint and intersections occur only at shared symbolic endpoints.
4. Twins, next/previous, star orders, boundary walks, face equivalence, face boundaries, and containment are complete and mutually consistent; isolated point incidences map exactly once without becoming edges.
5. Every positive-area patch has exact sign consistent with source orientation; mandatory cuts for non-Jordan faces and any further decomposition pieces exactly cover their parent face.
6. Sum of exact patch areas equals exact source-facet area.
7. Oriented patch boundaries cancel on every non-source internal edge with equal opposite multiplicity and reduce exactly to the directed source boundary on source edges.
8. For the complete per-facet planar graph, with isolated vertices included and faces grouped rather than boundary walks counted, `V-E+F=1+C` and `F_bounded=E-V+C`; patch decomposition independently preserves parent-face area/boundary.

Exact zero/nonzero decides all area behavior. A very small positive patch is valid and mandatory.

## 8. Controlled symbolic reconciliation

Define a `symbolic_reconciliation_request` containing the old symbolic generation/digest, facet, two normalized constraint occurrence keys, exact segment relation, exact crossing point/construction, required source/facet/carrier incidence, derivations/evidence, and a canonical request key. Component 7 only detects and reports; it does not assign the point an ID.

The pre-transaction global audit merges requests from all facet shards, proves duplicate requests exact-equal, sorts them canonically, and passes them to a Component 6 successor API equivalent to:

```cpp
reconcile_symbolic_complex(context, prior_symbolic_artifact, requests);
```

Implement that API in `YgorMeshesBooleanSymbolicRegistry.{h,cc}`, not in a Component 7 private helper. It must validate request construction ownership; independently verify every crossing against the prior artifact and exact facet constraints; intern points globally; split affected carriers, intervals, region boundaries, and source-edge sequences; and rebuild all mappings/incidence/orders/encodings. Extend the immutable Component 6 schema with a canonical, complete reconciliation-request store: each accepted request becomes a first-class provisional point member and retained provenance source alongside original vertices and raw point events. Revise the independent Component 6 verifier to reconstruct equivalence classes, constructions, incidence, and atomization from original/raw members plus every retained canonical request; it must prove each request against source constraints without calling Component 7 producer helpers. The normal generation has an empty request store and unchanged semantics.

After that revised full verifier passes, run successor-chain checks. The artifact transaction records `(slot, expected prior generation, successor generation)` and uses owner-checked compare-and-publish: a stale expected generation fails without changing the slot, a successful publication changes the context's latest-generation pointer atomically, and the prior immutable artifact remains alive and address-stable for existing holders. The successor strongly retains the prior artifact, complete request records, and all new/old construction storage needed by its records. Component 7 then discards all drafts/ranks and restarts from dependency validation.

The context tracks canonical request keys and generation progress. A request already satisfied by the successor is retained in history but not re-added; an identical unsatisfied request after one reconciliation, a request contradicting existing identity/incidence, or a successor with unchanged symbolic semantics is `internal_invariant_error`. Freeze `symbolic_reconciliation_schema_v1`, complete request encoding, successor encoding fields, generation limits, failure precedence, cancellation points, canonical request merge, and replay records in the Component 6 interface. `YGBCAN06` and `YGBSYM06` successor encodings include normalized request semantics/provenance rather than only a digest. Checked limits bound requests, successor generations, work, bytes, exact constructions, and retained-generation storage. No partial Component 7 artifact is visible during reconciliation, and old immutable symbolic artifacts remain valid for existing holders.

This protocol is only a completeness backstop. Focused tests must show normal Component 5/6 output requires zero reconciliation passes.

## 9. Canonical encoding, deterministic execution, and failures

Define `YGBCAN07` as operation/invocation-independent local-refinement semantics: schema/type versions; provenance-free canonical operand/symbolic semantic digests; chart/atomization/star/face/decomposition policy versions; bundle-level shared semantic edges; refinements in canonical facet order; point incidences, vertices, semantic/artificial edges, halfedges, boundary walks, faces, patches, maps, exact certificate facts; and deterministic semantic counts. Exclude owner tokens, pointers, setup digest, raw invocation ordinals, diagnostics, traces, worker data, timings, hash values, and approximations.

Frame invocation-bound `YGBREF07` with schema/type versions, setup digest, exact strong dependency generation/digests, policy versions, deterministic statistics, length-prefixed `YGBCAN07`, invocation-bound provenance/evidence, reconciliation history digest, and construction-storage binding. Wrap it with Component 1's `YGBART01` framing for `refined_facet_patches`. Decode rejects unknown enums/versions, bad owners/IDs/ranges, noncanonical exact values, unsorted/duplicate labels, broken cycles, bad lengths, and trailing bytes.

Partition the canonical facet range by a fixed versioned frontier independent of thread count. The crossing audit completes globally before arrangement construction. Workers write accounting-backed private facet shards and cannot assign final IDs or publish. Grant work/kernel/storage envelopes in sorted `canonical_work_key(local_refinement, facet_key, phase)` rounds; join all workers and select failures by canonical precedence. The coordinator alone merges facets, assigns bundle IDs, encodes, invokes verification, and publishes.

Before work, use checked arithmetic to bound source split occurrences, point/interval/region references, pair-audit work, potential atoms, halfedges, boundary walks, faces, cut edges, patches, mappings, certificates, sort scratch, and canonical bytes. Use conservative sparse bounds and deterministic chunk requests rather than unchecked quadratic products. Never omit a point/segment constraint, provenance label, zero-area incidence, or patch to fit a limit.

Check cancellation before dependency validation, each grant/worker phase, bounded segment-pair interval, exact fallback, reconciliation, allocation/growth, cycle walk, witness/classification, decomposition, encoding, verification, and publication. Catch `bad_alloc` and unexpected exceptions only at task/stage boundaries, join siblings, select the canonical failure, and roll back. Diagnostics identify facet, local/source/symbolic/curve IDs, invariant or reconciliation code, exact evidence, requested/current/limit facts, dependency generations/digests, and replay token.

## 10. Mandatory independent verifier

Register a stable Component 7 artifact tag, schema/checker versions, and invariant set with Components 1/13. The read-only verifier receives the artifact, exact symbolic/validated dependencies, exact-only kernel services, accounting, and cancellation. It must not call producer atomization, star construction, face walk, cell grouping, decomposition, ID assignment, certificate, or encoding helpers.

Mandatory checks:

1. Validate owner, slot/tag/schema, strong dependency identities/generations, setup/kernel/dependency digests, dense IDs, facet ranges, enum domains, role-qualified references, and exact construction owners.
2. Independently reconstruct every source boundary from Component 6 split sequences and every per-facet semantic constraint from curve/region mappings. Compare complete occurrence maps, labels, direction, multiplicity, and provenance.
3. Reconstruct every zero-dimensional facet incidence, including isolated point contacts/point-only carriers; prove exact facet location, complete provenance, one mapping, and correct used-by-edge status.
4. Exhaustively evaluate all constraint-segment pairs exact-only. Prove every intersection is a shared registered symbolic endpoint, every collinear overlap is fully atomized, and no reconciliation request remains.
5. Independently split occurrences by exact parameters and compare the complete geometric atomic-edge set and bundle-level shared identities. Prove positive length, endpoint incidence, no open overlap, and exact occurrence-chain coverage.
6. Recompute each vertex star with an independently written exact angular comparator; rebuild twins and next/previous relations and compare every halfedge and possibly weak boundary walk, including spikes and doubled slits.
7. Independently group boundary walks into actual faces using exact left witnesses and a separate plane-complement connectivity implementation. Recompute walk areas, unique unbounded face, source-polygon classifications, containment/hole roles, and all bounded source-domain faces; never equate walk count with face count.
8. Validate each mandatory non-Jordan cut and optional decomposition diagonal by exact visibility, noncrossing, canonical choice, source orientation, and complete atomization at every registered point in its closed segment. Reconstruct parent-face coverage, require every semantic edge side on patch boundaries, and prove artificial edges have no semantic labels/shared identity.
9. Recompute exact area sums, normalized boundary cancellation, point/segment constraint coverage, source-boundary equality, `C`, `V`, `E`, `H`, actual `F`, `V-E+F=1+C`, and `F_bounded=E-V+C`; compare every certificate fact.
10. Compare adjacent source facets through bundle-level `shared_atomic_edge_id`: their source-edge maps must reference identical shared atoms in exact reverse order, not merely endpoint-equal local records.
11. Independently re-encode and compare `YGBCAN07`, `YGBREF07`, and `YGBART01`; verify statistics, report, trace, replay, reconciliation-history, and construction-storage bindings separately.

Verifier resource exhaustion prevents publication with `resource_limit`; producer/verifier disagreement is `internal_invariant_error`. Exhaustive mode compares bounded cases against a separately implemented rational arrangement oracle that enumerates all intersections, constructs stars, and flood/classifies faces without using producer helpers.

Mutation tests alter every ID/owner/range, isolated point/location/provenance, shared edge identity, symbolic endpoint, projection/chart, edge label/multiplicity/parity, twin/next/previous, star order, boundary-walk member/role/area/face, face witness/equivalence, containment, parent-face/patch mapping, artificial cut, source-side orientation, occurrence map, Euler fact, dependency generation/digest, reconciliation history, and serialization field/order. Every mutation must fail in Release/NDEBUG.

## 11. Test plan

### 11.1 Focused arrangements

- Empty operands; one untouched triangle/convex/concave facet; and boundary-only split points yielding one identity-domain patch.
- Isolated interior/source-edge/source-vertex point contacts and point-only carriers that create local incidence but no edge or fictitious face.
- One open chord, several disjoint chords, crossing constraints, T-junctions, stars of high valence, closed interior loops, nested loops, and disconnected constraint components.
- Constraints ending at original vertices, source-edge interiors, and other constraint interiors; several events at one symbolic vertex; and a carrier through collinear source vertices.
- Equal, partially overlapping, contained, and oppositely directed collinear constraints with repeated labels, mixed intersection/overlap/source ownership, and multiplicities greater than one.
- Concave source polygons; faces with several boundary walks or holes; nested loops; zero-area doubled walks; boundary-attached and floating dangling/slit/tangent constraints; repeated-vertex spike walks; their mandatory canonical cut forests; and positive patches of arbitrarily small exact area.
- Coplanar overlap outer/hole cycles, same/opposite source orientation, overlap boundaries coincident with source boundaries, and several region records sharing atoms.
- Proper missing crossing and missing collinear endpoint fixtures that trigger one successor registry generation, plus duplicate request merging and non-progress rejection.
- Signed zero provenance, subnormal/minimum normal, maximum finite values, one-ULP gaps, cancellation-heavy rational projections, and distinct exact points with equal `T` rounding for all four template specializations.

Every focused case checks point incidences, atom chains, vertex stars, boundary walks/faces, exact area sum, boundary cancellation, Euler facts, source orientation, and independent verifier output.

### 11.2 Differential and metamorphic tests

- Compare producer, mandatory verifier, and independent exhaustive rational oracle over deterministic generated small simple polygons with segment arrangements.
- Permute constraint/event/region/derivation order, reverse insertion order, rotate source and region cycles, and vary provisional segment partition; canonical bytes remain identical for one frozen invocation.
- Vary threads 1/2/many, facet shard/frontier partition, worker delays, hash seed/collision mode, allocation addresses, exact filter path, and cache state; IDs, selected failure, diagnostics, and bytes remain identical.
- Reverse a source facet/ring only through a revalidated fixture and require the documented orientation/halfedge/boundary-walk mapping. Swap operands and require role/provenance mapping with equal unoriented local geometry.
- Subdivide a source edge/facet without changing geometry and compare normalized patch coverage under the documented feature-refinement map. Shared source edges always expose the identical bundle-level shared atom IDs in exact reverse order.
- Split/merge duplicate constraint occurrences and raw derivations without changing normalized multiplicity; geometric DCEL and patches remain equal while provenance mappings change only as specified.
- Compare polygon-with-holes patches against deterministic exact decomposition mode; normalized union area/boundary and semantic constraints are equal.
- Round-trip canonical and invocation encodings; golden bytes freeze empty, untouched facet, split boundary, crossing, closed-loop/hole, overlap-multiplicity, and artificial-diagonal grammars.

### 11.3 Failure and qualification

- Wrong owner, stale/replacement dependency, malformed facet view, off-plane point, broken split reversal, missing region edge, contradictory labels, unregistered crossing without permitted reconciliation, bad construction owner, and every artifact mutation fail closed.
- Exact-at-limit and one-over facet/point/atom/halfedge/walk/face/patch/reconciliation/private-byte/work/exact-number/diagnostic/trace cases; allocation failure and cancellation at every phase expose no partial artifact or stale replacement.
- Debug/Release and GCC/Clang outputs match. ASan/UBSan cover malformed ranges, walk guards, and rollback; TSan covers audit shards, facet workers, cancellation, reconciliation restart, verifier, and publication.
- Benchmark untouched-facet-heavy, split-boundary-heavy, crossing-heavy, collinear-overlap-heavy, high-valence, nested-loop, and coplanar-region cases. Record exact relation counts, points/atoms/halfedges/faces, reconciliation passes, verifier work, peak private bytes, and limb growth. Performance changes may alter only versioned execution policy, never semantics or verification strength.
- Serialize failures with input coordinate bits, exact projected values, source/symbolic/curve/local IDs, constraint keys, dependency generations/policy digests, PRNG state, and expected/actual normalized records. Decimal coordinates alone are insufficient.

## 12. Implementation sequence and completion criteria

Implement in this order:

1. Reconcile Components 1-6 local/shared-reference, per-facet point/segment view, exact-ranking, resource, verifier, and successor-generation interfaces; implement and test Component 1 compare-and-publish plus Component 6 reconciliation before local-refinement production work; freeze Component 7 enums, tags, policies, invariant codes, and encodings.
2. Implement immutable local point/segment constraint, shared/local edge, halfedge, boundary-walk, face, patch, map, and certificate schemas with checked accessors and canonical encode/decode unit tests.
3. Implement single-threaded exact projection, source-boundary reconstruction, per-facet constraint collection, occurrence splitting, collinear normalization, and complete occurrence-chain verification.
4. Implement the exhaustive crossing audit and controlled Component 6 reconciliation/restart path before allowing local arrangement publication.
5. Implement exact vertex-star ordering, paired halfedges, next/previous construction, weak boundary-walk extraction, exact face-equivalence grouping, unique-unbounded-face detection, witness classification, containment, and source-domain face extraction.
6. Implement direct polygon-with-holes patches for Jordan faces, mandatory canonical cut forests for every non-Jordan face, optional further deterministic exact decomposition, artificial-edge provenance, canonical IDs, explicit planar Euler certificates, and identity refinements.
7. Add deterministic facet workers, resource envelopes, cancellation, canonical failure selection, diagnostics, replay, and transaction rollback tests.
8. Implement the independent verifier, canonical encodings/digests, mutation suite, certificate gate, and atomic publication.
9. Run oracle, permutation, orientation/operand mapping, source-subdivision, reconciliation, exact/filter, thread/schedule, adversarial-bit, sanitizer, replay, and benchmark qualification before Component 8 integration.

Component 7 is complete only when:

- every validated source facet has exactly one verified local refinement range, including untouched facets;
- every zero-dimensional facet event has one exact local incidence record even when it creates no edge or area subdivision;
- source boundary chains exactly equal Component 6 split sequences, reference identical bundle-level shared atomic-edge IDs, and reverse exactly across source twins;
- every intersection and overlap constraint is represented by a gap-free atomic chain with complete orientation, multiplicity, ownership, derivation, and region provenance;
- every constraint crossing/overlap endpoint is one globally registered symbolic vertex, with no facet-private identities and no unresolved reconciliation request;
- the DCEL has exact vertex-star order, twin involution, complete weak boundary walks, exact walk-to-face equivalence, one unbounded face, complete incidence, and correctly retained spikes/slits/zero-area structures;
- every non-Jordan face is canonically cut so semantic edge sides occur on simple patch boundaries; positive-area patches have disjoint interiors, exact source-consistent orientation, and closures covering the source facet without gaps;
- exact patch area sum, boundary cancellation, point/segment constraint coverage, source-boundary equality, `V-E+F=1+C`, and `F_bounded=E-V+C` all pass independent recomputation;
- IDs and encodings are independent of constraint order, ring rotation, hashes, partitions, schedules, threads, pointers, approximations, and requested Boolean operation;
- independent oracle/verifier comparison, mutation detection, resource/cancellation/reconciliation rollback, replay, Debug/Release, GCC/Clang, ASan/UBSan, and TSan suites pass;
- Component 8 can stitch every source edge and intersection seam, preserve coincident provenance, and map every local patch/halfedge exactly once using only this immutable artifact and its retained symbolic dependency.
