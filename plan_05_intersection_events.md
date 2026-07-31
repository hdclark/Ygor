# Component 5 implementation plan: exact feature intersection events

## 1. Scope and outcome

Implement the operation-independent narrow phase that consumes Component 4's immutable canonical facet-pair stream and resolves every candidate into an exact source-polygon relation. Publish one deterministic `raw_event_set<T, I>` containing the complete point, interval, coplanar-region, carrier, incidence, orientation, and derivation data required by Component 6. A false-positive broad-phase candidate is a successful disjoint classification. No requested Boolean operation, rounded coordinate, tolerance, triangulation diagonal, or discovery order may affect event geometry.

The completed component must provide:

- exact support-plane classification and source-polygon intersection for every candidate;
- complete noncoplanar carrier interval decomposition, including concave multi-interval cases;
- complete coplanar boundary overlay and positive-area overlap regions;
- source-level point, interval, region, and carrier records with all local derivations and incidences;
- a one-record-per-candidate classification ledger proving complete candidate consumption;
- exact parameters, side/orientation labels, and substitution evidence for every construction;
- deterministic normalization, dense `raw_event_id` assignment, encoding, tracing, and transactional publication;
- deterministic parallel execution, accounting, cancellation, replay, and typed failure integration;
- a mandatory independent verifier and focused, differential, metamorphic, mutation, and adversarial tests.

Component 5 discovers redundant mathematical descriptions deliberately. It merges equal derivations within one source facet pair when the exact relation proves equality, but it does not intern equal entities found through different candidates, assign symbolic identities, close global incidence, or establish global edge/carrier orders; those are Component 6 responsibilities. It does not refine facets, classify volume, apply a Boolean truth table, or realize `T` coordinates.

All implementation must be in-tree, portable C++17, and free of external dependencies. Do not include or call `src/YgorMeshesBoolean{,2,3,4,5}.{h,cc}`.

## 2. Existing Ygor assessment and reuse boundary

### 2.1 Reuse and adapt

- Consume only Component 2's exact source rings, support planes/charts, stable role-qualified feature IDs, ring edge uses, vertex links, certified triangulations, bounds, and immutable topology. Triangles may accelerate a Component 3 relation, but are tagged `internal_nonsemantic` and cannot appear in an event key, incidence, derivation, or encoding.
- Consume Component 3's exact plane relation, plane-plane carrier, line/edge parameter, point/segment/polygon, segment/segment, segment/polygon, and source polygon/polygon relation APIs. Preserve their exact values, construction DAG nodes, predicate evidence, and all source-level relation components rather than rebuilding decisions from approximate values.
- Consume Component 4's canonical `(A facet_id, B facet_id)` records and retain strong ownership of that exact candidate artifact and its validated-operands dependency. Candidate bounds and BVH internals provide no narrow-phase evidence.
- Adapt the source-reference pattern from `WorkingPolygon::refs` and deterministic sweep/angular-order patterns in `src/YgorMathMonotoneDecomposition.cc`, but replace raw indices and floating comparisons with stable `feature_ref`s and Component 3 exact pre-ranks.
- Adapt canonical undirected-pair and ordered-container patterns from `src/YgorMeshesVerification.*` and `src/YgorMathConstrainedDelaunay.cc`. Adapt the useful `source edge -> one construction -> all incident facets` data flow from `fv_surface_mesh::slice_with_planes` in `src/YgorMath.cc`.
- Preserve the right-handed chart convention already frozen by Components 2 and 3: `drop_x:(y,z)`, `drop_y:(z,x)`, and `drop_z:(x,y)`.
- Port geometric fixtures, not implementation, from `tests/Test_MeshesBoolean5.cc` for disjoint, transverse, point-touch, shared-edge, coplanar-overlap, equal, and reversed-equal cases.
- Use Component 1's context, strong IDs, accounting containers, deterministic executor, canonical encoder, diagnostics, trace, verifier registry, replay binding, artifact transaction, and exception conversion. Forward optional post-publication summaries through `YLOGDEBUG`, `YLOGINFO`, or `YLOGWARN` only after deterministic records are frozen.

### 2.2 Reject as production behavior

