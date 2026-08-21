# Component 6 implementation plan: symbolic registry and exact ordering

## 1. Scope and outcome

Implement the operation-independent registry that consumes Component 5's deliberately redundant `raw_event_set<T, I>` and publishes one immutable `symbolic_complex<T, I>`. The stage must prove one canonical identity for every exact point represented by an original vertex or discovered event, intern equivalent carriers and finite curve intervals, close all source incidence, establish exact source-edge/carrier orders, and retain complete raw-to-canonical provenance before Component 7 refines any facet.

The completed component must provide:

- pre-registration and canonical mapping of every Component 2 original vertex, including vertices not touched by a cross-operand event;
- proof-based cross-candidate point, carrier, and interval equivalence independent of construction syntax, hash collisions, and `T` approximations;
- exact-original-vertex anchoring for constructed points at source vertices, while permitting several exact-coincident original vertices to share one symbolic point;
- complete, symmetric source vertex/edge-use/undirected-edge/facet/plane/overlap-region incidence with every closure addition justified by validated topology or raw evidence;
- canonical source-edge split sequences with exact parameters and opposite directed views for twins;
- canonical carrier directions, exact point/interval order, and exact planar angular/radial order records required by later stages;
- deterministic dense `symbolic_vertex_id` and `symbolic_curve_id` assignment after equivalence classes are finalized;
- complete typed mappings for every original vertex, raw point, interval, region, and carrier derivation;
- deterministic encoding, tracing, accounting, cancellation, replay binding, independent verification, and transactional publication.

Component 6 does not discover omitted intersections, refine facets, create local arrangement vertices, classify cells, apply Boolean operations, or realize `T` coordinates. A completeness defect found while registering is an `internal_invariant_error`; it is not repaired locally. Component 7 may request a controlled upstream reconciliation only under its separately specified protocol and may never publish a facet-private duplicate identity.

All implementation must be self-contained in Ygor, portable C++17, and free of external dependencies. Do not include or call `src/YgorMeshesBoolean{,2,3,4,5}.{h,cc}`.

## 2. Existing Ygor assessment and reuse boundary

### 2.1 Reuse and adapt

- Consume Component 1's `boolean_context`, owner tokens, strong IDs, `canonical_id_factory`, accounting containers, deterministic executor, checked arithmetic, cancellation, canonical encoder, diagnostics, trace, verifier registry, artifact transaction, replay binding, and typed failures. Use the three-argument verifier callback with `verification_environment_view` required by Component 2, not Plan 1's original insufficient callback.
- Consume Component 2's immutable exact original coordinates, rings, edge uses, twins, undirected-edge endpoint order, incident facets, vertex links, support planes/charts, and role-qualified feature accessors. Derive no authoritative topology from the borrowed `fv_surface_mesh` or `involved_faces`.
- Consume Component 3's canonical exact values, construction DAG, point/carrier/parameter equality, line parameter extraction, pre-ranking, angular comparison, and `carrier_radial_order_v1`. Preserve exact values and all valid construction/evidence nodes.
- Consume Component 5's point, interval, region, carrier, incidence, derivation, candidate-ledger, and construction-storage records. Retain strong ownership of the exact published raw-event and validated-operands artifacts.
- Adapt only general patterns from existing non-Boolean code: canonical undirected pairs and ordered edge aggregation in `src/YgorMeshesVerification.*`; adjacency traversal in `src/YgorMeshesOrient.cc`; and halfedge/angular-order structure in `src/YgorMathMonotoneDecomposition.cc`. Replace raw indices, mutable graph ownership, and floating comparisons with strong IDs and Component 3 exact ranks.
- Use `YLOGDEBUG`, `YLOGINFO`, or `YLOGWARN` only to forward optional summaries after deterministic records are frozen. Never use `YLOGERR` for an expected or invariant failure.

No currently implemented Ygor type provides the required arbitrary-precision geometry, stable symbolic identity, owner-safe handles, accounted equivalence classes, immutable incidence graph, or canonical replay format. Components 1-5 are planned production prerequisites and must be implemented and passing before Component 6 production integration.

