# Plan 06: Broad-Phase Collision Enumeration

## 0. Scope, review status, and non-negotiable constraints

Implement **only Component 06** from `component_06_broad_phase_collision_enumeration.md`.

The stage accepts the immutable `canonical_source_manifolds<T,I>` from Component 05 together with the frozen Component 01 context and Component 03 precision capabilities. It constructs deterministic conservative acceleration data and publishes exactly one immutable:

```cpp
canonical_candidate_stream<T,I>
```

for Component 07.

The semantic output is the complete set of directed canonical-edge/opposite-source-triangle pairs whose Component 03 finite closed conservative bounds are **not definitely separated**. The stage may emit geometric false positives. It must never omit a pair admitted by the bounded candidate contract, invent a narrow-phase fact, or make discovery order semantic.

V1 is fixed as follows:

```text
candidate domain: all_canonical_edges_against_all_opposite_source_triangles_v1
provider:         rank_morton_triangle_aabb_hierarchy_v1
leaf capacity:    8
producer query:   canonical edge against opposite triangle hierarchy
materialization:  count / checked prefix / exact reservation / emit
publication:      complete-key canonicalization followed by independent verification
```

The provider name is intentionally **AABB hierarchy**, not LBVH. The prescribed implementation sorts triangles by a deterministic rank-Morton key and forms fixed leaves followed by adjacent bottom-up binary reduction. It is not a license to substitute a different literature-specific LBVH construction.

The implementation must:

- enumerate every Component 05 canonical undirected edge exactly once through its canonical directed representative;
- include both original source edges and facet-internal triangulation diagonals, with internal diagonals remaining bookkeeping-only;
- execute both directed roles, `A-edge/B-triangle` and `B-edge/A-triangle`;
- use Component 03 finite closed AABBs and definite-separation evidence as the sole geometric pruning authority;
- preserve exact contact and every uncertain comparison;
- build immutable deterministic provider tables privately;
- count before materialization, reserve exact output capacity, and reconcile count and emit passes;
- assign candidate IDs only after complete semantic-key canonicalization;
- preserve runtime owner validation without placing raw runtime owner tokens in semantic identity;
- publish only after a separately implemented verifier reconstructs the provider and candidate set; and
- remain strict portable C++17 with no external dependency.

The implementation must not:

- call, adapt, copy from, or depend on `src/YgorMeshesBoolean{,2,3,4,5}*.{h,cc}`;
- call the public `rtree<T>`, `octree<T>`, `kdtree<T>`, or `cells_index<T>` as the production provider or normative verifier;
- use `index_bbox<T>` as authoritative conservative evidence;
- read mutable caller mesh arrays or reconstruct topology from coordinates;
- index both reciprocal halfedges as separate undirected edges;
- omit facet-internal diagonals under V1;
- infer adjacency, equivalence, provenance, source ownership, or duplicate identity from coordinates, bounds, Morton keys, hashes, pointers, or allocation order;
- expand bounds by caller tolerance as a universal proximity rule;
- use open overlap, unchecked subtraction, raw midpoint/center arithmetic, unqualified `vec3` arithmetic, plane-side tests, SAT, distances, or heuristic thresholds to prune;
- perform edge-triangle intersection, coplanarity, sidedness, tangency, symbolic perturbation, event construction, winding, or Boolean selection;
- place runtime owner-token values in semantic keys, semantic IDs, canonical bytes, semantic digests, or deterministic failure ordering;
- assign candidate IDs from traversal, node, allocation, worker, task, hash, or pointer order;
- truncate a candidate set to satisfy a limit;
- publish a partial hierarchy or candidate stream after failure, cancellation, resource exhaustion, or verifier rejection;
- throw exceptions for expected contract, numerical, resource, cancellation, codec, or verification failures;
- serialize native structs, padding, pointers, `size_t`, implementation-defined enum representations, or unordered iteration;
- use fast-math, unauthorized contraction, `long double`, changed rounding mode, or any external dependency; or
- compile production or normative-test code outside the strict bounded-Boolean C++17 target established by Component 01.

Use Component 01 for owner validation, strong-ID domains, checked arithmetic, typed outcomes, resources, cancellation, deterministic failure arbitration, diagnostics, replay, canonical bytes, SHA-256, transactions, immutable publication, and execution-policy validation. Use Component 03 as the sole authority for finite closed AABBs, scalar total-order keys, conservative union and containment, closed overlap, definite separation, and precision references. Use Component 05 as the sole authority for canonical vertices, edges, halfedges, triangles, source facets, shells, semantic classes, directions, provenance, and owner-free semantic keys.

`tracker.md` records completion of this **planning and independent-review step**, not future implementation completion. Mark Component 06 complete after this reviewed specification and concrete plan are internally consistent, cover the implementation handoff, and align with Components 01, 03, 05, 07, 16, and 17. Section 21 remains the future implementation definition of done.

## 1. Independent review conclusions and required corrections

The specification is already provider-neutral and consistent with the broad plan. The prior concrete plan had a strong conservative-enumeration, resource, verification, and test foundation, but this review identified five integration defects that this revision corrects.

### 1.1 Runtime owner tokens are not semantic candidate identity

Component 05 explicitly separates runtime owner validation from canonical semantic identity. The prior Component 06 plan incorrectly stated that complete edge and triangle keys include an owner.

V1 rule:

- checked handles and runtime artifact wrappers carry or reference Component 01 owner tokens;
- owner validation occurs before any dereference or cross-artifact lookup;
- semantic candidate keys use Component 05 **owner-free** complete semantic keys plus stable operand roles;
- candidate IDs are derived from canonical semantic order, never raw owner-token values;
- canonical semantic bytes, semantic digests, replay-equivalence comparisons, and deterministic primary-failure ordering exclude raw runtime owner tokens; and
- semantically identical invocations under different runtime owner anchors produce identical Component 06 semantic bytes and candidate IDs.

A wrong, stale, or cross-context owner still fails deterministically before use. Excluding its raw value from semantics does not weaken lifetime validation.

### 1.2 The tracker records planning completion

The prior wording tied the tracker checkbox to completion of future implementation and qualification. That contradicts Components 01, 03, 05, and 17 and the existing checked tracker state.

The tracker checkbox is set when the specification and concrete implementation plan have passed this review. Future implementation acceptance remains governed by Section 21 and executable evidence.

### 1.3 The existing Ygor audit must include `kdtree<T>`