- Do not call generic `line<T>`, `line_segment<T>`, `plane<T>`, `point_on_*`, `segments_intersect_*`, or `point_in_polygon_or_on_boundary` functions from `YgorMath`; they round, divide, use epsilon, or collapse rich relations to booleans.
- Do not call `YgorMeshesAdaptivePredicates` directly. Component 3 is the sole semantic authority and owns any certified filter/fallback behavior.
- Do not use BSP clipping, constrained Delaunay, monotone decomposition, mesh slicing, fan triangulation, snapping, welding, random perturbation, midpoint fallback, `long double`, or native `T` equality to discover events.
- Do not use legacy Boolean relation enums, rounded intersection points, or triangle-pair records. Legacy files are test-fixture references only and are prohibited dependencies by the broad plan.
- Do not suppress tangent, touching, coincident, or contained relations because a later regularized operation may select no boundary there.
- Do not merge records across candidate pairs, infer event identity from construction syntax/hash alone, or assign Component 6 symbolic IDs.

Components 1-4 and Component 13's verifier registration infrastructure are production prerequisites. Reconcile their planned concrete types before implementation rather than adding parallel context, exact-value, candidate, or artifact systems.

## 3. Files, namespace, and integration

Add:

- `src/YgorMeshesBooleanIntersectionEvents.h`: immutable relation/event schemas, normalized source-incidence and derivation records, artifact constants/read-only accessors, and the stage entry point.
- `src/YgorMeshesBooleanIntersectionEvents.cc`: pair evaluation, source-level extraction, normalization, deterministic merge/encoding, verifier adapter, transaction coordinator, and four `<T, I>` explicit instantiations.
- `tests/Test_MeshesBooleanIntersectionEvents.cc`: normative unit, contract, degeneracy, resource, rollback, and mutation tests.
- `tests/Test_MeshesBooleanIntersectionEventsProperties.cc`: deterministic generated differential, symmetry, subdivision, permutation, sharding, and schedule tests.
- `tests/MeshBooleanIntersectionEventFixtures.h`: test-only exact polygon builders, bit-pattern inputs, independent low-complexity overlay oracle, event normalizer, deterministic PRNG, and replay records.

Use namespace `ygor::mesh_boolean`. Keep provisional event tokens, overlay scratch, worker shards, extraction helpers, and independent verifier helpers private to the `.cc` or tests. Public templates remain thin and use explicit instantiation for `float`/`double` with `std::uint32_t`/`std::uint64_t`.

Add the `.cc` to Component 1's strict-arithmetic source list in `src/CMakeLists.txt`; effective fast-math and contraction must be disabled and compile-time guarded. Add both tests to `tests/compile.sh` and `tests/CMakeLists.txt`, register `MeshBooleanIntersectionEvents.Unit` and `.Properties`, and label them `mesh_boolean;component5`. Add authoritative GCC/Clang Debug/Release CI, ASan/UBSan malformed-record and rollback runs, and TSan shard/merge/cancellation/publication runs. No test may fetch a dependency from the network.

## 4. Stage and ownership contract

Expose a coordinator equivalent to:

```cpp
template<class T, class I>
status_or<std::shared_ptr<const published_artifact<raw_event_set<T, I>>>>
discover_intersection_events(boolean_context<T, I>& context);
```

The coordinator must:

1. Require the exact published `candidate_stream<T, I>` from `artifact_slot::candidate_stream`, its strongly retained `validated_operands<T, I>`, the same owner/setup/kernel policy, and the registered Component 5 verifier specification.
2. Reject a copied or replacement validated artifact even if its digest and feature IDs match. Strong artifact identity and dependency lifetimes are mandatory; digests are serialization bindings, not object identity.
3. Open one `intersection_events` transaction targeting `artifact_slot::raw_event_set`. Build all classifications, events, construction storage, encodings, diagnostics, and verification candidates privately.
4. Validate every candidate ID/key against the retained stores before geometric work. Process each dense candidate exactly once; duplicate, missing, wrong-role, stale, or out-of-range references are upstream/internal invariant failures.
5. Use the context's operation only as invocation metadata. Event policy is frozen and operation-independent; running union, intersection, difference, or symmetric difference over identical validated/candidate artifacts must produce identical source-level event semantics.
6. Normalize records, assign dense IDs, remap references, encode/digest, run mandatory verification, and publish atomically only with a matching certificate and final cancellation check.
7. Return `resource_limit` for declared byte/work/raw-event/exact-number/trace limits or cancellation, and `internal_invariant_error` for ownership, relation, extraction, incidence, encoding, producer, or verifier contradictions. Geometric degeneracy and an empty event set are successful.