### 2.2 Reject as production behavior

- Do not use `vec2<T>`/`vec3<T>` ordering, `long double`, `atan2`, normalized floating directions, epsilon, snapping, welding, rounded coordinate equality, or approximate bounds to prove identity/order.
- Do not use `bimap`, spatial trees, raw pointers, `std::any`, `std::hash` values, DSU roots, insertion order, worker order, or allocator addresses as canonical identity.
- Do not call generic `YgorMath` point/line/segment/polygon relations or `YgorMeshesAdaptivePredicates`; Component 3 is the sole exact authority.
- Do not use `YgorSerialize`, text archives, native object bytes, `Consistent_Hash_64`, or unwrapped MD5 as canonical serialization.
- Do not merge entities because their incidences or construction expressions look alike, and do not keep exact-equal entities separate because provenance differs.
- Do not discard a derivation, source ownership, tangent/contact relation, or overlap record merely because another derivation already identifies the geometry.

## 3. Files, namespace, and integration

Add:

- `src/YgorMeshesBooleanSymbolicRegistry.h`: closed schemas, immutable symbolic records/mappings, artifact constants, read-only accessors, and stage entry point.
- `src/YgorMeshesBooleanSymbolicRegistry.cc`: provisional equivalence machinery, normalization, carrier atomization, incidence closure, order construction, encoding, coordinator, verifier adapter, and four `<T, I>` explicit instantiations.
- `tests/Test_MeshesBooleanSymbolicRegistry.cc`: focused contract, identity, incidence, ordering, failure, mutation, and round-trip tests.
- `tests/Test_MeshesBooleanSymbolicRegistryProperties.cc`: generated permutation, partition, collision, differential, and metamorphic tests.
- `tests/MeshBooleanSymbolicRegistryFixtures.h`: synthetic validated/raw artifacts, exact construction builders, independent small registry oracle, bit-pattern fixtures, deterministic PRNG, and replay records.

Use namespace `ygor::mesh_boolean`. Keep provisional indices, hash buckets, DSU/sort-and-scan state, closure worklists, worker shards, and verifier scratch private to the `.cc` or tests. Public templates remain thin and use explicit instantiation for `float`/`double` with `std::uint32_t`/`std::uint64_t`.

Add the `.cc` to Component 1's strict-arithmetic source list in `src/CMakeLists.txt`; effective fast-math and contraction must be disabled and compile-time guarded. Add both tests to `tests/compile.sh` and `tests/CMakeLists.txt`; register `MeshBooleanSymbolicRegistry.Unit` and `.Properties` and label them `mesh_boolean;component6`. Add authoritative GCC/Clang Debug/Release, ASan/UBSan malformed-artifact/rollback, and TSan shard/merge/cancellation/publication runs. Tests must not fetch dependencies.

Before coding, reconcile these prerequisites rather than adding parallel facilities:

1. Component 3 must expose fallible batch pre-ranking for exact points, lines/carriers, parameters, intervals, planar directions, and construction-content keys. Comparators used by `std::sort` inspect only resulting immutable ranks/fixed fields and are `noexcept`.
2. Component 5 must expose every interval endpoint, carrier parameter interval, source-edge parameter mapping, region boundary reference, incidence, derivation, and construction owner needed below. A raw interval must be allowed to map to several globally split atomic intervals.
3. Component 1's `symbolic_curve_id` is the schema-v1 ID domain for both tagged canonical carrier records and tagged positive-length atomic interval records. Do not add an untyped carrier index. If implementation experience proves one domain inadequate, amend Component 1 explicitly with a strong carrier ID, resource limit, `feature_ref`, encoding, and tests before Component 6 integration.

## 4. Stage and ownership contract

Expose a coordinator equivalent to:

```cpp
template<class T, class I>
status_or<std::shared_ptr<const published_artifact<symbolic_complex<T, I>>>>
build_symbolic_complex(boolean_context<T, I>& context);
```