The prior plan reviewed `index_bbox`, R-tree, octree, cells index, and tetrahedralization code but omitted `src/YgorIndexKDTree.h/.cc`.

The existing KD-tree is not suitable because it:

- stores `std::any` entries and mutable lazy-build state;
- orders by raw floating AABB centers using `std::nth_element` without a complete canonical tie key;
- uses general-purpose `index_bbox<T>` rather than Component 03 conservative evidence;
- uses pointer-owned recursive nodes and discovery-order query output;
- has no owner/provenance/version/resource/cancellation/transaction/codec/verifier contract; and
- throws on expected invalid input.

Preserve it for existing users. It may be used only as a non-normative benchmark/reference. Do not retrofit it as the bounded provider.

### 1.4 Provider terminology must match the prescribed construction

The prior name `rank_morton_triangle_lbvh_v1` could invite substitution of a Karras-style or radix-tree LBVH whose topology, IDs, and counters differ from this plan. V1 is renamed:

```text
rank_morton_triangle_aabb_hierarchy_v1
```

and its implementation files use `TriangleAABBHierarchy`. Any materially different hierarchy construction requires a new provider/layout version and complete requalification.

### 1.5 Empty behavior and verification completeness must be explicit

For canonical provider bytes and replay:

- always construct one canonical triangle hierarchy artifact per operand;
- a zero-triangle operand has the canonical empty hierarchy;
- a nonempty operand is built normally even when the opposite directed pass is empty.

For every ordinary V1 publication, the independent verifier must perform a complete independent breadth-first traversal and exact candidate-key-set comparison. Sampling may supplement diagnostics or qualification, but may never be the sole completeness evidence for V1 publication.

## 2. Existing Ygor assessment and reuse boundaries

### 2.1 `index_bbox<T>`

`src/YgorIndex.h/.cc` provides useful general-purpose closed AABB operations. Preserve its public behavior, but do not use it as authoritative evidence because it:

- accepts nominal corners and canonicalizes reversed endpoints;
- lacks owner, operand, feature, formula, precision-lineage, and artifact-version binding;
- performs center, area, volume, and difference arithmetic without the Component 03 bounded graph;
- does not distinguish definite separation from uncertainty;
- does not record inherited uncertainty or outward construction evidence; and
- has no resource, replay, transaction, codec, or independent-verification contract.

Component 06 consumes Component 03 `finite_aabb<T>` capabilities directly. A tiny semantics-free closed-axis comparison helper may be generalized only if Component 03 owns and qualifies it; Component 06 must not create a second bound implementation.

### 2.2 `octree<T>`

`src/YgorIndexOctree.h/.cc` uses mutable pointer topology, `std::any`, center/octant calculations, insertion-driven subdivision/root behavior, heuristic limits, exceptions, and discovery-order results. Its structure depends on insertion history and cannot supply canonical identities, immutable layout, proof-producing bounds, deterministic failure, transactional resources, or an independent verifier.

Do not retrofit it. Preserve it for unrelated users and non-normative benchmarks.

### 2.3 `rtree<T>`

`src/YgorIndexRTree.h/.cc` is a mutable insertion/reinsertion R*-tree using raw floating volume and overlap heuristics, parent pointers, `std::any`, incomplete equal-cost tie handling, and exceptions. It has no bounded uncertainty, frozen candidate policy, deterministic layout, canonical codec, transaction, or verifier contract.

Do not retrofit it. Preserve it for unrelated users and non-normative benchmarks.

### 2.4 `kdtree<T>`

`src/YgorIndexKDTree.h/.cc` bulk-builds mutable recursive nodes from pending entries using raw AABB centers and `std::nth_element`. Equal centers lack a complete semantic tie key, lazy build mutates a logically queried object, and query order follows recursive provider discovery. It also depends on `index_bbox<T>`, `std::any`, pointer topology, and exceptions.

Do not use or retrofit it for production or normative verification.

### 2.5 `cells_index<T>`

`src/YgorIndexCells.h/.cc` depends on a caller-selected floating cell size, floating-to-integer cell mapping, `std::unordered_map`, `std::any`, replication behavior, and point-oriented insertion semantics. Signed zero, subnormals, extreme scales, cell-boundary ties, deterministic iteration, resource amplification, and conservative box coverage are not governed by the bounded contracts.

Do not use it. A future grid provider requires a new versioned conservative quantization and replication contract.

### 2.6 Tetrahedralization and generic mesh spatial code

`src/YgorMeshesTetrahedralize.cc` contains triangle/AABB SAT and octree centroid-search code. It mutates/triangulates public meshes, uses nominal arithmetic and heuristic epsilon tests, indexes centroids rather than complete conservative triangle bounds, and performs narrow-phase geometry.

Generic mesh Boolean, slicing, hole, orientation, remeshing, and intersection code likewise lacks the required lineage, bounded evidence, and transactional contract. It may supply fixtures or non-normative benchmark comparisons only.

### 2.7 Extraction gate

Before adding a duplicate low-level utility, inspect whether a small existing helper can be extracted without changing existing public behavior. An extraction is allowed only when it:

- is independent of mutable public index classes and `std::any`;
- accepts Component 03 checked bound views or caller-provided exact keys;
- performs no raw floating center/extent/volume calculation;
- has complete deterministic tie ordering;
- has typed failure and checked arithmetic;
- is free of provider state, pointers, logging, exceptions, and external dependencies;
- can be used without sharing producer and independent-verifier high-level control flow; and
- receives direct strict-target unit tests.

No currently identified public index provider satisfies this gate as a whole. Record implementation-time decisions in `BroadPhaseReuseAudit.md`.

### 2.8 Mandatory reuse

Reuse rather than duplicate:

From Component 01:

- checked count, byte, prefix, range, narrowing, and index arithmetic;
- owner-token validation and strong candidate IDs;
- typed outcomes, errors, deterministic finding arbitration, and emergency failure paths;
- resource reservations/leases and transaction publication;
- cancellation and execution-policy validation;
- canonical bytes, SHA-256, diagnostics, and replay.

From Component 03:

- finite closed AABB records;
- exact scalar bit and finite-total-order keys;
- conservative edge/triangle bound reconstruction;
- checked conservative AABB union and containment;
- closed overlap and definite separation with per-axis evidence;
- precision-ledger references and formula/version validation.

From Component 05:

- canonical edge records and one canonical directed representative per undirected edge;
- reciprocal halfedges and incident triangles;
- canonical source triangles and oriented vertex cycles;
- source-edge versus facet-internal-diagonal classes;
- source-facet, shell, provenance, and geometry-attachment mappings;
- owner-free complete semantic keys and exact-triangulation/source-semantic digests.

Use `vec3<T>` only as a nominal coordinate carrier where a predecessor capability exposes it. Never use its arithmetic as Stage 06 authority.

## 3. Fixed V1 domain and provider

### 3.1 Candidate domain

Freeze:

```text
all_canonical_edges_against_all_opposite_source_triangles_v1
```

For each operand, enumerate every Component 05 `manifold_edge_id` exactly once through its canonical directed representative. Include:

- `canonical_edge_class::source_edge`; and
- `canonical_edge_class::facet_internal_diagonal`.

Query every included edge against potentially overlapping source triangles of the opposite operand. Ordinary cross-operand V1 applies no topological exclusion. Every emitted candidate records:

```text
topological_filter_reason::not_filtered
```

Including internal diagonals guarantees triangle-local discovery for Component 07. They remain bookkeeping witnesses and cannot become original source-feature owners, symbolic owners, public event identities, winding barriers, or retained output features.

A future policy that omits internal diagonals requires a new candidate-domain version, proof of source-facet coverage, and exhaustive oracle qualification. It is not a V1 tuning switch.

### 3.2 Provider

Freeze:

```text
rank_morton_triangle_aabb_hierarchy_v1
```

Build one immutable triangle hierarchy for A and one for B. Edges remain query primitives in Component 05 canonical order:

```text
A canonical edges -> query B triangle hierarchy
B canonical edges -> query A triangle hierarchy
```

The hierarchy is a flat bottom-up binary AABB tree over triangles sorted by a deterministic rank-Morton key. Spatial ordering affects provider structure and performance only. It never establishes candidate semantics or justifies pruning.

### 3.3 Fixed constants and layout

Version and freeze:

- leaf capacity: `8` triangle primitives;
- per-axis key: `(lower_endpoint_total_order_key, upper_endpoint_total_order_key)`;
- equal endpoint pairs share one dense rank;
- rank bit width: checked maximum width required by any axis rank for that operand;
- interleave order: X, Y, Z for each bit position, most-significant bit first;
- final spatial tie key: complete owner-free canonical triangle semantic key;
- leaves: consecutive groups of at most eight sorted primitives;
- internal construction: pair adjacent current-level nodes; carry an odd last node unchanged;
- serialized internal nodes have exactly two children;
- node IDs: leaves first by primitive range, then internal nodes by level and left range;
- empty hierarchy: no nodes and no root;
- singleton nonempty hierarchy: one leaf that is root;
- producer traversal: depth-first, left before right;
- verifier traversal: breadth-first with independently implemented control flow;
- candidate publication order: complete semantic candidate key; and
- pre-dedup diagnostic order: `(candidate_key, witness_key)`.

Any change that affects provider bytes, node IDs, counters, replay, or structural evidence requires a provider/layout version change.

## 4. Files, targets, and capability boundaries

### 4.1 Production files

Add under `src/YgorMeshesBooleanBounded/`:

- `BroadPhaseTypes.h` — closed enums, fixed constants, strong-ID aliases, keys, counters, and typed records.
- `CandidateDomainPolicy.h/.cc` — V1 edge inclusion and directed-role mapping.
- `BroadPhasePrimitiveTables.h/.cc` — immutable edge/triangle records and predecessor validation.
- `RankMortonKey.h/.cc` — endpoint ranking, dense ranks, interleaving, spatial keys, and known-answer helpers.
- `TriangleAABBHierarchy.h/.cc` — immutable flat hierarchy schema, deterministic construction, checked views, and producer structural checks.
- `BroadPhasePreflight.h/.cc` — count/representability/resource preflight and private count plans.
- `BroadPhaseTraverse.h/.cc` — serial count/emit traversal and Component 17-ready edge-range adapters.
- `CandidateCanonicalization.h/.cc` — complete-key validation, sort, duplicate policy, IDs, and partitions.
- `CanonicalCandidateStream.h` — immutable artifact schema and checked downstream views.
- `BroadPhaseBuild.h/.cc` — typed stage orchestration.
- `BroadPhaseCodec.h/.cc` — canonical encode/decode and domain-separated digests.
- `BroadPhaseVerifier.h/.cc` — independent primitive/rank/hierarchy reconstruction, breadth-first traversal, exact set comparison, all-pairs oracle, codec verification, and mutation rejection.
- `BroadPhaseQueries.h` — narrow owner-checked immutable queries for Component 07.
- `BroadPhaseReuseAudit.md` — implementation-time audit of existing Ygor functionality and extraction decisions.

Extend, do not duplicate:

- `ContractVersions.h`;
- Component 01 stage, checkpoint, ID-domain, error-subcode, resource-kind, diagnostic, replay, and transaction registries;
- Component 03 capability declarations only for genuinely missing bound operations; and
- the strict bounded-Boolean target and explicit-instantiation lists.

### 4.2 Tests

Add under `tests/mesh_boolean_bounded/`:

- `TestBroadPhaseDomain.cc`;
- `TestBroadPhaseRankMorton.cc`;
- `TestBroadPhaseHierarchy.cc`;
- `TestBroadPhaseOverlap.cc`;
- `TestBroadPhaseTraversal.cc`;
- `TestBroadPhaseKnownCandidates.cc`;
- `TestBroadPhaseAllPairsOracle.cc`;
- `TestBroadPhaseProvenance.cc`;
- `TestBroadPhaseCanonicalization.cc`;
- `TestBroadPhaseCodecReplay.cc`;
- `TestBroadPhaseAlternativeTriangulation.cc`;
- `TestBroadPhaseMutation.cc`;
- `TestBroadPhaseProperties.cc`;
- `TestBroadPhaseAdversarial.cc`;
- `TestBroadPhaseResourcesCancellation.cc`;
- `TestBroadPhaseStructuralPerformance.cc`;
- `BroadPhaseFixtures.h/.cc`;
- `BroadPhaseExactOracle.h/.cc`; and
- `GoldenBroadPhaseV1.h`.

Register focused CTest cases. Apply the strict floating target to all production and normative-test translation units. Use no downloaded or optional test framework.

### 4.3 Typed entrypoint

Provide an internal entrypoint equivalent to:

```cpp
template <class T, class I>
stage_outcome<canonical_candidate_stream<T,I>>
build_canonical_candidate_stream(
    const boolean_context<T,I>& context,
    const precision_context<T>& precision,
    const canonical_source_manifolds<T,I>& manifolds);
```

The entrypoint must:

- validate all owners, versions, dispositions, and predecessor digests before provider allocation;
- support either or both operands empty;
- construct the canonical A and B hierarchy forms privately;
- execute both directed roles under one transaction;
- select the same primary failure for every qualified schedule;
- join or complete all private work before rollback;
- publish neither hierarchy independently; and
- commit one immutable artifact only after independent verification.

A test-only lower-level entrypoint may build one hierarchy or execute one role. Ordinary pipeline publication remains all-or-nothing.

### 4.4 Component 07 view

Expose a `candidate_stream_view<T,I>` with:

- header/version/digest inspection;
- exact candidate count;
- canonical forward iteration and checked random access;
- deterministic partition ranges;
- candidate key, role, edge ID/class, triangle ID, and overlap witness;
- immutable recovery of canonical edge endpoints, reciprocal halfedges, triangle orientation, source facets, shells, provenance, and precision references; and
- no hidden traversal, allocation, mutation, or lazy cache.

Component 07 cannot request more candidates, rerun the hierarchy, or mutate order.

## 5. Versions, checkpoints, and failures

### 5.1 Version registry

Register explicit nonzero versions for:

- artifact schema and provider;
- candidate-domain policy;
- primitive, axis-key, dense-rank, rank-Morton, hierarchy-node, count-plan, witness, candidate-key, candidate-record, partition, statistics, evidence, codec, replay, and verifier schemas;
- leaf/layout, spatial ordering, node-ID, traversal, candidate-order, duplicate, and encoding policies.

Zero and unknown required versions fail. Reserved fields must be zero. Provider/policy/constants appear in the runtime artifact, canonical semantic bytes where required for interpretation, diagnostics, verification evidence, and replay.

Raw runtime owner-token values are never canonical semantic fields.

### 5.2 Fixed checkpoints

Register stable checkpoints in this order:

1. context capability and strict-environment validation;
2. Component 03/05 owner, version, disposition, and digest validation;
3. provider/domain-policy validation;
4. count, pair-product, index, byte, and work preflight;
5. fixed resource reservation;
6. A edge/triangle primitive construction;
7. B edge/triangle primitive construction;
8. independent bound reconstruction and attachment validation;
9. A endpoint-key generation and dense ranking;
10. A rank-Morton ordering;
11. A leaf/internal hierarchy construction;
12. A producer structural verification;
13. B endpoint-key generation and dense ranking;
14. B rank-Morton ordering;
15. B leaf/internal hierarchy construction;
16. B producer structural verification;
17. A-edge/B-triangle count traversal;
18. B-edge/A-triangle count traversal;
19. checked prefixes and exact candidate/work reservation;
20. A-edge/B-triangle emit traversal;
21. B-edge/A-triangle emit traversal;
22. per-edge count/work reconciliation;
23. candidate/witness validation;
24. complete-key sort and duplicate scan;
25. candidate-ID/ordinal assignment;
26. deterministic partition construction;
27. producer artifact checks;
28. canonical encoding and digest construction;
29. independent primitive/rank/hierarchy reconstruction;
30. independent breadth-first traversal and exact key-set comparison;
31. bounded exhaustive all-pairs verification when required;
32. codec/digest/replay/resource reconciliation; and
33. final cancellation check and transaction commit.

Do not renumber released checkpoints.

### 5.3 Failure subcodes

Allocate a disjoint Component 06 range covering at least:

- unsupported provider, domain policy, schema, layout, codec, verifier, or predecessor formula;
- wrong, stale, cross-context, cross-operand, or cross-domain handle;
- predecessor digest/disposition contradiction;
- malformed directed role or operand mapping;
- entity, pair-product, count, rank, node, height, byte, work, prefix, or index overflow;
- fixed, temporary, persistent, candidate, verifier, diagnostic, replay, or codec resource exhaustion;
- malformed edge/triangle primitive or provenance;
- missing, non-finite, inverted, stale, or non-conservative bound;
- edge/triangle bound inconsistent with bounded endpoints/vertices;
- candidate-domain inclusion mismatch;
- internal diagonal mislabeled or source-feature eligible;
- duplicate canonical edge representative or reciprocal twin emitted twice;
- malformed endpoint key, dense rank, rank-Morton key, or spatial order;
- malformed leaf, child, range, node, root, containment, or acyclicity;
- count traversal limit exceeded;
- emit count/work mismatch;
- missing, malformed, or contradictory overlap witness;
- definitely separated pair emitted;
- non-definitely-separated pair omitted;
- candidate role, family, key, class, policy, provenance, or precision mismatch;
- duplicate candidate key or nonzero V1 duplicate discovery;
- key order, ID, ordinal, or partition mismatch;
- codec tag/length/count/reserved-field/trailing-byte/digest error;
- independent provider reconstruction mismatch;
- independent traversal candidate-set mismatch;
- exhaustive all-pairs false negative;
- producer/verifier counter contradiction;
- resource reconciliation failure;
- cancellation; and
- internal construction invariant failure.

Every error records stable stage/checkpoint, provider/policy versions, directed role, least canonical offending semantic keys, expected/observed values, bound bits and comparison evidence, counters/limits, predecessor digests, and replay payload. Runtime owner validation evidence may be retained diagnostically in non-semantic process-local form, but raw owner values do not participate in canonical error ordering or replay-equivalence bytes.

## 6. Strong IDs and semantic keys

Use Component 01 `candidate_id`. Add distinct strong domains for:

- `broad_phase_edge_primitive_id`;
- `broad_phase_triangle_primitive_id`;
- `broad_phase_node_id`;
- `overlap_witness_id`;
- `candidate_partition_id`; and
- verifier-evidence IDs where necessary.

Do not alias domains to `I`, `size_t`, raw offsets, Component 05 IDs, or each other.

Closed enums use explicit nonzero values, including:

```cpp
enum class directed_candidate_role : std::uint8_t {
    a_edge_b_triangle = 1,
    b_edge_a_triangle = 2
};

enum class broad_phase_relation_family : std::uint8_t {
    canonical_edge_source_triangle = 1
};

enum class topological_filter_reason : std::uint8_t {
    not_filtered = 1,
    same_operand_incident_triangle = 2,
    policy_excluded_internal_diagonal = 3
};

enum class hierarchy_node_kind : std::uint8_t {
    leaf = 1,
    internal = 2
};
```