The artifact retains typed strong handles to the exact candidate stream, validated operands, and all escaped Component 3 construction storage. It contains no borrowed mesh pointer, hierarchy node, temporary triangulation object, worker-local handle, or approximate coordinate.

## 5. Pair-classification and event schema

### 5.1 Closed enums and candidate ledger

Freeze closed numeric enums for:

- plane relation: `nonparallel`, `parallel_disjoint`, `coincident_same_orientation`, `coincident_opposite_orientation`;
- pair aggregate: `disjoint`, `point_contact`, `curve_contact`, `coplanar_boundary_contact`, `coplanar_positive_area_overlap`, `equal_same_orientation`, and `equal_opposite_orientation`;
- event dimension: `point`, `interval`, `region`;
- point kind: proper transverse endpoint, proper boundary crossing, vertex/vertex, vertex/edge-interior, vertex/facet-interior, edge/edge, edge/facet, tangency, and overlap-boundary vertex;
- interval kind: transverse facet intersection, coincident source-edge subsegment, coplanar overlap-boundary segment, and coplanar interior carrier segment;
- local incidence location: source vertex, directed edge origin/destination/open interior, facet boundary, or facet open interior.

Use richer bit-free enums rather than combining unrelated categories into flags. Unknown values are rejected when decoding.

Publish one `candidate_classification` per dense `candidate_id`, in candidate order, containing the normalized facet key, plane relation/orientation, aggregate relation, canonical ranges of generated raw events and carrier derivations, exact relation-evidence digest, and deterministic semantic counts by dimension/kind. A disjoint candidate has empty event/carrier ranges and exact disjoint evidence; its optional full trace explains the rejection but is observational. This ledger is the mandatory proof that no candidate disappeared, even when trace limits truncate details.

### 5.2 Source incidences and derivations

Define normalized records equivalent to:

```cpp
struct raw_source_incidence {
    feature_ref source;
    local_incidence_location location;
    optional<exact_parameter> directed_edge_parameter;
    optional<exact_sign> local_side;
    orientation_parity orientation;
};

struct raw_derivation {
    construction_node_id construction;
    accounting_vector<feature_ref> defining_sources;
    accounting_vector<raw_source_incidence> incidences;
    predicate_evidence evidence;
};
```

Use the concrete Component 3 construction reference rather than duplicating its DAG schema. Sort/deduplicate source references and incidences by complete semantic content after exact parameters are pre-ranked. Preserve multiple derivations that construct the same exact value unless their full provenance and evidence are equal. Hash equality is only a bucket hint and never a proof.

For a point equal to an original vertex, include that vertex, every source edge use ending there in the candidate facet, the owning undirected edges, the candidate facet, and topology-provided adjacent source facets required by endpoint attribution. For an edge-interior point include both directed edge-use and undirected-edge identity plus its exact parameter and owning facet. Include both operands' facet-open-interior incidences for an interior carrier point. Component 6 may add global closure, but Component 5 may not omit incidence already available from the candidate pair and validated local topology.

### 5.3 Point, interval, and region records

Each `raw_point_event` contains `raw_event_id`, candidate/facet pair, exact point handle/value, point kind, all exact parameters on every incident source edge and noncoplanar carrier, sorted complete incidences, sorted derivations, local crossing/tangent signs, and evidence. Never choose one privileged construction when several were discovered.

Each `raw_interval_event` contains its ID, candidate/facet pair, interval kind, canonical exact line carrier and direction parity, two endpoint point-event IDs, a nonempty closed exact carrier-parameter interval, mapped directed parameter intervals on every coincident source edge, orientation/multiplicity for A and B boundary ownership, incidences/derivations, and evidence. Positive length is mandatory; a collapsed interval is represented only as a point event. Reversing a source edge maps parameters and orientation but not the canonical geometric carrier interval.

Each `raw_region_event` contains its ID, candidate/facet pair, same/opposite support-plane orientation, Component 3's exact planar region value, nonzero exact area, canonical outer/hole cycles, one proven open-interior witness, boundary atomic references to point/interval events, per-atomic-halfedge A/B source ownership/direction multiplicity, face-side provenance, derivations, and evidence. A region may have holes; concave overlap may produce multiple disconnected region events. Region cycles contain only source edges or exact source-edge subsegments, never internal triangulation diagonals.