The coordinator must:

1. Require the exact published `raw_event_set<T, I>` from `artifact_slot::raw_event_set`, its strongly retained `validated_operands<T, I>`, matching owner/setup/kernel policy, and the registered Component 6 verifier specification.
2. Reject copied or replacement dependencies even when their digests/bytes match; digests bind serialization and do not establish process object identity.
3. Open one `symbolic_registry` transaction targeting `artifact_slot::symbolic_complex`; all classes, stores, mappings, encodings, reports, and diagnostics remain private until publication.
4. Validate dense IDs, role-qualified source references, construction owners, endpoint references, candidate ranges, and region boundary references before registry work.
5. Ignore the requested Boolean operation except as invocation metadata. Identical upstream geometry/events under different operations produce identical symbolic semantics.
6. Normalize, prove equivalence, assign all final IDs, close incidence, encode, verify independently, and publish atomically only after a final cancellation check and matching certificate.
7. Return `resource_limit` for declared bytes/work/entity/exact-number/trace limits, allocation failure, or cancellation; return `internal_invariant_error` for dependency, equality, parameter, incidence, mapping, order, encoding, producer/verifier, or ownership contradictions.

The artifact retains typed strong handles to the exact raw-event and validated-operands artifacts and all escaped Component 3 construction storage. It contains no borrowed input-mesh pointer, worker/provisional index, mutable hash table, DSU parent, temporary rank table, or approximate coordinate.

## 5. Public artifact schema

### 5.1 Closed kinds and symbolic vertices

Freeze closed numeric enums for symbolic curve kind (`carrier`, `atomic_interval`), incidence subject/relation, source location, curve ownership role, order kind, and raw mapping kind. Unknown values are rejected on decode.

Define a `symbolic_vertex` equivalent to:

```cpp
struct symbolic_vertex {
    symbolic_vertex_id id;
    exact_handle<exact_point3> point;
    accounting_vector<original_vertex_id> original_vertices;
    accounting_vector<edge_use_id> edge_uses;
    accounting_vector<undirected_edge_id> undirected_edges;
    accounting_vector<facet_id> facets;
    accounting_vector<support_plane_ref> planes;
    accounting_vector<raw_event_id> overlap_regions;
    accounting_vector<symbolic_curve_id> incident_curves;
    accounting_vector<construction_node_id> constructions;
    accounting_vector<raw_derivation_ref> derivations;
    accounting_vector<raw_event_id> raw_points;
};
```

Use concrete earlier-component types and accounting vectors. `support_plane_ref` is a canonical support-plane value/class plus sorted establishing facets, not a hash-derived ID. Region incidence may retain Component 5's `raw_event_id` because Component 6 does not globally intern positive-area regions; if later stages need region identity, add a strong ID deliberately rather than flattening the event ID.

Vertex identity is exact 3D point equality only. Incidence and provenance are merged payload, not identity separators. A class can contain zero, one, or several original vertices; every exact-equal raw construction joins it. Distinct validated source vertices remain distinct topological records in Component 2 but may map to one symbolic point when cross-operand or otherwise permitted geometry proves coordinate coincidence. Component 2's same-operand contract prevents invalid zero-length/self-contact cases before this stage.

### 5.2 Carriers and atomic intervals

Define `symbolic_curve` as a tagged variant sharing the `symbolic_curve_id` domain:

- A `carrier` record stores one canonical exact line, its Component 3 canonical direction, all raw carrier derivations, source facets/planes, point-only event incidence, and its ordered symbolic vertices/atomic intervals.
- An `atomic_interval` record stores its parent carrier ID, positive closed canonical parameter interval, two distinct endpoint symbolic vertex IDs in canonical carrier order, exact ownership/orientation multiplicities by source edge/facet/operand, overlap-region incidence, relation kinds, all covering raw interval IDs, and derivations/evidence.