V1 ordinary candidates use only `not_filtered`.

The complete semantic candidate key is equivalent to:

```text
(
  directed_candidate_role,
  broad_phase_relation_family,
  Component05 owner-free complete edge semantic key,
  Component05 owner-free complete triangle semantic key,
  canonical_edge_class,
  candidate_domain_policy_version
)
```

Stable operand role/namespace is represented by the directed role and predecessor semantic keys. Raw runtime owner tokens, provider version, node ID, traversal order, witness ID, worker/task identity, and spatial key are excluded from semantic candidate identity.

Before duplicate validation, sort by `(candidate_key, overlap_witness_key)`. V1 requires one witness per candidate key. A duplicate run is an invariant failure, not silently collapsed.

## 7. Artifact and record schemas

### 7.1 Runtime artifact wrapper and canonical semantic content

The immutable runtime wrapper may carry a Component 01 owner token for checked access. Canonical semantic content stores:

- operand roles and stable predecessor artifact/digest references;
- Component 06 provider/policy/schema/layout/traversal/codec/verifier/key versions;
- required Component 01/03/04/05 contract versions and formulas;
- source semantic and exact-triangulation digests;
- fixed provider constants;
- edge/triangle counts by operand/class;
- hierarchy node/leaf/root/height counts;
- directed-role candidate counts;
- checked table ranges and byte lengths;
- precision capability/profile references;
- resource/statistics and verification-evidence ranges;
- deterministic partitions;
- replay reference; and
- domain-separated table/artifact digests.

Raw owner-token values, pointers, mutable workspaces, allocator state, caller references, and worker state do not enter semantic canonical bytes.

### 7.2 Edge primitive

For each canonical Component 05 edge store:

- Stage 06 primitive ID/ordinal;
- operand role;
- exact Component 05 edge ID and owner-free complete semantic key;
- `canonical_edge_class`;
- canonical directed representative and reciprocal halfedges;
- endpoint identities and incident triangles;
- source-edge or internal-diagonal provenance;
- incident source facets/shells;
- Component 03 edge AABB attachment and exact canonical endpoint bits;
- endpoint/aggregate precision references;
- V1 inclusion disposition; and
- zero reserved fields.

Runtime access validates owner tokens before resolving referenced records. Edge primitive order follows Component 05 canonical edge order. Each undirected edge appears once.

### 7.3 Triangle primitive

For each source triangle store:

- Stage 06 primitive ID/ordinal;
- operand role;
- exact Component 05 triangle ID and owner-free complete semantic key;
- canonical oriented vertex cycle;
- source facet, shell, group, and exact-triangulation provenance;
- Component 03 triangle AABB attachment and exact endpoint bits;
- vertex/aggregate precision references;
- three axis endpoint-pair keys and dense ranks;
- rank-Morton key and provider-local spatial ordinal; and
- zero reserved fields.

Spatial ordinal is non-semantic.

### 7.4 Hierarchy node

Every node stores:

- strong node ID and dense ordinal;
- operand role;
- node kind;
- conservative Component 03 AABB;
- contiguous spatial primitive range;
- subtree primitive count;
- level-from-leaves and checked height data;
- canonical provider node key;
- leaf or internal payload; and
- zero reserved fields.

Leaves contain `1..8` primitives. Internals contain exactly two distinct backward child IDs. Child ranges are adjacent, nonempty, and exactly cover the parent. Parent bounds are checked Component 03 unions in frozen left/right order.

Node key:

```text
(operand role, first spatial primitive, subtree count,
 node kind, level from leaves, layout version)
```

### 7.5 Count plans, witnesses, candidates, and partitions

For each queried edge, the private count plan records role, edge primitive, node tests, leaf visits, primitive overlap tests, definite prunes, uncertain retentions, candidate count, output prefix, work, and digest contribution.

Each overlap witness records:

- role and schema version;
- edge/triangle primitive and bound references;
- admitting leaf and triangle spatial slot;
- Component 03 per-axis comparison categories;
- uncertainty status;
- formula/provider references;
- `topological_filter_reason::not_filtered`;
- edge count-plan offset; and
- canonical witness key.

It must not contain an intersection point, side sign, residual, contact class, crossing multiplicity, or symbolic decision.

Each candidate stores final ID/ordinal, complete semantic key, role/family, edge and triangle references, class, witness, provenance/precision references, domain/provider versions, optional partition reference, and zero reserved fields.

Partition boundaries depend only on final candidate ordinals and a frozen maximum-records-per-partition profile constant. Thread count cannot change them.

## 8. Preflight and resource model

Before provider construction, validate with Component 01 checked arithmetic:

- `E_A`, `E_B`, `T_A`, and `T_B` in all ID/storage domains;
- `E_A * T_B` and `E_B * T_A` as representable worst-case pair products;
- `ceil(T/8)` leaf counts;
- nonempty node upper bound `2 * leaves - 1`;
- hierarchy height, stacks, verifier queues, ranks, prefixes, and partition counts;
- every count-to-byte conversion; and
- Component 07 candidate-ID capacity.

Representability failure is distinct from caller resource exhaustion.

Account separately for:

- primitive tables;
- axis-sort references, ranks, and spatial keys;
- leaves/internal nodes;
- count plans;
- producer stack and verifier queue work;
- node/primitive comparisons;
- pre-dedup/final candidates and witnesses;
- sorting, partitions, codec, diagnostics, replay, and verifier storage;
- persistent artifact bytes.

Reserve fixed tables/work before construction. Do not reserve quadratic candidate storage eagerly. After count traversal:

1. checked-sum per-edge counts in canonical role/edge order;
2. build exclusive prefixes;
3. validate total count/work limits;
4. compute exact candidate/witness/canonicalization/verifier bytes;
5. reserve transactionally; and
6. only then emit.

Never emit a prefix, lower precision, change provider constants, or switch providers automatically after a reservation failure.

## 9. Bound preparation and conservative semantics

For every edge and triangle, obtain Component 05 geometry attachments and Component 03 AABBs. Validate owner, operand, feature identity, formula version, precision lineage, finite endpoint bits, and predecessor digest.

Reconstruct producer-side:

- edge bound as the Component 03 hull of its two bounded endpoints; and
- triangle bound as the hull of its three bounded vertices.

