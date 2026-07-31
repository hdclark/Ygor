# Component 4 implementation plan: conservative broad phase

## 1. Scope and outcome

Implement the operation-independent stage that consumes Component 2's immutable `validated_operands<T, I>` and publishes a deterministic superset of cross-operand facet pairs requiring Component 5 exact intersection classification. Broad-phase rejection is permitted only after exact authoritative bounds, or separately certified enclosing bounds, prove a strict gap. Touching bounds always overlap. A failure to construct or certify an acceleration bound routes the affected facet through exhaustive comparison; it never removes work.

The completed component must provide:

- authoritative closed exact facet bounds and checked inclusive bound operations;
- a private deterministic flat binary BVH over safely bounded facets;
- complete cross-operand BVH traversal and a reusable duplicate-free self-query primitive;
- exhaustive cross/self enumerators retained as verification oracles and small-input paths;
- canonical cross-operand facet-pair keys, sorting, deduplication, and `candidate_id` assignment;
- deterministic worker sharding and merging through Component 1;
- resource, cancellation, tracing, diagnostic, and transactional-publication integration;
- a mandatory independent sweep verifier and exhaustive no-false-negative test mode;
- adversarial, differential, mutation, property, schedule, and performance tests.

The broad phase makes no plane, polygon, intersection, adjacency, or Boolean-operation conclusion. A candidate means only "not rejected by a certified conservative spatial test." Component 5 must exact-classify every published candidate, including false positives. Do not publish edge/facet or lower-dimensional candidate domains in schema version 1: exact source-facet pair coverage is the simple, complete baseline, and Component 5 already has exact source polygon boundaries.

All code must remain in-tree, portable C++17, and free of external dependencies. Do not include or call `src/YgorMeshesBoolean{,2,3,4,5}.{h,cc}`.

## 2. Existing Ygor assessment and reuse boundary

### 2.1 Reuse and adapt

- Preserve the inclusive overlap convention of `index_bbox<T>` in `src/YgorIndex.h/.cc`: boxes sharing a face, edge, or point overlap. Reimplement the operation over Component 3 exact values; do not use `index_bbox<T>` as an authoritative Boolean type.
- Reuse the broad structural idea of `kdtree<T>` in `src/YgorIndexKDTree.{h,cc}`: bulk construction, a flat logical binary hierarchy, subtree bounds, and range pruning. Replace rounded centers, `std::nth_element`, mutable lazy construction, `std::any`, and unaccounted storage.
- Treat the R-tree and octree in `src/YgorIndexRTree.*` and `src/YgorIndexOctree.*` only as range-query test precedents. Their inclusive range searches are useful comparisons for ordinary finite data, but insertion/reinsertion order, midpoint/volume arithmetic, public mutable nodes, and incomplete tie rules do not satisfy this component.
- Consume Component 2's canonical `validated_facet` store, exact coordinate extrema, stable role-qualified `facet_id`s, and immutable artifact ownership. Never rescan raw `fv_surface_mesh`, use `involved_faces`, or index internal triangulation triangles.
- Use Component 3 exact scalar comparison, exact box/interval operations, exact-order pre-ranking, and certified exact-to-`T` enclosing intervals. Exact extrema remain authoritative even when a faster approximate enclosure is available.
- Use Component 1's `boolean_context`, strong IDs, canonical encoder, accounting containers, deterministic executor, cancellation, diagnostics, trace sink, transaction, artifact store, verifier registry, and replay binding. Do not use `work_queue` from `YgorThreadPool.h`; it swallows worker exceptions and has no deterministic status merge.
- Follow the existing explicit specialization matrix: `T` is `float` or `double`, and `I` is `std::uint32_t` or `std::uint64_t`.
- Forward optional post-publication summaries through `YLOGDEBUG`, `YLOGINFO`, or `YLOGWARN` only after deterministic diagnostics are frozen. Never use `YLOGERR` for expected resource or invariant failures.

### 2.2 Reject as production broad-phase behavior

- Do not use `index_bbox<T>` scalar ordering, `volume`, `surface_area`, `center`, or `volume_increase`; these use rounded arithmetic and can overflow or underflow for valid finite input.
- Do not wrap `rtree<T>`, `kdtree<T>`, `octree<T>`, or `cells_index<T>` as the production hierarchy. They lack exact containment proof, stable typed IDs, deterministic canonical output, context accounting, cancellation, dual-tree traversal, and self-query guarantees.
- Do not use BSP partitioning, triangle/AABB SAT from `YgorMeshesTetrahedralize.cc`, epsilon expansion, mesh-wide radii, floating centroids, random insertion, Morton quantization, or grid-cell conversion. None may decide rejection.
- Do not call adaptive orientation predicates to compare scalar endpoints. Bounds require exact scalar order, not determinants.
- Do not filter topologically adjacent self-pairs in the generic query primitive. The caller owns any policy-specific adjacency interpretation.
- Do not serialize hierarchy nodes, pointers, capacities, worker assignments, timings, hash order, or implementation-specific traversal statistics as candidate-stream semantics.