Store participating nonparallel plane-plane carriers and coplanar support carriers as normalized `raw_carrier_derivation` records keyed by candidate and exact carrier value. Disjoint candidates publish no carrier record. Carriers are raw provenance, not globally interned curves; Component 6 proves cross-candidate equivalence.

All event records retain exact values even when a diagnostic approximation is available. Approximation bits, decimal text, filter path, timings, and worker identity are excluded from semantics.

### 5.4 Intra-pair normalization boundary

Within one candidate, merge exact-equal point components only after comparing exact point values and compatible relation semantics; union all derivations/incidences. Split every positive-length component at every source vertex, edge endpoint, proper crossing, overlap-region boundary vertex, or category transition. Merge adjacent atomic intervals only when exact endpoint equality is proven and doing so loses no source incidence, orientation change, event category, or region boundary vertex.

Do not merge records from different candidates, even if their values and sources overlap. Preserve their separate `candidate_id` provenance for Component 6. Original source vertices encountered by several pairs therefore produce several raw point events which Component 6 maps to one original symbolic vertex.

## 6. Exact pair evaluation

### 6.1 Common preparation

For each candidate in canonical ID order:

1. Load the two exact validated source polygons and verify role, support-plane, ring, chart, bound, edge-use, and construction-owner consistency.
2. Call Component 3's exact plane relation. Record the unperturbed same/opposite orientation result; never invoke symbolic perturbation for event discovery.
3. Evaluate the complete source polygon/polygon relation in exact mode selected by context policy. If a planned Component 3 API omits parameters, all-source incidences, boundary atomic ownership, or disconnected components required here, extend that API and its verifier rather than infer missing data with `T` arithmetic.
4. Reconcile any accelerated triangle results to source polygon features. Assert that no artificial diagonal ID/value occurs in returned components and that relation permutation mappings hold.
5. Convert relation components to provisional records, run pair-local coverage/invariant checks, and freeze the candidate classification only after all records succeed.

### 6.2 Nonparallel planes

Construct one exact canonical plane-plane carrier and verify it lies in both planes by substitution. Independently for each closed source polygon:

1. Intersect every source boundary edge with the carrier in ring order using exact coplanar line/segment relations.
2. Collect point parameters and collinear edge intervals with complete endpoint/edge/facet incidences; pre-rank and merge only exact-equal parameters.
3. Split the carrier at all parameters. Classify each point and each open parameter interval against the polygon using exact boundary tests and rational interior witnesses.
4. Produce a sorted, disjoint union of point and closed interval components for `polygon intersect carrier`; concavity may produce any number of components.
5. Verify endpoints reconstruct to the same exact 3D point through the carrier and every attributed source edge, and verify open intervals have constant classification.

Intersect the A and B component unions exactly. Emit a point event for every zero-dimensional common component and for every endpoint/category transition of a positive-dimensional component. Emit positive-length transverse interval events for common intervals. Classify endpoint interior/boundary/vertex status independently for both polygons to distinguish proper crossings, endpoint touches, vertex-on-face, edge-on-plane, and tangency. The ordered side sequence immediately before/after each event on the canonical carrier supplies exact enter/leave/tangent signs for both oriented facets.

The producer may consume Component 3's already normalized nonparallel polygon relation instead of duplicating this algorithm, but the returned data and producer checks must be observationally equivalent to these steps. The mandatory verifier implements the edge/carrier decomposition independently of producer extraction helpers.

### 6.3 Parallel and coplanar planes

Parallel-disjoint planes produce an exact disjoint classification and no event. Coincident planes use one exact right-handed chart and a source-edge planar overlay:

1. Exhaustively classify every A/B source-edge pair, retaining proper crossings, vertex/edge contacts, vertex coincidences, and collinear overlap intervals with both directed parameters.
2. Split both polygon boundaries at every exact event parameter. Merge exact-equal overlay vertices while retaining all local source derivations and ownership.
3. Construct paired atomic halfedges, sort outgoing directions by Component 3's exact half-plane/angular order, and walk bounded left faces. No `std::sort` comparator performs fallible exact arithmetic; pre-rank points, parameters, carriers, and directions first.
4. Classify a proven exact open witness for every bounded face against both source polygons. Select `inside_A && inside_B` overlap atoms, merge only across atomic edges proven internal, and extract canonical outer/hole cycles.
5. Emit point events for all boundary crossings, vertex-on-edge/vertex coincidences, shared-interval endpoints, and overlap-cycle vertices. Emit coincident-source-edge interval events and distinct overlap-boundary intervals with complete A/B ownership. Emit one region event per connected positive-area overlap component.
6. Derive strict containment, equal same/opposite orientation, partial positive-area overlap, boundary-only contact, and disjoint aggregate classes from exact boundary components and selected regions. A contained polygon still emits its positive-area region and complete source-owned overlap boundary even when no two source boundaries cross.