Carrier IDs sort before interval IDs. Carrier identity is exact canonical line equality. Atomic interval identity is exact `(carrier, lower parameter, upper parameter)` equality after proving endpoints reconstruct to the registered points. Payload differences merge by normalized union unless the contradiction table below rejects them.

Split each carrier at every registered point that is an endpoint or lies in a covering raw interval. Produce positive-length atomic intervals for covered open runs. Do not merge adjacent atoms across a symbolic vertex, source ownership/multiplicity transition, relation-kind transition, overlap-region boundary, or provenance boundary required by a raw mapping. A carrier may have no finite interval, for example a point-only plane-plane contact, and remains a valid canonical entity.

### 5.3 Edge sequences and order records

Publish one `source_edge_split_sequence` per Component 2 `undirected_edge_id`, including edges with no new event:

```cpp
struct source_edge_split_sequence {
    undirected_edge_id edge;
    original_vertex_id canonical_origin;
    original_vertex_id canonical_destination;
    accounting_vector<symbolic_vertex_id> vertices;
    accounting_vector<exact_parameter> parameters;
};
```

Use the validated edge's ordered endpoints as canonical direction and exact domain `[0,1]`. The first/last vertices map from those original endpoints at parameters `0`/`1`; internal parameters are strictly increasing. Publish directed edge-use views as `(sequence, forward)` or `(sequence, reverse)` so twins necessarily expose identical split points in opposite order; do not duplicate mutable arrays.

For every canonical carrier publish its strict point order and ordered atomic intervals. Publish planar angular/radial order records only for sets explicitly required by event incidence or Component 7: normalize directions, pre-rank exact half-plane/orientation relations, group equal/collinear rays, and use stable feature/curve IDs only after geometric equality is proved. Use `carrier_radial_order_v1` for incident noncoplanar sheets and record unperturbed coincidence groups separately from deterministic ties.

### 5.4 Incidence and mappings

Build one canonical sorted relation table first, then derive all forward/reverse ranges from it. Relations cover symbolic vertex/curve to original vertex, edge use, undirected edge, facet, support-plane class, overlap region, derivation, and each other where defined. Every reverse relation is present exactly once; records cannot disagree because both views derive from the same frozen table.

Publish complete typed mappings:

- `original_vertex_mapping`: every original vertex to exactly one symbolic vertex;
- `raw_point_mapping`: every raw point event to exactly one symbolic vertex;
- `raw_interval_mapping`: every raw interval to a nonempty ordered vector/range of atomic interval IDs, canonical endpoint IDs, and parity relative to carrier direction;
- `raw_region_mapping`: every raw region to ordered canonical boundary vertex/interval cycles preserving outer/hole, direction, ownership, and multiplicity;
- `raw_carrier_mapping`: every raw carrier derivation to exactly one canonical carrier ID.

Each dense raw event appears exactly once in the mapping appropriate to its dimension. A region boundary mapping must have the same cycle cardinality and closure as its raw record after replacement; it does not merge distinct region interiors. Mappings preserve all relation kinds, source ownership, orientation, evidence, and candidate provenance even where several raw records map to one symbol.

## 6. Point equivalence and provenance normalization

Create one provisional point member for every validated original vertex and every raw point event. Raw interval/region endpoints reference those members; a missing endpoint point event is an upstream invariant defect.

Normalize construction DAG references by Component 3 full-content keys in child-before-parent order. Deduplicate only byte/content-equal derivations after exact parameter/value proof; retain different valid derivations of the same point. Construction syntax, preferred formula, source count, or candidate ID never decides point identity.

Use a two-level grouping that cannot miss equality:

1. Compute a canonical value fingerprint/coarse signature from normalized exact rational coordinates. Equal mathematical points must have equal signatures; signed-zero bits, owner, approximation, and provenance are excluded. Hash collisions and deliberately forced constant hashes are supported.
2. Within each bucket, use Component 3 exact point equality to form classes. A private accounting-backed DSU or sort-and-scan may accelerate this, but root/member order is discarded. In exhaustive verification, compare all members or canonical exact encodings independently to prove no equal pair was separated across buckets.