Compare exact endpoint bits and formula references with the predecessor attachment. A mismatch is a predecessor invariant contradiction; do not choose whichever box is larger.

All bounds are finite and closed. Exact endpoint equality retains. Uncertainty retains. Only `definitely_separated` prunes. Signed zero, subnormal handling, finite total order, and outward construction follow Component 03.

Do not add `options.tolerance` to all axes. If a finite conservative bound or union is unrepresentable, fail with the Component 03 numerical disposition. Never use infinity, clamp, fall back to nominal bounds, or skip a primitive.

## 10. Rank-Morton spatial ordering

For each triangle and axis form:

```text
axis_endpoint_key =
(lower_endpoint_total_order_key, upper_endpoint_total_order_key)
```

Use Component 03 exact scalar-bit/total-order capabilities. Do not compute centers, extents, normalized coordinates, or floating grid cells.

For each operand and axis:

1. sort triangle references by `(axis_endpoint_key, complete triangle semantic key)`;
2. assign equal dense rank to exactly equal endpoint pairs;
3. increment only when the endpoint pair changes; and
4. check every increment and range.

Rank zero is valid. Complete triangle keys make sorting total without forcing distinct ranks for identical bounds.

Interleave rank bits most-significant first in X/Y/Z order using explicit unsigned masks/shifts and canonical word significance. Avoid bitfields and implementation-defined layout.

Final spatial order:

```text
(rank_morton_key,
 x_axis_endpoint_key,
 y_axis_endpoint_key,
 z_axis_endpoint_key,
 complete triangle semantic key)
```

Verify endpoint keys against AABB bits, no-gap dense ranks, interleaving, total order, duplicate freedom, and complete triangle coverage.

## 11. Triangle AABB hierarchy construction

For `T == 0`, construct the canonical empty hierarchy: no primitives in spatial table, no nodes, and no root.

For `T > 0`:

- create leaves over consecutive spatial ranges of at most eight triangles;
- compute each leaf bound with checked Component 03 union in ascending spatial order;
- assign leaf IDs by range order;
- repeatedly pair adjacent current-level nodes;
- create one parent for each pair with exact adjacent child-range union;
- compute parent bound by checked left-then-right union;
- assign parent IDs after all prior nodes in pair order;
- carry an odd final existing node unchanged to the next level; and
- finish when one node remains as root.

Do not serialize unary nodes. Parent IDs exceed child IDs. The final root covers `[0,T)`.

Before traversal, producer checks independently reconstruct dense IDs, leaf coverage/capacity, child direction/distinctness, adjacent ranges, exact unions, root coverage, node keys, node-count bounds, and canonical empty/singleton forms.

## 12. Directed count and emit traversal

Process roles in order:

1. `a_edge_b_triangle`;
2. `b_edge_a_triangle`.

Within each role, query edges in Component 05 canonical order. This controls count-plan order and deterministic failure witnesses, not final candidate IDs.

For each edge:

- if the opposite hierarchy is empty, record zero output;
- otherwise start at root with a private preflighted stack;
- test edge/node bounds through Component 03;
- prune only on definite separation;
- for an internal retained node, push right then left so left is processed first;
- for a retained leaf, test each triangle in ascending spatial slot order;
- emit/count exactly one candidate for overlap or uncertainty; and
- record exact structural counters.

The count pass writes no candidate records. It checks node, primitive, candidate, work, resource, and cancellation limits at deterministic increments.

After checked prefix/reservation, repeat the identical traversal for emission. Each edge writes only into its preassigned range. Reject overflow, underfill, uninitialized slots, count/work/category mismatch, or noncanonical references. The output vector does not grow during emit.

The only geometric prune is Component 03 definite separation of finite closed AABBs.

## 13. Candidate validation, canonicalization, and IDs

Before sort, validate each emitted record:

- role maps operands correctly;
- edge and triangle handles validate against runtime owners and expected namespaces;
- referenced semantic keys reconstruct exactly and exclude raw owner tokens;
- edge class matches Component 05;
- internal-diagonal provenance is bookkeeping-only;
- witness identifies the correct leaf/primitive;
- bounds are not definitely separated;
- filter reason is `not_filtered`; and
- provenance/precision references are total.

Sort by `(candidate_key, witness_key)` with explicit complete comparators. Do not rely on hashes or stable-sort history.

V1 is non-replicating and must discover each semantic key exactly once. Any duplicate run blocks publication. Do not silently choose one witness.

Only after duplicate validation:

- assign dense candidate IDs and ordinals in semantic key order;
- reconstruct each key from referenced semantic records;
- build deterministic partitions; and
- verify that runtime owner anchors do not affect canonical semantic bytes.

## 14. Diagnostics, statistics, codec, and replay

Record deterministic counters for:

- edge counts by operand/class and triangle counts;
- distinct ranks;
- leaves/nodes/root/height;
- count/emit node tests, leaf visits, primitive tests, prunes, overlaps, uncertainties, and candidates by role;
- duplicate discovery;
- final candidates by role/class;
- maximum stack/verifier queue depth;
- reservations, peaks, and reconciliation;
- cancellation progress; and
- canonical codec bytes.

Wall time is optional and non-authoritative.

Use Component 01 `CanonicalBytes`. Encode explicit tags/lengths and exact fixed-width fields. Encode `T` endpoints by Component 03 exact bit policy. Never serialize raw structs, pointers, owner-token values, `std::any`, `size_t`, padding, or container capacity.

Compute domain-separated SHA-256 digests for primitive tables, each hierarchy, candidates/witnesses, partitions, statistics/evidence, and complete semantic artifact.

Decode fail-closed: preflight tags/lengths/counts before allocation; reserve resources; decode privately; validate all runtime ownership through the receiving context; reconstruct semantic keys/hierarchy/candidates; reject nonzero reserved fields and trailing bytes; recompute digests; and publish only after the same independent verifier passes.

Replay reproduces domain/provider versions, fixed constants, predecessor semantic digests, primitive inclusion/order, bound bits/formulas, endpoint ranks/Morton words, hierarchy nodes/ranges/root, count plans/counters, candidates/witnesses/IDs/partitions, bytes/digests, and primary semantic failure. Runtime owner-token values and thread schedule are non-semantic.

## 15. Independent verifier

`BroadPhaseVerifier` consumes immutable Stage 06 records and predecessor capabilities. It must not call producer hierarchy construction, depth-first traversal, count/emit, canonicalization, or producer structural-check helpers as its sole source of truth.