Verify every overlay atomic edge is covered by its sources, all cycles close without repeated directed halfedges, holes are strictly assigned to one outer cycle, region interiors are disjoint, exact area is positive, and selected atom area equals extracted region area. Same/opposite support-plane and edge-direction provenance must survive normalization for later coincident-sheet selection.

## 7. Canonical IDs, sorting, and encoding

All fallible exact equality/order work occurs before sorting. Pre-rank exact points lexicographically, canonical carriers, carrier parameters, intervals, planar cycles, construction content keys, and evidence values through Component 3. Build nonthrowing complete keys from ranks plus fixed enums and stable source IDs.

Normalize provisional event keys as `(candidate_id, dimension order point/interval/region, exact geometry ranks, relation kind, normalized source-incidence key, normalized derivation key)`. Canonicalize all pair-local references through provisional semantic keys, sort/group equal intra-pair records with proof, then assign one dense invocation-global `raw_event_id` sequence through Component 1. Remap endpoint and region-boundary references only after every target ID exists. Require strict event key order, dense IDs, acyclic point-before-interval-before-region references, and exact candidate ranges. Different dimensions never merge.

Publish deterministic semantic statistics: candidate count; plane/aggregate relation counts; point/interval/region counts by kind; construction/carrier derivation counts; disjoint count; maximum source-ring sizes; and checked presence/value records for potentially overflowing pair/edge-product counts. Worker, cache, filter, timing, and allocation statistics are observational only.

Define `YGBCAN05` as the invocation-independent source-event semantic payload: schema/type versions; provenance-free canonicalized operand digests; Component 4 `YGBCAN04` digest; event-policy/relation-schema versions; candidate ledger with canonical facet identities; carriers; construction DAG semantic/provenance records; and events in dense ID order. Exclude setup owner, raw source ordinals, candidate artifact digest, diagnostics, traces, reports, certificates, approximations, and execution details.

Frame the invocation-bound artifact payload as `YGBRAW05`: schema/type versions; setup digest; exact candidate, validated-artifact, and kernel-policy digests; policy versions; deterministic statistics; length-prefixed `YGBCAN05`; invocation-bound source provenance/evidence; and construction-storage binding. Compute its artifact digest through Component 1's `YGBART01` framing for `raw_event_set`. Empty vectors use canonical zero counts. Golden encodings must freeze field order and distinguish semantic equivalence from invocation binding.

Candidate traversal reversal, worker schedule, and relation filter path must not change either encoding for one frozen invocation. Source permutations/retriangulations that preserve Component 2's canonical source-facet semantics must preserve `YGBCAN05`; invocation-bound bytes may differ with setup/provenance. Swapping operands has a specified field-wise role/orientation mapping, not generally identical bytes.

## 8. Deterministic execution and resources

Partition the dense candidate range into a fixed, versioned frontier independent of thread count. Each candidate has a `canonical_work_key(intersection_events, candidate_id, phase)`; workers write only accounting-backed private shards. Component 1 grants kernel envelopes and output/trace chunks in sorted work-key rounds. A task checkpoints before each candidate, before an exact polygon relation, every 1024 edge-pair/overlay operations, before chunk growth, and after relation extraction. It preserves candidate/component cursors exactly when requesting another fixed chunk.

Before work, checked-arithmetic bounds ring edge products, potential edge intersections, relation components, event references, construction/evidence nodes, sort scratch, and canonical bytes. Component 3 receives its formula/ring-size-derived complete request envelope. After a relation returns, request actual extraction storage in the next deterministic grant round; do not reserve a Cartesian all-candidate maximum. Apply the `raw_events` limit to the final normalized event count, while private-byte/work limits govern redundant provisional discovery. If private storage cannot hold required derivations before normalization, return `resource_limit`, never discard provenance.