Materialize each class as sorted provisional members, union normalized payload, pre-rank its exact point, and validate every source claim. Assign IDs only after all classes are complete, sorted by exact point rank. Original anchoring means that a raw point exactly at an original point maps to the same class; it does not reuse `original_vertex_id` as a `symbolic_vertex_id` or privilege a discovery record.

Compatibility is explicit:

- Different source features, candidates, derivations, relation kinds, or local incidences are additive and normally compatible.
- Repeated claims on one directed source edge must reconstruct the same exact parameter/location; endpoint categories must agree with `0`/`1`, and interior requires `0<t<1`.
- A point claimed on an edge/facet/plane/carrier must satisfy it exactly. An `open interior` claim conflicting with a proven boundary feature is contradictory.
- Opposite directed twin parameters must map by `t -> 1-t`; orientation parity and side claims must follow the frozen reversal tables.
- An interval endpoint claim must equal the referenced point; a region cycle occurrence must equal the referenced boundary vertex.
- Two exact-different points are never merged even if fingerprints collide, parameters compare equal on unrelated carriers, or every `T` conversion rounds identically.

Reject a contradiction; never choose one claim, erase it, or split an exact-equal class by provenance.

## 7. Incidence closure

Seed incidence from all Component 2 original records and every normalized Component 5 claim, then run a deterministic sorted worklist to a fixed point. Every closure rule has a stable rule/evidence code:

1. Original vertex incidence adds its complete validated outgoing link, owning undirected edges, incident facets, support planes, and shell/operand provenance.
2. Edge-use incidence adds origin/destination as applicable, owning undirected edge, incident facet, ring predecessor/successor endpoint facts, and twin relation.
3. Undirected-edge incidence adds both directed uses and both adjacent facets. A point on its open interior is incident to both edge uses/facets with reversed parameters, while retaining which raw candidate observed it.
4. Facet incidence adds its support-plane class; exact facet-open-interior, boundary-edge, and boundary-vertex categories remain distinct.
5. Interval incidence adds both endpoints, carrier, all covering source edges/facets, and region boundaries. Vertex/curve incidence is symmetric.
6. Region incidence adds every mapped boundary vertex/interval and preserves outer/hole cycle orientation and per-halfedge ownership.
7. Coincident carrier/interval records union all source ownership and plane provenance but do not infer incidence with unrelated collinear features absent exact raw/topology evidence.

Do not globally test each point against every source feature merely to enlarge incidence; complete event attribution is Component 5's obligation, and topology closure adds only logically implied adjacent records. Every addition records its seed or closure rule so the verifier can reconstruct it. The fixed point is finite over upstream IDs and checked against work/entity limits.

## 8. Exact ordering and canonical IDs

All fallible exact equality/comparison completes before any standard sort. Batch pre-rank exact points, lines, parameters, intervals, direction rays, construction keys, and evidence through Component 3. Normalized key comparators contain ranks, fixed enums, stable source IDs, and explicit parity only; they are total and `noexcept`.

For each source edge:

1. Gather its original endpoint classes and every symbolic point with proven incidence on either edge use.
2. Extract the parameter independently from the canonical origin/destination and substitute the point in all coordinates.
3. Require `[0,1]`, pre-rank parameters, and group equal ranks.
4. Merge equal parameters only when exact point equality maps to the same symbolic vertex. Equal parameter with different exact points is `internal_invariant_error`.
5. Require endpoints exactly at `0`/`1`, no other endpoint category, and strict internal increase.
6. Compare the reverse twin view against `1-t` and reversed symbolic IDs.

For each carrier, independently extract every point parameter, require reconstruction, sort strictly by exact rank, and place each atomic interval between its endpoint ranks. Equal carrier parameters with distinct points are invariant defects. Interval interiors on one carrier are disjoint atoms; overlapping raw intervals contribute multiplicity/provenance to the same atoms.