Components 1, 3, 13's verifier registration infrastructure, and Component 2 are production prerequisites. Their currently planned interfaces must be reconciled before implementation rather than duplicated locally.

## 3. Files, namespace, and integration

Add:

- `src/YgorMeshesBooleanBroadPhase.h`: immutable candidate records, canonical pair keys, stable schema constants, deterministic semantic statistics, read-only candidate-stream artifact, stage entry point, and the bounded-feature query contract used by the Component 2 adapter.
- `src/YgorMeshesBooleanBroadPhase.cc`: exact bound validation, private BVH build/traversal, exhaustive paths, self-query, canonical merge, encoding, verifier adapter, and four explicit template instantiations.
- `tests/Test_MeshesBooleanBroadPhase.cc`: normative unit, contract, resource, transaction, and mutation tests.
- `tests/Test_MeshesBooleanBroadPhaseProperties.cc`: deterministic generated differential, monotonicity, permutation, hierarchy-policy, and schedule tests.
- `tests/MeshBooleanBroadPhaseFixtures.h`: test-only exact boxes, bit-pattern constructors, bounded-feature builders, independent sweep/exhaustive oracles, deterministic PRNG, and replay records.

Use namespace `ygor::mesh_boolean`. Keep BVH nodes, traversal tasks, provisional candidates, and split helpers private to the `.cc`; hierarchy layout is replaceable and must not become an installed ABI. Keep public templates thin and explicitly instantiate the four supported `<T, I>` combinations.

Build integration:

- Add `YgorMeshesBooleanBroadPhase.cc` to Component 1's named strict-arithmetic source list in `src/CMakeLists.txt`. Apply the same no-fast-math and contraction-off options as Components 2 and 3; reject effective `__FAST_MATH__`/finite-math assumptions in this translation unit.
- Add both tests to `tests/compile.sh` and Component 1's `tests/CMakeLists.txt`. Register `MeshBooleanBroadPhase.Unit` and `MeshBooleanBroadPhase.Properties`, link only in-tree Ygor plus `Threads::Threads`, and label them `mesh_boolean;component4`.
- Add authoritative GCC/Clang Debug/Release CTest jobs to `.gitlab-ci.yml` without `|| true`. Add ASan/UBSan runs for malformed bounds, accounting, and rollback, and TSan runs for shard merge, cancellation, verification, and publication.
- Do not add a network-fetched testing, geometry, arithmetic, or serialization dependency.

## 4. Public stage and dependency contract

Expose a coordinator equivalent to:

```cpp
template<class T, class I>
status_or<std::shared_ptr<const published_artifact<candidate_stream<T, I>>>>
enumerate_broad_phase_candidates(boolean_context<T, I>& context);
```

The coordinator must:

1. Require the published `validated_operands` artifact from the same owner and setup digest, the checked Component 3 kernel policy, and the exact Component 4 verifier registration/specification.
2. Open one `broad_phase` transaction targeting `artifact_slot::candidate_stream`; the upstream validated artifact remains alive for the transaction and the published candidate artifact lifetime.
3. Read only canonical facet records for operand A and operand B. Reject cross-owner, stale, wrong-role, duplicate-ID, or missing-bound records as an internal upstream invariant defect.
4. Reserve build, task, candidate, sort, encoding, trace, and verifier envelopes through Component 1 in canonical work-key order. Catch boundary exceptions using Component 1's conversion policy.
5. Check cancellation before preparation/build, at fixed intervals during node/pair loops, before every potentially large growth, before merge, before verification, and immediately before publication.
6. Build all nodes and candidate shards privately. No node, provisional pair, candidate ID, statistic, or trace record is visible on failure.
7. Normalize and deduplicate all pair keys, assign IDs, freeze the artifact, encode/digest it, run the registered independent verifier, and publish only with a matching certificate.
8. Return `resource_limit` for configured bytes/work/candidates/trace/cancellation exhaustion and `internal_invariant_error` for violated bound/upstream/producer/verifier assumptions. Geometric degeneracy, empty operands, or no candidates are successful results.