Join every task and select failures by Component 1 canonical precedence, not completion time. Merge shards by work key, then globally normalize and assign IDs. Thread count, allocation address, hash seed, cache warmth, and completion delay cannot alter successful semantic bytes or the selected failure for identical frozen policy/limits.

Check cancellation before dependency validation, dispatch, every bounded loop interval, exact fallback, large allocation, normalization, verification, encoding, and publication. Catch `bad_alloc` and unexpected exceptions only at specified boundaries. Every failure contains stage, candidate/facet and narrower source IDs when known, invariant/formula code, exact relation/evidence references, current/requested/limit facts, dependency digests, and replay token. No partial classification/event/construction becomes visible.

## 9. Mandatory verifier

Register a stable Component 5 artifact tag, checker version, and invariant set with Component 1/13. The verifier receives the frozen event artifact, exact candidate and validated dependencies, exact-only kernel services, policy metadata, accounting, and cancellation. It is read-only and does not call producer extraction, merge, or encoding helpers.

Mandatory checks:

1. Validate owner, slot/tag/schema, setup/dependency/kernel digests, strong artifact identities, dense candidate/event IDs, strict key order, ranges, reference direction, and every role-qualified source reference.
2. Require exactly one ledger record for every candidate and no record for a nonexistent candidate. Recompute support-plane relations exact-only.
3. For nonparallel pairs, independently reconstruct both polygon/carrier component unions by exhaustive source-edge relations and open-interval classification, intersect them, and compare all point/interval geometry, categories, parameters, signs, incidences, and derivations.
4. For coplanar pairs, invoke Component 3's exact-only source polygon relation and independently translate its canonical components without producer helpers. On bounded mandatory inputs, also rebuild the exhaustive source-edge overlay; Component 13 may version a certified scalable checker later, but schema v1 does not trust stored regions alone.
5. Substitute every point into all claimed planes, carriers, and source edges; reconstruct every edge parameter; prove interval endpoint/order/domain and positive length; verify all region cycles, ownership, area, witness membership, holes, and disjoint interiors.
6. Reconstruct local topological incidence from validated rings/twins/vertex links and require complete attribution. Reject extra as well as missing feature claims.
7. Prove candidate aggregate classes agree with components; disjoint pairs have no event/carrier; every event belongs to its candidate; artificial triangulation features occur nowhere; and pair-argument reversal maps to the documented relation.
8. Independently normalize construction DAGs and re-encode/compare `YGBCAN05`, `YGBRAW05`, and `YGBART01`. Verify trace/report bindings separately.

Verifier resource exhaustion returns `resource_limit` and prevents publication. Any producer/verifier mismatch is `internal_invariant_error`. Exhaustive verification additionally compares against a separately implemented low-complexity rational 2D arrangement oracle and reruns successful filter-capable relations in forced exact-only mode.

Mutation tests alter every enum, ID/range, point coordinate, parameter, interval endpoint, carrier parity, region cycle/hole/area/witness, incidence, direction multiplicity, derivation child/source/evidence, aggregate count, dependency digest, schema length, and event order. Every mutation must fail in Release/NDEBUG before Component 6 can consume it.

## 10. Test plan

### 10.1 Focused relation matrix

- Empty candidate stream and all-disjoint candidates; nonparallel, parallel-disjoint, coincident-same, and coincident-opposite planes.
- Proper transverse segment, isolated proper point, endpoint touch, vertex/vertex, vertex/edge, vertex/facet, edge/edge, edge-on-opposite-plane, and tangent contacts.
- Concave facets whose common carrier intersection has zero, one, or several disjoint intervals, including alternating boundary/interior runs and collinear carrier edges.
- Coplanar proper crossings, T-junctions, repeated geometric vertex incidences, partial/equal shared edges in same/opposite directions, boundary-only touch, strict containment, equal polygons in same/opposite orientation, partial area overlap, holes, and disconnected overlap regions.
- Source rings with collinear boundary vertices and intersections exactly at category transitions. Artificial triangulation diagonals never appear.
- Signed zero, subnormal/minimum normal, maximum finite values, one-ULP gaps, large exponent differences, cancellation-heavy rational intersections, and distinct exact points rounding to the same `T`, for all four `<T, I>` specializations.

Every expected point is substituted into all defining carriers; every interval's open interior is sampled exactly; every region's exact area and boundary ownership are checked.

### 10.2 Differential and metamorphic tests