Assign all vertex IDs from sorted completed point classes. Then assign curve IDs from complete keys `(kind, carrier rank, interval endpoint ranks when present, normalized semantic tie fields)`, with carriers before their atoms. Hash bucket order, DSU representative, raw IDs, candidate traversal, source ordinal, partition, thread count, and pointer values cannot affect IDs.

## 9. Canonical encoding and deterministic execution

Define `YGBCAN06` as invocation-independent symbolic semantics: schema/type versions; provenance-free canonicalized operand/event semantic digests; registry/order policy versions; canonical vertices and exact points; carriers/atomic intervals; source-edge sequences; normalized incidence; raw semantic mappings with candidate ordinals replaced by canonical source/event semantics where possible; and deterministic semantic counts. Exclude owner tokens, pointers, setup digest, raw input ordinals, diagnostics, traces, approximations, hash values, worker data, and timings.

Frame invocation-bound payload `YGBSYM06`: schema/type versions; setup digest; exact raw-event/validated/kernel-policy digests and strong dependency bindings; deterministic statistics; length-prefixed `YGBCAN06`; invocation-bound raw IDs/provenance/evidence/mappings; and construction-storage binding. Wrap it with Component 1's `YGBART01` framing for `symbolic_complex`. Empty inputs have canonical zero counts. Round-trip decoding rejects unknown enums/versions, noncanonical rationals, invalid IDs, bad lengths, unsorted vectors, duplicate relations, and trailing bytes.

Partition pre-ranking/bucket validation and carrier processing by fixed versioned canonical work frontiers independent of thread count. Workers write accounting-backed private shards only. Grant work/kernel/storage envelopes in sorted `canonical_work_key(symbolic_registry, domain, rank, phase)` rounds; join all workers and select failures by canonical precedence. The coordinator alone materializes classes, assigns IDs, closes global incidence, encodes, verifies, and publishes.

Before work, use checked arithmetic to bound original/raw member counts, incidence expansion, carrier endpoint references, interval atoms, edge sequences, rank scratch, mapping entries, construction references, and canonical bytes. Apply `symbolic_vertices` to final classes and `symbolic_curves` to both tagged carrier and interval records. Private redundant members/buckets/closure queues use stage-private bytes/work. Never discard provenance to fit a limit.

Check cancellation before dependency validation, each grant/worker phase, every bounded exact-comparison/closure interval, allocation/growth, ID assignment, encoding, verification, and publication. Catch `bad_alloc` and unexpected exceptions only at specified task/stage boundaries, join siblings, choose the canonical failure, and roll back. Failures identify stage, raw/source/symbolic IDs available at that point, rule/formula/invariant code, requested/current/limit facts, dependency digests, and replay token.

## 10. Mandatory independent verifier

Register a stable Component 6 artifact tag, schema/checker versions, and invariant set with Component 1/13. The read-only verifier receives the artifact, exact upstream dependencies, exact-only kernel services, accounting, and cancellation through `verification_environment_view`. It must not call producer bucket, DSU, closure, atomization, ID assignment, or encoding helpers.

Mandatory checks:

1. Validate owner, slot/tag/schema, strong dependency object identities, setup/kernel/dependency digests, dense IDs, strict keys, ranges, enum domains, and all role-qualified references.
2. Independently collect every original/raw point and recompute exact equivalence classes. Prove every member maps once, no published classes share a point, and each class contains all and only exact-equal members.
3. Substitute every class point into every claimed source edge, facet plane, carrier, interval endpoint, and region occurrence; recompute all directed parameters/location categories and reversal mappings.
4. Rebuild canonical carriers from raw derivations, independently split all raw intervals by all covered registered points, and compare every positive atomic interval, ownership/multiplicity transition, endpoint, coverage chain, and raw mapping.
5. Reconstruct every source-edge sequence from validated topology plus raw incidence. Require `0`/`1` endpoints, strict internal increase, equal-parameter consistency, complete membership, and exact twin reversal.
6. Recompute carrier point/interval order and every published planar angular/radial order exact-only, including equality/collinearity groups and stable-ID ties only after geometric equality.
7. Rebuild incidence closure from normalized seeds with an independently written rule engine. Compare the canonical relation table and both forward/reverse views exactly; reject missing and extra relations.
8. Verify every raw point/interval/region/carrier and original vertex mapping exactly once without lost relation kind, provenance, orientation, multiplicity, cycle role, or construction reference.
9. Validate construction DAG owner, acyclicity, child-before-parent normalization, exact result equality, and complete retained distinct derivations.
10. Independently re-encode and compare `YGBCAN06`, `YGBSYM06`, and `YGBART01`; verify statistics, report, trace, and replay bindings separately.