For every ordinary V1 publication it must:

1. independently enumerate Component 05 edges and source triangles and apply the V1 domain;
2. validate runtime owners before access while reconstructing owner-free semantic keys;
3. reconstruct Component 03 bounds and compare primitive records;
4. independently rebuild endpoint keys, dense ranks, rank-Morton words, and spatial order;
5. independently rebuild leaves and bottom-up levels and compare every node/range/child/bound/key/ID/root;
6. traverse each reconstructed opposite hierarchy breadth-first using Component 03 definite separation;
7. collect and sort the complete expected candidate semantic-key set;
8. compare it exactly with the published set;
9. reconstruct every witness, role, class, provenance, ID, order, and partition;
10. re-encode semantic records and compare lengths/digests; and
11. reject unknown, stale, truncated, duplicated, reordered, contradictory, nonzero-reserved, or owner-bearing semantic data.

Complete breadth-first traversal and exact set comparison are mandatory. Sampling is supplemental only.

When pair products are within a frozen exhaustive threshold, or normative tests request it, independently enumerate every directed edge/triangle pair without either hierarchy. Apply only the V1 domain and Component 03 definite separation. Compare exact key sets. Under V1, the emitted set must equal the set of all non-definitely-separated pairs.

Verifier inability due to a hard limit is a typed resource failure, never permission to skip verification.

## 16. Transactionality, cancellation, and deterministic execution

All construction occurs in one Stage 06 transaction. Both hierarchies are private tables inside the final artifact.

Poll cancellation at deterministic safe points during:

- primitive batches;
- endpoint-key sorts/rank scans;
- spatial ordering;
- leaf/internal levels;
- producer checks;
- each edge and bounded node-test interval in count/emit traversal;
- prefix/reservation;
- candidate sorting and ID/partition assignment;
- codec batches;
- verifier reconstruction/traversal; and
- all-pairs batches.

On cancellation, all qualified tasks join, unpublished state is destroyed, leases return, and the typed result is `cancelled`. No partial stream is ordinary success.

Implement the executable serial semantic reference first. Component 17 may partition canonical edge ranges. Each task reads immutable inputs and writes private count plans or disjoint preassigned emit ranges. Tasks cannot assign final IDs or mutate shared traversal state. Canonical merge, counters, bytes, digests, and primary failures must reproduce serial semantics.

## 17. Explicit edge-case behavior

- **Both operands empty:** construct two canonical empty hierarchies and publish an empty candidate stream with complete metadata/evidence.
- **One operand empty:** construct the canonical empty hierarchy for that operand and the ordinary hierarchy for the nonempty operand; both directed candidate sets are empty because each role lacks either query edges or opposite triangles.
- **No edges or no opposite triangles:** the corresponding role is empty without fabricated candidates.
- **Singleton triangle hierarchy:** one leaf is root; no internal node.
- **Disjoint root bounds:** each query edge prunes at root when definitely separated.
- **Exact-bound contact:** retain.
- **Nominal gap covered by uncertainty:** retain.
- **Nested solids without boundary contact:** stream may be empty; containment is Component 09 responsibility.
- **Coincident features/bounds:** retain every identity-distinct required pair.
- **Dense all-overlap:** quadratic output is valid; fail deterministically if the true required set exceeds resources.
- **Coordinate-coincident topologically distinct features:** remain separate semantic keys.
- **Non-finite/unrepresentable bounds:** typed failure; never clamp or skip.
- **Signed zero, subnormal, and extreme finite scale:** follow Component 03 qualified semantics.

## 18. Test and qualification plan

### 18.1 Bound and overlap unit tests

Cover ordinary/zero-extent edge and triangle AABBs, exact contact, one-ULP gaps/overlaps, signed zero, subnormals, adjacent large values, maximum finite values, large translations with small geometry, inherited uncertainty larger than nominal gap, and union expansion boundaries.

For every case test definite separation, retained overlap, uncertainty, non-finite rejection, and nonrepresentable union.

### 18.2 Rank-Morton known answers

Commit exact endpoint keys, dense ranks, active width, multiword interleaving, spatial order, leaves, nodes, and root for:

- empty/singleton operands;
- equal bounds;
- monotone movement along each axis;
- axis ties resolved by complete triangle semantic key;
- signed-zero pairs;
- subnormal/normal boundaries;
- extreme finite keys;
- duplicate coordinates with distinct identities; and
- canonical-order permutations.

Use an independently implemented slow bit-by-bit interleaving oracle.

### 18.3 Hierarchy and traversal tests

Verify leaf coverage/capacity, odd carries, child-before-parent IDs, exact adjacent ranges, union order, containment, root coverage, node-count bounds, empty/singleton forms, root prune, partial internal prune, mixed leaves, all uncertainty, stack bounds, exact prefixes, disjoint emit ranges, and count/emit mismatch detection.

### 18.4 Known candidate fixtures

Commit complete expected keys for:

- empty/empty and empty/nonempty;
- disjoint tetrahedra;
- nested boxes without boundary interaction;
- one transverse triangle pair;
- vertex-, edge-, and face-touching boxes;
- equal boxes;
- thin crossing slivers;
- disconnected shells;
- coordinate-coincident topologically separate shells; and
- concave source polygons with internal diagonals.

Expected sets distinguish both directed roles and edge classes.

### 18.5 Exhaustive all-pairs oracle

For every bounded fixture and generated small manifold pair:

1. enumerate all canonical edges, including internal diagonals, exactly once;
2. enumerate every opposite source triangle in both roles;
3. reconstruct Component 03 bounds independently;
4. apply definite separation only; and
5. compare complete expected/emitted semantic-key sets.

Any missing expected key is release-blocking. Run across `float`, `double`, `uint32_t`, `uint64_t`, retained provider/domain versions, and qualified execution modes.

### 18.6 Provenance and integration tests

Verify:

- reciprocal halfedges produce one edge primitive;
- source-edge and internal-diagonal payloads remain distinct;
- internal diagonals are non-source/non-symbolic/non-barrier;
- source-facet/shell/triangle provenance is complete;
- equal numeric IDs in different operand namespaces remain distinct;
- alternative legal source-facet triangulations may change bookkeeping candidates but not later canonical source relations or Boolean results;
- Component 07 consumes candidates without rerunning broad phase; and
- changing only runtime owner anchors leaves candidate semantic keys, IDs, canonical bytes, and semantic digests unchanged while wrong-owner access still fails.