The `candidate_stream` retains the exact owner-bound typed `shared_ptr<const published_artifact<validated_operands<T, I>>>` (or Component 1's equivalent strong typed artifact handle) that it indexes. This reference establishes lifetime and object identity. Retain the upstream digest additionally for canonical serialization and stale-binding detection, but never treat a digest match as an equality or ownership proof. Component 5 must reject a stream paired with any other validated artifact even if its digest or some facet IDs happen to match.

## 5. Authoritative bound model

### 5.1 Closed exact boxes

Use Component 3's exact closed 3D box or define a thin Component 4 record composed exclusively from its canonical exact scalar/interval values:

```cpp
struct exact_feature_bound3 {
    exact_closed_interval x;
    exact_closed_interval y;
    exact_closed_interval z;
};
```

Every interval is nonempty and stores canonical `min <= max`. Zero extent in any or all axes is valid. Bounds are mathematical values: `+0` and `-0` compare equal, subnormals decode exactly, and maximum finite endpoints require no arithmetic widening. Signed-zero source bits remain in Component 2 provenance but do not create distinct boxes.

For each source facet, use Component 2's exact per-axis extrema over every source-ring vertex. Recompute the extrema from the retained exact ring in producer mandatory checks and compare to the stored facet bound before acceleration. Facet interiors lie in the convex hull of their boundary vertices, so coordinate-wise vertex extrema contain the full closed planar polygon; retain this containment rationale as a versioned invariant code.

Node union performs only exact endpoint `min`/`max`. Inclusive overlap is exactly:

```text
not (a.x.max < b.x.min or b.x.max < a.x.min or
     a.y.max < b.y.min or b.y.max < a.y.min or
     a.z.max < b.z.min or b.z.max < a.z.min)
```

Only strict separation rejects. Equality is overlap. Do not express this through negated approximate `<=`, native floating comparison under fast math, or an epsilon.

### 5.2 Certified acceleration enclosures

An optional `certified_enclosing_box<T>` may cache Component 3 outward-rounded intervals. Each endpoint carries proof/policy version and exact-source binding. It may accelerate overlap tests as follows:

1. A strict gap between two certified enclosing intervals proves exact separation.
2. Enclosure overlap or uncertainty falls back to exact bound comparison; it does not itself prove a candidate must be emitted.
3. Unbounded conversion, overflow, unsupported proof policy, stale source binding, or failed containment disables the approximate cache for that feature/node.
4. No ambient rounding mode, cast, `nextafter`, center, extent, volume, or surface area may be used unless Component 3's bit-level API certifies the result.

The initial implementation should use exact endpoints for all authoritative node tests. Add approximate caches only after the exact implementation and differential suite pass; caches may affect work counters but not candidate keys, IDs, errors selected under an unlimited budget, or semantic bytes.

### 5.3 Unsafe-bound fallback

Validated source facets should always have exact extrema. Still define a closed fallback path for a missing, malformed, cross-owner, or uncertifiable optional bound:

- a malformed authoritative exact facet bound is `internal_invariant_error`, because Component 2 certified it;
- failure only in an optional approximate representation disables that representation and keeps exact traversal;
- a generic bounded-feature query whose caller cannot provide a certified exact bound marks that feature `exhaustive_fallback` and compares it against every feature in the opposite set;
- never fabricate infinite exact rationals or silently drop a feature.

Record the stable fallback reason for diagnostics/statistics. Deliberate fault injection must prove every unsafe feature receives complete Cartesian coverage.

## 6. Candidate and artifact schema

### 6.1 Canonical keys and records

Define a role-specific cross key:

```cpp
struct facet_candidate_key {
    facet_id operand_a_facet;
    facet_id operand_b_facet;
};

struct facet_candidate {
    candidate_id id;
    facet_candidate_key key;
};
```

Validate from the referenced stores that the first ID belongs to A and the second to B. The canonical order is `(A facet_id, B facet_id)`. There is one schema-v1 candidate kind, so do not add a redundant enum unless Component 1's tagged record grammar requires it.

The published key set is exactly:

- every safe A/B facet pair whose authoritative closed exact boxes overlap; plus
- every pair involving an `exhaustive_fallback` feature.

This fixed definition is stronger and easier to verify than an unspecified superset. Hierarchy shape may add duplicate provisional discoveries but may not add arbitrary published pairs. Every geometric boundary interaction is present because both closed facets are contained in their boxes.

Sort provisional keys with a nonthrowing comparator over strong IDs, deduplicate adjacent equal keys, enforce the published candidate limit, then assign dense `candidate_id`s in sorted order with Component 1's canonical ID factory. Empty input produces an empty store; no sentinel candidate is created.

### 6.2 Statistics and trace

Publish deterministic semantic statistics derived from inputs/final keys only:

- A/B facet counts;
- safe and fallback facet counts by role/reason;
- Cartesian pair count encoded as `{representable_in_u64, value_if_representable}`; omit the value when checked multiplication overflows rather than saturating or truncating it;
- exact-box-overlap candidate count;
- fallback-added pair count after deduplication;
- final candidate count.

Attach implementation statistics such as node count, depth, node-pair tests, provisional duplicates, shard count, filter use, and timings as observational stage/report data excluded from candidate semantic bytes and artifact digest. They may vary with an explicitly selected hierarchy policy while candidate semantics remain identical. Freeze their field order for deterministic diagnostics within one policy.

Optional per-candidate trace records contain the key, six exact interval comparison outcomes or fallback reason, canonical traversal task key, and source bound references. Normalize traces by `(candidate key, evidence kind, source IDs)` after worker merge. Trace is observational and excluded from semantic artifact bytes, but its own canonical digest and truncation/resource status follow Component 1 policy.

### 6.3 Canonical encoding

Define two deliberately distinct encodings. `YGBCAN04` is the invocation-independent candidate-record payload used to compare broad-phase semantics across hierarchy policies, schedules, and equivalent canonical inputs:

1. schema and artifact type versions;
2. provenance-free canonicalized operand A/B digests from Component 2;
3. bound-semantics version, excluding replaceable hierarchy policy;
4. candidate count and candidates in dense ID order, each encoding `candidate_id`, A `facet_id`, then B `facet_id`.

Frame the complete invocation-bound artifact payload as `YGBBPA04`: schema/type versions; setup digest; exact upstream validated-artifact digest; exact-kernel digest; bound and selected build-policy versions; deterministic semantic statistics including the explicit Cartesian-count presence flag; then the length-prefixed `YGBCAN04` payload. Use Component 1's fixed-width integer and strong-ID encoding. Zero counts use canonical zero `u64`. Exclude hierarchy nodes, approximate boxes, exact-order scratch ranks, capacities, allocators, addresses, worker/task layout, implementation counters, diagnostics, trace payload, reports, and certificates. Compute the artifact digest from `YGBBPA04` through Component 1's standard `YGBART01` framing for `artifact_slot::candidate_stream`.

`YGBCAN04` bytes and candidate IDs must agree across thread counts, hierarchy shapes, and raw-input permutations that preserve Component 2's canonicalized operand digests. `YGBBPA04`, `YGBART01`, replay descriptors, reports, and diagnostics remain invocation-bound and may differ when setup options, raw provenance, or thread policy differ. Tests must compare the correct layer rather than assert impossible invocation-artifact equality.

## 7. Deterministic BVH construction

### 7.1 Private flat representation

Use private accounting-backed arrays with index handles, not pointers:

```cpp
struct bvh_node {
    exact_feature_bound3 bound;
    std::uint64_t begin;
    std::uint64_t count;
    node_index left;
    node_index right;
    bool is_leaf;
};
```

Leaf ranges refer to a private array of `(facet_id, bound, split_ranks)`. A node is either a nonempty leaf with no children or an internal node with exactly two nonempty children. Empty roles have no root. Use checked `uint64_t`/`size_t` conversions and invalid-handle values that cannot alias a valid node.

Freeze schema-v1 build policy metadata: binary fanout, fixed leaf capacity (start with 8 after benchmarking), largest-spread split selection, axis tie order X/Y/Z, lower-median split convention, and complete split-key order. Policy changes are replay-visible implementation metadata but cannot change candidate semantics.

### 7.2 Fallible exact preparation

Exact rational comparison/construction may fail and must not occur inside `std::sort` comparators. Before build:

1. Gather all endpoint values and exact doubled centers `min + max` in facet-ID order through Component 3 requests.
2. Use Component 3's fallible exact pre-ranking to assign dense immutable ranks for endpoint and doubled-center values on each axis; equal mathematical values receive equal ranks.
3. Build nonthrowing complete split keys `(center_rank, min_rank, max_rank, facet_id)` per axis.
4. Derive an axis spread from exact `node.max - node.min`, pre-rank the three values, and choose greatest spread, breaking equality X, then Y, then Z.

If doubled-center or spread construction reaches a configured exact-number/work/byte limit, return `resource_limit`; do not use rounded centers. Unlimited correctness mode must complete because finite dyadic endpoints are closed under these operations. A simpler ID-median split is a valid test policy and fallback only if selected before work by a frozen policy; switching after a resource failure would change deterministic failure precedence.

### 7.3 Recursive build invariants

Start each role's feature array sorted by `facet_id`. For a range:

1. Compute its exact union bound in ID order and verify it contains every member bound.
2. Emit a leaf when count is at most the fixed leaf capacity.
3. Otherwise choose the split axis as above, use in-place `std::sort` over the complete nonthrowing key, and split at the fixed lower median so both children are nonempty. `facet_id` makes the key unique, so stability is unnecessary and no hidden `stable_sort` scratch allocation is permitted.
4. Build children in canonical left-then-right order; parallel child construction may use preassigned deterministic node-index ranges or private subtrees remapped after join, never allocation completion order.
5. Recompute the parent from child bounds and verify equality with the member union.

Identical/coincident bounds still progress because `facet_id` completes the key. Depth is bounded for a median split and checked against count. No recursion may overflow the machine stack: use an explicit accounting-backed build stack or prove and enforce a safe logarithmic depth bound before recursive implementation.

## 8. Candidate enumeration

### 8.1 Cross-operand dual traversal

Traverse the A and B roots with canonical node-pair work items:

1. Test certified approximate separation if enabled, then exact inclusive overlap unless separation was proved.
2. Discard a pair only on an exact/certified strict gap.
3. For two leaves, test each facet bound pair and emit its key exactly when the authoritative boxes overlap.
4. For leaf/internal, descend into the internal node's left then right child.
5. For internal/internal, choose one node to split by larger member count, then greater exact maximum axis spread, then role A before B; enqueue child pairs in canonical node-key order.
6. Charge one node-pair or leaf-pair work unit at the documented point and check cancellation every fixed number of tests.

Node identity for task ordering derives from canonical tree path bits plus role, not allocation index. Encode it as `(role, depth_u64, packed_path_bytes)`, where the root has depth zero and no bytes, left appends bit 0, right appends bit 1, bits are packed most-significant-first, and unused low bits in the last byte must be zero. Check depth increment and byte count before growth. Median depth is logarithmic, but malformed/noncanonical or unrepresentable paths are internal/resource failures rather than truncation. Traversal correctness must not depend on the split-choice heuristic: tests force all four child-product orders and alternate leaf capacities and require the same final keys.

After safe-safe traversal, emit exhaustive pairs for fallback features. Compare every fallback A feature with all B features and every fallback B feature with all safe A features; duplicates are allowed privately and removed globally. Checked multiplication/addition precedes reservation and loop execution.

### 8.2 Reusable self-query

Provide an internal/publicly consumable bounded-feature service that Component 2 may call inside its own validation transaction without creating a `candidate_stream` or `candidate_id`. Its neutral input record is:

```cpp
struct bounded_feature_key {
    context_owner_token owner;
    std::uint32_t caller_domain;
    std::uint64_t canonical_rank;
};

struct bounded_feature_view {
    bounded_feature_key key;
    bound_source source; // exact box, certified enclosure, or exhaustive_fallback
};
```

The caller first fallibly canonicalizes its complete provisional semantic keys, proves them pairwise unique, and assigns dense ranks independent of insertion/raw traversal order. Component 2 uses its Phase-E provisional semantic facet encodings, not final `facet_id`s or raw ordinals. The service validates one owner/domain, unique dense ranks, canonical bounds, and complete rank coverage before build; duplicate keys, conflicting records, or cross-owner inputs are invalid preconditions. All internal comparisons are nonthrowing `(caller_domain, canonical_rank)` comparisons. Returned pairs contain those neutral keys normalized by that order, allowing the caller to map them back while retaining ownership. `bound_source` is a closed tagged variant: a Component 3 exact box is authoritative; a certified enclosure includes its proof/source binding; `exhaustive_fallback` includes a stable reason. An enclosure that cannot prove rejection remains overlapping; only the explicit fallback tag lacks a queryable bound.

For a node queried with itself:

1. visit `(left,left)`;
2. visit `(left,right)` once;
3. visit `(right,right)`;
4. for distinct nodes, use the normal overlap traversal;
5. at leaves reject equal feature keys and normalize each pair as `(min key, max key)`.

Sort/deduplicate before return. A self fallback feature compares against every other feature, never itself. The service performs no mesh adjacency filtering and accepts neutral keys so it does not include Component 2 headers or create a dependency cycle. Component 2 may enable it only behind its required exhaustive differential gate; its initial exhaustive validation remains authoritative.

### 8.3 Exhaustive paths and small inputs

Retain `enumerate_exhaustive_cross` and `enumerate_exhaustive_self` permanently. They iterate canonical IDs, apply the same exact inclusive overlap rule for safe bounds, and apply complete pairing for fallback bounds. They must share value-level bound predicates but not BVH traversal logic.

A fixed, versioned small-input threshold may select exhaustive production enumeration before building either hierarchy. The selected policy is frozen in context/replay metadata. Tests can force BVH, exhaustive, and alternate hierarchy policies; successful semantic bytes must match. Never switch from BVH to exhaustive after partially consuming a finite work budget unless Component 1 defines and versions that deterministic accounting behavior.

### 8.4 Deterministic parallel execution

Use Component 1's deterministic executor:

1. Freeze a schema-v1 frontier target of 64 tasks, independent of available thread count. Starting from the root pair, repeatedly remove the least canonical splittable node pair, perform the same deterministic split rule as serial traversal, insert its children in path-key order, and stop at 64 frontier items or when none can split. The exhaustive small-input path uses one canonical task.
2. Encode each frontier node pair as a `canonical_work_key` containing stage, query kind, role/path keys, and ordinal assigned by sorted key.
3. Use Component 1's deterministic additional-request rounds rather than reserving Cartesian worst-case output. Before first dispatch, each task requests only its checked fixed metadata, a traversal stack bound derived from the two logarithmic subtree depths, one 4096-record candidate chunk, one matching trace chunk when enabled, and 1024 node/leaf work units. A worker stops before exceeding any chunk at a checkpoint containing its immutable traversal stack/cursors and produced-record count; all unfinished tasks then submit the next fixed candidate/trace/work chunk they actually need. The coordinator sorts requests by work key, grants them in that order, and resumes the next round only after every task has stopped or finished. The first denied canonical request is the deterministic failure. A leaf Cartesian loop checkpoints every 1024 tested pairs and before filling a candidate/trace chunk, preserving loop indices exactly. Fallback loops are separate rank-keyed tasks whose canonical cursors partition their specified Cartesian coverage. Fixed chunk sizes and checkpoint intervals are schema-v1 execution-policy constants. Thus separated inputs reserve storage proportional to actual emitted candidates rather than `F_A*F_B`, while finite-limit outcomes remain independent of completion schedule. A task writes only its accounting-backed private shard, and unused grants return as Component 1 specifies.
4. Propagate typed statuses and converted exceptions, join every submitted task, and select failure by Component 1's canonical precedence rather than completion time.
5. Merge successful shards in work-key order, then globally sort/deduplicate. Publication never uses worker order.

Because the frontier and envelopes do not depend on worker count, thread count, completion delays, allocation addresses, and hash seed must not alter successful `YGBCAN04` bytes or candidate IDs. For an identical frozen context/policy and limits, completion order cannot alter diagnostics or selected failure. Separate contexts with different thread/setup options have different invocation-bound bytes and compare only the candidate semantic layer.

## 9. Resource, cancellation, and failure rules

Before allocation or loops, check facet counts, Cartesian pair products, node upper bounds, stack/frontier counts, candidate capacities, canonical-byte sizes, and conversions to `size_t`. A binary tree over `n > 0` leaves has fewer than `2n` nodes; use this checked bound for initial reservation without assuming every facet is a leaf.

Account separately for exact preparation, BVH nodes/features, build/sort scratch, traversal tasks/stacks, provisional candidates, dedup scratch, published records, trace, encoding, producer checks, and verifier scratch. Candidate resource policy is:

- private bytes/work are enforced while discovering provisional pairs;
- the `candidates` limit applies to the deduplicated published count;
- if duplicate-heavy traversal cannot fit private storage before deduplication, return `resource_limit` rather than omit pairs; a later sort-run/spill strategy must remain in-memory/in-tree and be separately specified;
- unlimited correctness mode has no artificial candidate/work cap, but checked host representability and `bad_alloc` remain failures.

Expected failures carry stage `broad_phase`, sorted facet references where applicable, invariant/resource code, current/requested/limit values, bound/build policy versions, upstream artifact digest, and replay token. Approximate-cache failure alone is not an error. A bound contradiction, missing validated facet, incorrect role, impossible node containment, candidate mismatch, or verifier disagreement is `internal_invariant_error` and cancels publication.

## 10. Mandatory independent verifier

Register a stable Component 4 artifact type tag, checker version, and invariant set with Component 13/Component 1. The verifier receives the frozen candidate artifact, immutable validated operands, exact kernel, policy metadata, and accounting/cancellation environment. It is read-only and must not call producer BVH build/traversal helpers.

Mandatory production checks:

1. Validate owner, slot, type/schema versions, setup/upstream/kernel digests, role partitions, dense candidate IDs, strict key order, and every facet reference.
2. Recompute every facet's exact coordinate extrema directly from its exact source ring and prove the stored closed bound contains all boundary vertices; compare extrema and check interval canonical form.
3. Recompute fallback classifications/reasons and semantic statistics without trusting cached counters.
4. Independently reconstruct the expected safe-bound candidate set with this exact sweep-and-prune algorithm. Exclude fallback features from endpoint events. Fallibly pre-rank all safe X endpoints, then sort events by `(x_rank, phase, operand_role, facet_id)`, where `start=0` and `end=1`, so every start at a coordinate is processed before every end there. Maintain A/B active sets ordered by `facet_id`. On a start event, compare its Y/Z closed intervals with every currently active opposite-role feature, emit each overlap, then insert it into its role's active set. On an end event, require and erase it from its role's active set. Thus zero-width intervals are inserted and removed at the same coordinate, intervals ending there remain active for all starts there, and two starts there meet when the later ordered start observes the earlier one. Reject duplicate/missing events or invalid active transitions. Finally add every fallback A/all-B and fallback B/safe-A pair and sort/deduplicate.
5. Compare the reconstructed sorted set exactly with the artifact. Missing and extra keys are producer defects under schema-v1's fixed publication definition.
6. Independently re-encode and compare `YGBCAN04`, then the complete invocation-bound `YGBBPA04`, then recompute and compare its `YGBART01` artifact digest. Verify diagnostics/trace bindings separately when requested; never compare the semantic subrecord directly to the invocation-bound digest.

The sweep implementation may share Component 3 scalar comparison and immutable bound values, but not BVH nodes, split ranks, traversal routines, or producer candidate shards. Production verifier cost is `O((F_A + F_B) log F + K_x)` plus exact Y/Z tests, with all work accounted. A verifier resource failure returns `resource_limit` and prevents publication.

Exhaustive verification/test mode additionally evaluates every A/B facet pair in canonical order and compares the exact-bound result to both sweep and BVH outputs. Before Component 5 production work begins, use Component 3's exact source-polygon/polygon relation directly over every pair in the focused and bounded generated low-complexity corpus and prove every non-disjoint geometric pair is present. Component 5 may later add a second end-to-end event-discovery cross-check, but it is not a prerequisite for Component 4's geometric no-false-negative gate. Verification never requires false-positive elimination by geometric narrow phase.

Mutation tests must corrupt each bound endpoint, fallback flag/reason, semantic count, candidate ID, A/B facet reference, candidate ordering, upstream/kernel digest, schema version, and payload length. Every mutation must fail in Release/NDEBUG before downstream publication.

## 11. Test plan

### 11.1 Bound and unit tests

- Empty/empty, empty/nonempty, one/one, all-separated, all-overlapping, containment, and mixed overlap cases.
- Face-, edge-, and point-touching boxes; one-ULP strict gaps; equal endpoints; zero extent on one, two, and three axes.
- `+0`/`-0`, minimum/maximum subnormal, normal/subnormal boundary neighbors, powers of two, maximum finite positive/negative values, opposite extreme exponents, and huge dynamic ranges for float and double.
- Exact vertex-extrema containment for convex, concave, collinear-boundary, and non-axis-aligned planar facets.
- Identical boxes and identical split keys at large counts; prove median progress, bounded depth, and no candidate loss.
- Missing/invalid optional approximate enclosure disables it; generic unsafe bounds route through exhaustive coverage; malformed authoritative validated bounds fail closed.
- Canonical key role validation, sort, deduplication, dense IDs, empty encoding, golden `YGBCAN04` and `YGBBPA04` bytes, stale upstream digest, strong-handle mismatch despite a copied digest, and owner mismatch.
- Self-query excludes `(f,f)`, emits unordered pairs once, preserves touching pairs, and does not filter adjacency.
- Exact-at-limit and one-over byte/work/candidate/trace limits, checked count overflow, allocation failure at each phase, cancellation checkpoints, rollback, and no partially visible artifact.

### 11.2 Differential and property tests

- Compare forced BVH, mandatory sweep, and exhaustive exact-bound enumerators over deterministic generated boxes and validated meshes; require identical sorted sets.
- Use Component 3 exact polygon relations and assert zero geometric false negatives for a focused matrix of disjoint facets, proper crossings, endpoint/vertex touches, collinear edge overlap, edge-on-face contact, coplanar strict containment, partial positive-area overlap, equal same/opposite orientation facets, transverse and coplanar degeneracies, and concave facets with multiple carrier intervals, plus bounded generated low-complexity facets.
- Expand any one bound endpoint outward and assert no prior candidate involving that feature disappears. Shrinking is not required to preserve candidates.
- Permute raw input vertices/facets/ring rotations before Component 2, then compare `YGBCAN04` candidate semantics whenever canonicalized operand digests are equal; separately require each invocation-bound artifact to bind its own setup/provenance.
- Shuffle feature insertion vectors, traversal child visitation in test policies, and worker completion; vary thread counts 1, 2, and a larger available count; require identical `YGBCAN04` bytes. Require identical diagnostics/failure selection only for repeated execution of the same frozen context policy and limits; verify differing setup policies remain correctly reflected in `YGBBPA04`/replay bytes.
- Force leaf capacities 1, 2, 8, and larger-than-input; force ID-median and exact-spatial split test policies; final keys/IDs remain identical.
- Swap operands and verify the key set maps exactly under role swap. Test repeated coordinates and symmetric meshes without relying on raw ordinals.
- Compare self-query with all `n*(n-1)/2` canonical unordered pairs filtered by exact overlap.
- Generate concentrated, diagonal, nested, long-thin, flat, point, bimodal, all-identical, and one-outlier distributions with a fully specified in-tree PRNG.
- Run all four `<T, I>` specializations and compare float/double cases built from exactly shared values where applicable.

### 11.3 Verification, sanitizer, and performance tests

- Fault-inject every hierarchy producer invariant and every published field; require producer checks or the independent verifier to detect it.
- Run mandatory checks in Debug and Release/NDEBUG. Run ASan/UBSan over malformed counts, stack/frontier limits, exact-rank preparation, and rollback; run TSan over parallel build/traversal, cancellation, merge, verification, and publication.
- Benchmark build time, node-pair tests, exact bound tests, candidate count, duplicate count, peak private bytes, sweep verification, and exhaustive crossover on separated, uniform, clustered, coincident, and adversarial distributions.
- Establish the small-input exhaustive threshold from benchmarks only after correctness tests pass. Performance results may change the versioned policy, never inclusive overlap or candidate semantics.
- Serialize every generated failure with seed/state, source coordinate bits/exact bounds, policy versions, thread count, limits, upstream digest, and expected/actual canonical keys; decimal coordinates alone are insufficient.

## 12. Implementation sequence

1. Reconcile Component 1 candidate/resource/artifact/verifier interfaces, Component 2 facet-bound records, and Component 3 exact box/pre-ranking APIs; freeze Component 4 tags, policy, invariant codes, and encoding.
2. Implement exact closed-bound validation, union, containment, inclusive overlap, fallback classification, canonical encoding, and focused extreme-value tests.
3. Implement exhaustive cross/self enumerators and canonical pair publication first; use them as the permanent oracle.
4. Implement fallible split-value preparation and nonthrowing ranks/keys, then the single-threaded flat BVH with producer containment checks.
5. Implement cross dual traversal, self traversal, fallback merge, sort/dedup, semantic statistics, and trace normalization; differential-test every step against exhaustive enumeration.
6. Add deterministic frontier sharding and Component 1 executor/accounting/cancellation integration; prove schedule and limit determinism.
7. Implement `YGBCAN04`, transaction freeze, verifier adapter, independent sweep reconstruction, certificates, mutation tests, and atomic publication.
8. Integrate the neutral self-query with Component 2 only after its exhaustive differential tests prove identical validation pair coverage/outcomes.
9. Add optional certified `T` interval acceleration only after exact-only production and verifier suites pass; force cache enabled/disabled differential equivalence.
10. Run compiler, configuration, sanitizer, adversarial, replay, and benchmark qualification before Component 5 consumes the artifact.

## 13. Completion criteria

Component 4 is complete only when:

- every accepted validated facet is covered by an authoritative exact closed bound, and every node/fallback rule has executable containment checks;
- touching, zero-extent, signed-zero, subnormal, maximum finite, and huge-range cases cannot be pruned incorrectly;
- the published stream equals the fixed exact-bound-overlap plus fallback definition, is strictly sorted/deduplicated, and has dense stable IDs;
- exhaustive and independent sweep oracles find no missing pair across the complete deterministic corpus;
- exact narrow-phase tests find no geometrically interacting pair absent from the stream;
- self-query never emits self/duplicate pairs and remains policy-neutral for Component 2;
- hierarchy shape, insertion order, allocation, hash state, worker schedule, and thread count cannot alter `YGBCAN04` candidate semantic bytes, while invocation-bound bytes continue to identify their actual setup and upstream artifact;
- resource exhaustion, cancellation, malformed upstream state, and verifier disagreement publish nothing and return deterministic structured failures;
- mandatory verification, mutation detection, rollback, replay, Debug/Release, GCC/Clang, ASan/UBSan, and TSan suites pass;
- Component 5 can consume only this candidate stream plus the bound validated operands and exact-classify every pair without consulting hierarchy internals or approximate decisions.