Verifier resource exhaustion prevents publication with `resource_limit`; producer/verifier disagreement is `internal_invariant_error`. Exhaustive mode performs all-pairs point/carrier equality independent of fingerprints and compares bounded cases with a separately implemented rational registry/interval-union oracle.

Mutation tests alter every ID, point coordinate, class member, original anchor, parameter, sequence entry, carrier/direction, interval endpoint/domain, ownership count/parity, incidence/reverse incidence, raw mapping/range/cycle, construction child/source/evidence, dependency digest, statistic, schema length, and serialization order. Every mutation must fail in Release/NDEBUG.

## 11. Test plan

### 11.1 Focused identity and order cases

- Empty validated operands/events, original vertices only, and untouched source edges with endpoint-only sequences.
- One raw point equal to an original vertex; many candidates/construction DAGs deriving one point; and coincident original vertices across operands.
- Distinct exact points that round to the same binary32/binary64 value, signed zero provenance, subnormals, one-ULP gaps, extreme exponents, and cancellation-heavy rational intersections.
- Deliberate constant-hash collisions containing equal and unequal points/carriers; identity must match exact proof only.
- Vertex/vertex, vertex/edge interior, vertex/facet interior, edge/edge, tangency, carrier endpoint, and coplanar overlap-boundary incidence.
- Edge parameters at `0`, `1`, and open interior; repeated equal derivations; reversed twins; collinear source vertices; and injected equal-parameter/different-point contradiction.
- Point-only carriers, equal carriers from different plane pairs, reversed carrier derivations, disjoint/overlapping/contained/equal raw intervals, and intervals globally split by points discovered through other candidates.
- Region outer/hole cycles, shared boundary intervals, same/opposite source ownership, disconnected overlap regions, and multiplicity transitions.
- Exact angular quadrants, opposite/equal/collinear directions, radial coincident sheets, carrier reversal, and plane reversal.

### 11.2 Differential and metamorphic cases

- Compare producer classes, mandatory verifier reconstruction, and independent small exact oracle over deterministic generated point/carrier/interval sets.
- Permute every raw event, candidate component, derivation, incidence, class member, region cycle start, and carrier endpoint input order; canonical artifact bytes remain identical for one frozen invocation.
- Vary threads 1/2/many, shard/frontier partition, worker delays, hash seed/collision mode, allocation addresses, exact filter path, and cache state; IDs, mappings, diagnostics, and bytes remain identical.
- Swap operands and require the documented role/orientation mapping with equal unoriented geometry. Reverse source edge/facet orientation only through revalidated fixtures and check parameter/parity transformations.
- Rotate source rings, permute raw mesh vertices/facets, and retriangulate without changing Component 2 semantics; compare `YGBCAN06`. Source subdivision compares through the documented feature-refinement mapping and must preserve geometric coverage while adding legitimate source identities.
- Duplicate or remove one derivation/incidence in private synthetic input to prove normalized duplicates merge and missing mandatory attribution is detected by the verifier.
- Canonical encode/decode round trips preserve all provenance and IDs; golden bytes freeze empty, one-point, anchored-point, split-edge, and overlap-interval grammars.

### 11.3 Failure and performance qualification