### 18.7 Determinism and metamorphic tests

Apply vertex/facet/shell/component permutations, canonical input reversal, build-order permutations, axis ties, exact translations, power-of-two scaling, axis permutations, operand exchange with role remap, thread counts 1/2/maximum, forced task delays/reversed completion, and repeated execution.

For a fixed provider/domain version, semantic primitive tables, hierarchies, candidate keys/IDs/order, counters, partitions, digests, and primary failures must be byte-identical. Process-local owner tokens may differ but cannot affect semantic evidence.

### 18.8 Mutation tests

Mutate valid artifacts by deleting/duplicating primitives, replacing an edge by its twin, changing class/provenance, inserting owner values into semantic keys/bytes, shrinking or altering bounds, corrupting endpoint keys/ranks/Morton words/spatial order, changing leaf ranges/capacity, children/ranges/order/root, creating cycles/self/forward links, changing count plans/prefixes, omitting/adding/duplicating/reversing candidates, forging witnesses/filter reasons, scrambling IDs/partitions/counters/resources, corrupting versions/reserved fields/codec/digests, and substituting stale predecessor digests.

The independent verifier must reject every mutation; the all-pairs oracle must detect every omission.

### 18.9 Fuzzing and shrinking

Generate valid exact-template manifold pairs varying shell nesting, triangle count/aspect ratio, concave facets/triangulations, clustering/separation, coordinate duplication without identity merge, ULP perturbations, uncertainty, translations/scales, disconnected components, dense overlap, and resource limits.

Serialize exact input bits, semantic predecessor digests, versions, counters, and replay on failure. Shrink while preserving failure category.

### 18.10 Resources and cancellation

For every resource class test limit-minus-one, limit, and limit-plus-one. Cancel during every checkpoint class, both directed count/emit passes, codec, verifier traversal, and all-pairs oracle. Confirm tasks join, leases reconcile, private state is destroyed, no partial artifact is visible, and retry produces canonical semantic bytes.

### 18.11 Structural performance

Use deterministic counters, not wall time alone, to gate:

- rank/key construction and hierarchy build within documented `O(T log T)` comparison growth;
- exact linear node-count bounds;
- constant root work per queried edge for clearly separated root bounds;
- subquadratic primitive tests for large disjoint/localized fixtures;
- no full pair product for ordinary sparse scenes;
- output-sensitive growth with overlap density; and
- memory proportional to primitives, nodes, count plans, and final candidates.

Worst-case quadratic output is acceptable when conservative bounds genuinely admit it. It must be visible and controlled by typed limits.

## 19. Implementation sequence and handoff gates

Implement in this order:

1. register versions, enums, IDs, checkpoints, subcodes, resource kinds, and strict-target files;
2. add `BroadPhaseReuseAudit.md` and close every existing-index audit item;
3. define owner-free semantic keys, runtime owner-validation wrappers, schemas, and comparators;
4. implement/test V1 domain policy;
5. implement Component 03/05 narrow adapters and bound reconstruction tests;
6. implement endpoint keys, dense ranks, interleaving, and known answers;
7. implement spatial ordering and primitive verification;
8. implement triangle AABB hierarchy and golden tests;
9. implement producer structural checks;
10. implement serial count traversal and overlap tests;
11. implement checked prefixes and exact reservation;
12. implement serial emit and count/emit reconciliation;
13. implement candidate validation, duplicate rejection, IDs, and partitions;
14. implement immutable Component 07 views;
15. implement codec, semantic digests, and replay;
16. implement independent primitive/rank/hierarchy reconstruction;
17. implement mandatory independent breadth-first traversal and exact set comparison;
18. implement bounded exhaustive all-pairs oracle;
19. add mutation, provenance, alternative-triangulation, owner-anchor, adversarial, resource, cancellation, fuzz/shrink, and structural-performance suites;
20. expose Component 17-ready private edge-range adapters without changing serial semantics;
21. run all supported scalar/index and strict-build profiles; and
22. verify Component 07 integration without broad-phase recomputation.

Do not add a future provider, omit internal diagonals, or add narrow-phase pruning while implementing V1.

## 20. Planning-step completion

This independent planning review is complete when:

- the provider-neutral Component 06 specification remains satisfied;
- the concrete V1 provider/domain are unambiguous;
- runtime owner validation is separated from canonical semantic identity;
- all existing Ygor spatial providers, including `kdtree<T>`, have explicit reuse decisions;
- empty operand and mandatory verifier behavior are explicit;
- Components 03, 05, 07, 16, and 17 integration is consistent;
- every future implementation requirement below has an implementable instruction and executable gate; and
- `tracker.md` marks Component 06 complete as a planning/review milestone.

## 21. Future implementation definition of done

Component 06 implementation is complete only when:

- every canonical source edge and internal diagonal is queried exactly once against all potentially overlapping opposite source triangles in both roles;
- no ordinary V1 cross-operand filter is applied;
- all bounds are Component 03 finite closed conservative AABBs reconstructed from Component 05 lineage;
- tolerance is never universal broad-phase inflation;
- provider keys, ranks, layout, node IDs, traversal evidence, and bytes are versioned and deterministic;
- both hierarchies contain every required triangle exactly once and every node contains descendants;
- pruning occurs only on definite separation;
- exact contact and uncertainty retain candidates;
- count/prefix/reservation/emit reconciles exact capacity and work;
- candidate keys are complete, owner-free, unique, canonical, and receive IDs only after canonicalization;
- runtime owner validation rejects stale/cross-context access without affecting semantic bytes;
- V1 duplicate discovery is zero;
- every candidate exposes complete immutable provenance, bounds, precision references, and non-narrow overlap evidence;
- all explicit edge cases have tested behavior;
- the independent verifier reconstructs primitives, ranks, hierarchy, mandatory breadth-first candidate set, codec, and digests without producer control flow;
- exhaustive all-pairs comparison finds no false negatives;
- mutation tests reject omitted/corrupt/owner-bearing semantic artifacts;
- resource and cancellation paths publish nothing partial and reconcile all leases;
- structural gates prevent accidental all-pairs behavior in sparse/localized scenes while documenting true worst-case quadratic output;
- serial and qualified parallel schedules produce identical semantic artifacts, counters, and primary failures;
- Component 07 consumes the stream without rerunning discovery or recomputing bounds; and
- all production and normative-test code is self-contained strict portable C++17 with no external dependency.