- Compare producer output, mandatory verifier reconstruction, and independent low-complexity overlay oracle over deterministic generated convex/concave pairs.
- Swap operands and require exact geometry equality with role, parameter, side, and orientation fields mapped by the frozen symmetry table. Reverse either facet winding only in separately revalidated fixtures and check orientation mapping.
- Reverse candidate traversal, shuffle worker completion, vary threads 1/2/many, hash seed, shard frontier, and exact filter path; require identical canonical bytes for one invocation.
- Rotate source rings, permute raw vertices/facets, and substitute alternate certified source triangulations. When Component 2 canonical source semantics match, require identical `YGBCAN05` and no diagonal event.
- Subdivide a source edge/facet without changing its geometry and compare normalized geometric event coverage after the documented source-feature refinement mapping; new legitimate source incidences may differ, but no component may appear/disappear.
- Split one candidate relation into all component permutations before extraction; normalization must recover identical IDs/bytes.
- Test every exact parameter at `0`, `1`, open interior, shared endpoints, equal intervals, containment, and reversal `t -> 1-t`.

### 10.3 Failure, verification, and performance tests

- Wrong owner, stale/replacement dependency, malformed candidate, missing facet, cross-role ID, invalid construction owner, contradictory relation, and every published mutation fail closed.
- Exact-at-limit and one-over raw-event, private-byte, work, exact-number, diagnostic, and trace cases; allocation failure and cancellation at every phase; transaction rollback exposes no artifact or replay update.
- Debug/Release and GCC/Clang results match. ASan/UBSan cover malformed counts/references and rollback; TSan covers exact service sharing, shard output, cancellation, merge, verifier, and publication.
- Benchmark exact relation, extraction, coplanar overlay, verifier, peak private bytes, event count, and limb growth on separated, transverse, coincident, high-edge-count concave, and overlap-heavy pairs. Performance changes may alter only versioned execution policy, never event semantics or verification strength.
- Serialize generated failures with coordinate bits, exact values, source IDs, candidate, policy/dependency digests, PRNG state, and expected/actual canonical event records; decimal coordinates alone are insufficient.

## 11. Implementation sequence

1. Reconcile Components 1-4 artifact handles, Component 3 source polygon relation payload, Component 2 local incidence accessors, Component 4 candidate semantics, Component 13 registration, and freeze Component 5 enums/tags/invariant codes/encodings.
2. Implement immutable incidence, derivation, candidate-ledger, point/interval/region/carrier schemas and canonical validation/encoding tests.
3. Implement single-threaded exact plane classification and nonparallel edge/carrier decomposition with exhaustive focused tests.
4. Implement coplanar source-edge overlay extraction, region ownership/orientation, containment/equality classification, and independent low-complexity oracle tests.
5. Implement pair-local normalization, construction retention, exact pre-ranking, global sorting, dense IDs, reference remapping, statistics, and trace normalization.
6. Add deterministic executor rounds, resource accounting, cancellation, canonical failure merge, and transaction rollback tests.
7. Implement the independent verifier, canonical encodings/digests, mutation suite, certificate gate, and atomic publication.
8. Run operand-swap, ring/mesh permutation, source-subdivision, retriangulation, forced exact/filter, thread/schedule, adversarial-bit, sanitizer, replay, and benchmark qualification before Component 6 integration.

## 12. Completion criteria

Component 5 is complete only when:

- every dense broad-phase candidate has exactly one exact classification and every false positive is explicitly classified without an event;
- every point and positive-dimensional component of each candidate source-facet intersection is covered by normalized point/interval/region records;
- concave multi-interval, tangent, coincident edge, containment, equality, positive-area overlap, holes, and disconnected regions are ordinary successful cases;
- every endpoint has complete available source-feature incidence, exact parameters, all derivations, side/orientation provenance, and passing substitution evidence;
- no topology or identity depends on `T` arithmetic, epsilon, approximation, internal triangulation, hash iteration, pointer identity, or requested Boolean operation;
- candidate traversal, component order, worker schedule, and thread count cannot alter canonical semantic records or IDs;
- independent exact verification, low-complexity oracle comparison, mutation detection, resource/cancellation rollback, replay, Debug/Release, GCC/Clang, ASan/UBSan, and TSan suites pass;
- Component 6 can consume the artifact solely through immutable source-level records, prove cross-candidate equivalence, close incidence, and establish global exact order without recovering omitted geometry or provenance.