- Wrong owner, stale/replacement dependency, malformed raw ranges/endpoints, cross-role features, cyclic/wrong-owner constructions, contradictory source location, bad twin parameter, and every artifact mutation fail closed.
- Exact-at-limit and one-over symbolic vertex/curve, private/committed byte, work, exact-number, diagnostic, and trace cases; allocation failure and cancellation at every phase expose no partial artifact or replay update.
- Debug/Release and GCC/Clang outputs match. ASan/UBSan cover malformed references/counts and rollback; TSan covers bucket/rank workers, carrier shards, cancellation, verifier, and publication.
- Benchmark original-heavy, duplicate-derivation-heavy, collision-heavy, carrier-heavy, overlap-heavy, and high-valence cases. Record exact comparisons, class/member counts, closure additions, interval atoms, verifier work, peak private bytes, and limb growth. Performance changes may alter only versioned execution policy, never semantics or verification strength.
- Serialize failures with input coordinate bits, exact values, all source/raw IDs, dependency/policy digests, PRNG state, and expected/actual normalized classes/orders. Decimal coordinates alone are insufficient.

## 12. Implementation sequence and completion criteria

Implement in this order:

1. Reconcile Components 1-5 concrete artifact/accessor/pre-ranking/verifier interfaces and freeze Component 6 tags, enums, contradiction tables, closure rules, order policy, and encodings.
2. Implement immutable vertex/curve/incidence/order/mapping schemas with checked accessors and canonical encode/decode unit tests.
3. Implement single-threaded original/raw point collection, construction normalization, proof-based equivalence, payload merge, exact pre-ranking, original anchoring, and vertex ID assignment.
4. Implement carrier interning, global endpoint collection, interval atomization, ownership transitions, curve IDs, and all raw interval/region/carrier mappings.
5. Implement source-edge sequences, twin reversal, carrier/planar/radial orders, and deterministic incidence closure with forward/reverse indices.
6. Add resource envelopes, deterministic workers/merge, cancellation, canonical failure precedence, diagnostics, trace, and rollback tests.
7. Implement independent verifier, canonical encodings/digests, mutation suite, certificate gate, and atomic publication.
8. Run permutation, operand/orientation mapping, subdivision/retriangulation, collision, exact/filter, thread/schedule, adversarial-bit, sanitizer, replay, and benchmark qualification before Component 7 integration.

Component 6 is complete only when:

- every original vertex and raw point maps to exactly one symbolic vertex and there is exactly one class per represented exact mathematical point;
- exact-equal points from different syntax/candidates are merged, exact-different points with equal `T` approximations remain distinct, and original coincidences use the original-anchored class;
- every carrier/positive raw interval is represented by canonical carriers and a complete ordered chain of positive atomic intervals with all ownership/provenance retained;
- every source-edge sequence starts/ends at its original endpoints, is strictly increasing, and exactly reverses across twins;
- incidence closure is complete, justified, symmetric, and contradiction-free; every raw relation maps without semantic loss;
- all exact/angular/radial orders use exact predicates and documented stable ties only after equality/collinearity;
- IDs and encodings are independent of hashes, DSU roots, input permutations, partitions, schedules, threads, pointers, approximations, and Boolean operation;
- independent exact verification, oracle comparison, mutation detection, resource/cancellation rollback, replay, Debug/Release, GCC/Clang, ASan/UBSan, and TSan suites pass;
- Component 7 can build every facet constraint and shared source-boundary chain solely from the immutable symbolic complex without recomputing identity, recovering omitted provenance, or inventing private split points.

## 14. Plan-gap amendment: geometric identity only

`symbolic_vertex_id` denotes one exact geometric point and nothing stronger. Equal symbols may participate in several disconnected surface-sheet germs. Components 8-12 must represent those with separate topological occurrence IDs and must never create adjacency, mate, continuation, classification transition, or output welding solely from symbolic or coordinate equality. Component 6 verifies point equality and incidence provenance but does not group topological occurrences.
