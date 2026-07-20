# Plan 06: Broad-Phase Collision Enumeration

## 0. Scope and non-negotiable constraints

Implement **only Component 06** from `component_06_broad_phase_collision_enumeration.md`. This component accepts the immutable `canonical_source_manifolds<T,I>` from Component 05 together with the frozen Boolean and precision contexts, constructs deterministic conservative acceleration data, and publishes exactly one immutable `canonical_candidate_stream<T,I>` for Component 07.

The implementation must schedule every directed canonical edge/opposite source-triangle relation whose conservative closed feature bounds are not definitely separated. It may emit false positives. It must never omit an interaction admitted by the Component 03 bound model, invent a narrow-phase fact, or allow traversal order to become semantic.

The V1 provider is fixed by this plan:

- build one immutable **rank-Morton linear AABB hierarchy** over the canonical source triangles of each operand;
- query the B-triangle hierarchy once for every canonical undirected edge of A and query the A-triangle hierarchy once for every canonical undirected edge of B;
- include both original source edges and facet-internal triangulation diagonals, with the latter remaining explicitly bookkeeping-only;
- use a deterministic count/prefix/emit pipeline so exact output capacity is known before candidate materialization;
- canonicalize by complete candidate key after traversal and require zero duplicate discovery for the V1 non-replicating provider; and
- run an independent structural and traversal verifier before publication.

The component must not:

- call, adapt, copy from, or depend on `src/YgorMeshesBoolean{,2,3,4,5}*.{h,cc}`;
- call the existing public `rtree<T>`, `octree<T>`, or `cells_index<T>` as the production provider;
- use `index_bbox<T>` as authoritative conservative evidence;
- read mutable caller mesh arrays or rebuild source topology from coordinates;
- index both halfedge twins as separate undirected edges;
- omit facet-internal diagonals in V1;
- infer adjacency, equivalence, source ownership, or duplicate identity from coordinates, bounds, spatial keys, or hashes;
- expand bounds by the caller tolerance as a universal proximity rule;
- use open overlap, unchecked subtraction, raw midpoint arithmetic, unqualified `vec3` arithmetic, or a nominal plane-side test to prune;
- perform edge/triangle intersection, coplanarity, sidedness, tangency, contact ownership, symbolic perturbation, event construction, winding, or Boolean selection;
- assign candidate IDs from traversal, allocation, task, worker, hash, node, or pointer order;
- truncate candidates to satisfy a limit;
- publish a partial hierarchy or stream after failure, cancellation, or resource exhaustion;
- throw exceptions for expected contract, resource, numerical, cancellation, codec, or verification failures;
- use raw struct serialization, object padding, locale-dependent text, `std::hash` as identity, or non-canonical unordered iteration;
- use fast-math, floating contraction contrary to Component 03, `long double`, a changed rounding mode, or any external dependency; or
- compile production or normative-test code outside the strict portable C++17 target established by Component 01.

Use Component 01 for owner tokens, strong IDs, stage/checkpoint registration, checked count and byte arithmetic, typed outcomes, resource leases, cancellation, deterministic failure arbitration, diagnostics, replay, canonical bytes, SHA-256, transactions, immutable publication, and execution-policy validation. Use Component 03 as the sole authority for finite closed AABBs, endpoint total-order keys, conservative union/containment, closed overlap, definite separation, and precision references. Use Component 05 as the sole authority for canonical edge, halfedge, triangle, source-facet, shell, semantic-class, direction, and provenance identities.

No failed, cancelled, limit-exhausted, partially emitted, partially encoded, or verifier-rejected artifact may publish. Mark Component 06 complete in `tracker.md` only after every requirement in Section 24 is represented by an implementable instruction and qualification gate.

## 1. Existing Ygor assessment and mandatory reuse decisions

### 1.1 `index_bbox<T>` is not the bounded-Boolean AABB contract

`src/YgorIndex.h/.cc` provides `index_bbox<T>` with min/max corners, containment, intersection, union, volume, center, and finite checks. Its closed `intersects()` shape is conceptually useful, but it is not suitable as Component 06 evidence because it:

- accepts raw nominal corners and silently canonicalizes reversed endpoints;
- has no owner, operand, feature, formula, precision-lineage, or artifact-version binding;
- computes center, volume, area, and differences with unchecked ordinary floating arithmetic;
- does not distinguish definite separation from an uncertain comparison;
- does not record outward-rounding or inherited uncertainty;
- exposes no proof record, replay formula, resource accounting, or independent verification contract; and
- is shared by mutable general-purpose indexes whose behavior is not frozen for Boolean semantics.

Keep `index_bbox<T>` source-compatible for existing Ygor users. Do not widen its contract or make Component 06 correctness depend on it. Consume Component 03 `finite_aabb<T>` records and services directly.

### 1.2 Existing `octree<T>` is substantially deficient for this component

`src/YgorIndexOctree.h/.cc` supports point and box insertion and closed box search, but it uses a mutable pointer tree, `std::any`, insertion-driven root expansion, raw center calculations, raw `<=`/`>=` decisions, fixed heuristic limits, exceptions, and traversal results in provider discovery order. Its topology and bytes depend on insertion history; it exposes no canonical primitive identities, immutable flat representation, bound-containment proof, deterministic failure, cancellation, resource transaction, codec, or verifier.

Do not refactor the public octree into the Boolean provider. Such a refactor would either break its public behavior or leave Component 06 coupled to incompatible semantics. It may be used only in non-normative benchmark comparisons.

### 1.3 Existing `rtree<T>` is insertion-order and floating-heuristic dependent

`src/YgorIndexRTree.h/.cc` implements a mutable R*-tree using insertion, reinsertion, volume/overlap heuristics, parent pointers, `std::any`, and floating equality/tie behavior. Split and subtree choices do not include complete canonical feature-key tie breakers, and the implementation throws on expected failures. It has no conservative uncertainty contract, immutable layout, candidate-domain policy, deterministic serialization, or independent structural verifier.

Do not call or incrementally retrofit it for Component 06. Preserve it as a general-purpose index. The V1 Boolean hierarchy must be a new bounded-subsystem provider with a frozen layout.

### 1.4 Existing `cells_index<T>` is not appropriate

`src/YgorIndexCells.h/.cc` uses a caller-selected floating cell size, floating-to-integer cell mapping, `std::unordered_map`, `std::any`, and point-oriented insertion semantics. Extreme-scale coordinates, subnormals, signed zero, bin-boundary ties, replicated boxes, iteration order, and resource growth are not governed by the Component 03/Component 01 contracts.

Do not use it for production or normative verification. A grid provider may be added only under a future provider version with an independently specified conservative quantization contract.

### 1.5 Tetrahedralization spatial code is a non-authoritative reference only

`src/YgorMeshesTetrahedralize.cc` contains a triangle/AABB SAT helper and an octree-based centroid search. It triangulates and mutates a public mesh, uses raw arithmetic and heuristic epsilon tests, indexes centroids rather than complete conservative triangle bounds, and performs narrow-phase geometry. None of those routines satisfy the broad-phase contract. Do not reuse them in the producer or verifier.

### 1.6 Mandatory reuse

Reuse, rather than duplicate:

- Component 05 canonical edge records, canonical directed representatives, reciprocal halfedges, triangle records, source-facet/shell groups, semantic edge classes, provenance maps, and geometry-attachment IDs;
- Component 03 finite AABB records, exact scalar bit/total-order keys, closed-overlap and definite-separation results, checked conservative union, containment, and bound reconstruction;
- Component 01 strong candidate IDs, checked integers, resource kinds, transaction services, canonical bytes, SHA-256, diagnostics, replay, and deterministic failure selection; and
- `vec3<T>` only as a nominal coordinate carrier where a predecessor view requires it, never for authoritative Stage 06 arithmetic.

Do not create a second conservative-bound implementation, precision ledger, source-edge registry, triangle registry, source-facet grouping, shell grouping, or hash/digest subsystem.

### 1.7 Permitted implementation machinery

Use strict C++17 standard-library facilities such as fixed-width integers, `std::array`, `std::vector`, `std::optional`, `std::variant`, `std::sort`, `std::lower_bound`, checked index wrappers, and deterministic merge. Prefer contiguous arrays and sorted vectors. `std::unordered_*` is unnecessary for V1 and must not be used in the producer, verifier, codec, or normative oracle.

## 2. V1 provider and candidate-domain decisions

### 2.1 Candidate-domain policy

Freeze V1 as:

```text
all_canonical_edges_against_all_opposite_source_triangles_v1
```

For each operand, enumerate every Component 05 `manifold_edge_id` exactly once through its canonical directed representative. This includes:

- `canonical_edge_class::source_edge`; and
- `canonical_edge_class::facet_internal_diagonal`.

Query it against every potentially overlapping source triangle of the opposite operand. Do not apply any ordinary cross-operand topological exclusion. Every emitted ordinary candidate has `topological_filter_reason::not_filtered`.

Including internal diagonals is deliberate. It guarantees complete triangle-local discovery for Component 07 without requiring a proof that source edges alone cover every concave-facet interaction. Internal diagonals remain bookkeeping witnesses and never become original source-feature owners, symbolic owners, classification barriers, or final event identities.

A future policy that omits internal diagonals requires a new nonzero candidate-domain version, an explicit proof contract, and exhaustive oracle qualification. It must not be introduced as a tuning switch under V1.

### 2.2 Acceleration provider

Freeze the V1 provider as:

```text
rank_morton_triangle_lbvh_v1
```

Build one hierarchy over A triangles and one over B triangles. Edges remain in canonical Component 05 order and are query primitives; no edge hierarchy is needed in V1. The directed pipeline is:

```text
A canonical edges -> query B triangle LBVH
B canonical edges -> query A triangle LBVH
```

The hierarchy is a flat, immutable, bottom-up binary AABB tree over triangles sorted by a deterministic rank-Morton key. Spatial keys affect performance and provider bytes only. They never justify pruning; only Component 03 definite separation does.

### 2.3 Fixed provider constants

Version and freeze these V1 constants:

- leaf capacity: `8` triangle primitives;
- axis order in rank interleaving: X, then Y, then Z for each bit position;
- bit order: most-significant rank bit first;
- endpoint-key order per axis: lower endpoint total-order key, upper endpoint total-order key;
- equal endpoint-key pairs share one dense rank;
- rank bit width is the checked maximum width required by X, Y, or Z ranks for that operand;
- final primitive-order tie key: complete canonical triangle key;
- leaf creation: consecutive groups of at most eight sorted primitives;
- internal creation: pair adjacent nodes level by level; carry an odd final node unchanged to the next level;
- all serialized internal nodes have exactly two children;
- node IDs: leaves first in primitive-range order, then internal nodes in level/left-range order;
- root: the sole node remaining after level reduction, or absent for an empty hierarchy;
- producer traversal: depth-first, left child before right child;
- verifier traversal: breadth-first, independent control flow; and
- candidate order: complete lexicographic candidate key, followed only during pre-dedup sorting by canonical witness key.

Changing a constant that can change provider bytes, structural counters, replay, or candidate discovery evidence requires a provider/layout version change. Candidate semantics must remain unchanged if only the acceleration provider changes under an identical domain policy.

## 3. Exact file and target layout

Add under `src/YgorMeshesBooleanBounded/`:

- `BroadPhaseTypes.h` — stable enums, tags, fixed constants, strong-ID aliases, keys, counter records, and closed result types;
- `CandidateDomainPolicy.h/.cc` — V1 edge inclusion, directed-role mapping, semantic-class checks, and future-policy dispatch;
- `BroadPhasePrimitiveTables.h/.cc` — immutable edge/triangle primitive records and predecessor-bound validation;
- `RankMortonKey.h/.cc` — endpoint-key ranking, dense rank assignment, rank interleaving, complete spatial keys, and known-answer helpers;
- `TriangleLBVH.h/.cc` — immutable flat hierarchy schema, deterministic build, checked views, and producer structural checks;
- `BroadPhasePreflight.h/.cc` — counts, representability, upper-bound arithmetic, resource estimates, and exact count-plan storage;
- `BroadPhaseTraverse.h/.cc` — serial count traversal, serial emit traversal, private partition task entrypoints, and overlap witnesses;
- `CandidateCanonicalization.h/.cc` — complete-key validation, sorting, duplicate handling, candidate-ID assignment, and partition construction;
- `CanonicalCandidateStream.h` — immutable artifact, headers, tables, records, statistics, and checked downstream views;
- `BroadPhaseBuild.h/.cc` — typed stage entrypoint and fixed phase orchestration;
- `BroadPhaseCodec.h/.cc` — canonical encoding/decoding and digest calculation;
- `BroadPhaseVerifier.h/.cc` — independent primitive reconstruction, LBVH reconstruction, breadth-first traversal, all-pairs oracle, codec verification, and mutation rejection; and
- `BroadPhaseQueries.h` — narrow owner-checked immutable views for Component 07 and diagnostics.

Extend existing bounded-subsystem registries rather than creating parallel infrastructure:

- `ContractVersions.h` for Component 06 provider, policy, schema, key, layout, codec, and verifier versions;
- Component 01 stage, checkpoint, strong-ID-domain, error-subcode, resource-kind, diagnostic, and replay registries;
- Component 03 capability declarations only where endpoint total-order, closed overlap, union, containment, or bound reconstruction is not already exposed; and
- the strict bounded Boolean CMake target and explicit-instantiation lists.

Add under `tests/mesh_boolean_bounded/`:

- `TestBroadPhaseDomain.cc`;
- `TestBroadPhaseRankMorton.cc`;
- `TestBroadPhaseHierarchy.cc`;
- `TestBroadPhaseOverlap.cc`;
- `TestBroadPhaseKnownCandidates.cc`;
- `TestBroadPhaseAllPairsOracle.cc`;
- `TestBroadPhaseProvenance.cc`;
- `TestBroadPhaseCanonicalization.cc`;
- `TestBroadPhaseAlternativeTriangulation.cc`;
- `TestBroadPhaseMutation.cc`;
- `TestBroadPhaseProperties.cc`;
- `TestBroadPhaseAdversarial.cc`;
- `TestBroadPhaseResourcesCancellation.cc`;
- `TestBroadPhaseStructuralPerformance.cc`;
- `BroadPhaseFixtures.h/.cc`;
- `BroadPhaseExactOracle.h/.cc`; and
- `GoldenBroadPhaseV1.h`.

Register separate CTest cases for domain, rank keys, hierarchy, overlap, known candidates, exhaustive oracle, provenance, canonicalization/codec, alternative triangulation, mutation, properties/fuzz, adversarial floating point, resources/cancellation, and structural performance. Apply the strict floating target to every production and normative-test translation unit. No network discovery or optional test package is permitted.

Keep mutable primitive proposals, axis-sort arrays, count plans, traversal stacks, task-local buffers, sort workspaces, verifier scratch, and mutation helpers out of installed/public headers. Templates must be header-defined or explicitly instantiated for exactly the supported `float`/`double` and `uint32_t`/`uint64_t` combinations.

## 4. Stable versions, stages, checkpoints, and failure subcodes

### 4.1 Version registry

Add explicit nonzero V1 constants for:

- Component 06 artifact schema and provider;
- candidate-domain policy;
- edge primitive, triangle primitive, axis endpoint key, dense rank, rank-Morton key, LBVH node, count-plan, overlap-witness, candidate-key, candidate-record, partition-table, statistics, verification-evidence, and replay schemas;
- leaf-capacity/layout policy;
- primitive spatial-order policy;
- node-ID and level-construction policy;
- traversal policy;
- candidate canonical order and duplicate policy;
- canonical encoding and digest layouts; and
- Component 06 verifier.

Zero is invalid. Reject unknown required versions, unsupported enum values, nonzero reserved fields, incompatible predecessor formulas, or a replay/provider combination not registered as qualified. Encode all versions and fixed constants in the artifact header, canonical bytes, diagnostics, verifier evidence, and replay.

### 4.2 Fixed checkpoints

Register and use these Component 06 checkpoints in order:

1. context capability and strict-environment validation;
2. Component 03 and Component 05 owner/version/digest validation;
3. provider and candidate-domain policy validation;
4. edge/triangle count, index, byte, and worst-case pair preflight;
5. fixed-work and primitive-table resource reservation;
6. A edge and triangle primitive-table construction;
7. B edge and triangle primitive-table construction;
8. primitive-bound reconstruction and validation;
9. A triangle axis-key generation and dense ranking;
10. A rank-Morton ordering;
11. A leaf construction;
12. A internal-level construction and root finalization;
13. A hierarchy producer verification;
14. B triangle axis-key generation and dense ranking;
15. B rank-Morton ordering;
16. B leaf construction;
17. B internal-level construction and root finalization;
18. B hierarchy producer verification;
19. A-edge/B-triangle count traversal;
20. B-edge/A-triangle count traversal;
21. checked prefix construction and exact candidate/work reservation;
22. A-edge/B-triangle emit traversal;
23. B-edge/A-triangle emit traversal;
24. per-edge count/work reconciliation;
25. candidate-record validation;
26. complete-key sort and duplicate scan;
27. final candidate-ID and ordinal assignment;
28. deterministic partition-table construction;
29. producer artifact verification;
30. canonical encoding and digest construction;
31. independent verifier primitive/hierarchy reconstruction;
32. independent verifier traversal and candidate-set comparison;
33. bounded exhaustive all-pairs verification when required;
34. replay/resource reconciliation; and
35. final cancellation check and transaction commit.

Do not renumber released checkpoints. Provider-specific additions require reserved gaps or a new provider/stage version.

### 4.3 Required failure subcodes

Allocate a disjoint Component 06 range with explicit values for at least:

- unsupported provider, policy, layout, codec, verifier, or predecessor formula;
- wrong, stale, cross-context, cross-operand, or cross-domain handle;
- predecessor artifact/digest/disposition mismatch;
- malformed operand role or directed-pass mapping;
- entity, pair-product, count, rank, node, height, byte, work, or index overflow;
- fixed, temporary, persistent, candidate, verifier, diagnostic, or replay resource exhaustion;
- malformed edge or triangle primitive reference;
- missing, non-finite, inverted, stale, or non-conservative bound;
- edge bound inconsistent with endpoint bounds;
- triangle bound inconsistent with vertex bounds;
- candidate-domain inclusion mismatch;
- internal diagonal mislabeled as source edge or source-feature eligible;
- duplicate canonical edge representative or twin emitted as a second primitive;
- axis endpoint key malformed;
- dense rank missing, non-monotone, duplicated incorrectly, or out of range;
- rank-Morton key mismatch;
- primitive spatial order duplicate or non-total;
- leaf empty, oversized, non-contiguous, or out of range;
- node child missing, duplicated, self-referential, forward, non-contiguous, or cyclic;
- node bound missing, non-finite, or not containing a child/primitive;
- root missing, duplicated, or not covering all primitives;
- count traversal work/candidate limit exceeded;
- emit count differs from count plan;
- emit work differs from count plan where required;
- overlap witness missing, malformed, or contradicting bound comparison;
- definitely separated pair emitted;
- overlapping or uncertain pair omitted by producer reconstruction;
- candidate owner, role, edge, triangle, class, policy, or relation-family mismatch;
- candidate key duplicate;
- V1 duplicate discovery count nonzero;
- candidate key order or ID/ordinal mismatch;
- partition range invalid, overlapping, incomplete, or non-canonical;
- codec tag, length, count, reserved-field, or digest error;
- independent hierarchy reconstruction mismatch;
- independent traversal candidate-set mismatch;
- exhaustive all-pairs false negative;
- producer/verifier counter mismatch;
- resource reconciliation failure;
- cancellation; and
- internal construction invariant failure.

Every failure records stage/checkpoint, context owner, provider/policy versions, directed role, least canonical offending edge/triangle/node/candidate key, expected and observed values, relevant bound bits and comparison evidence, exact counters, applicable limits/reservations, predecessor digests, and deterministic replay payload. Parallel-capable paths submit failures to Component 01 canonical failure arbitration; no worker throws or logs directly.

## 5. Public entrypoint and capability boundaries

### 5.1 Typed entrypoint

Provide an internal entrypoint equivalent to:

```cpp
template <class T, class I>
stage_outcome<canonical_candidate_stream<T,I>>
build_canonical_candidate_stream(
    const boolean_context<T,I>& context,
    const precision_context<T>& precision,
    const canonical_source_manifolds<T,I>& manifolds);
```

The exact wrapper naming may follow Component 01 conventions. The observable behavior must:

- validate all three inputs before allocating provider state;
- support either or both operands empty;
- build both triangle hierarchies privately;
- execute both directed passes under one transaction;
- select the same primary failure for every allowed schedule and thread count;
- join or complete all private work before rollback;
- publish no hierarchy independently from the final stream; and
- commit one immutable artifact only after the independent verifier accepts it.

A lower-level test entrypoint may build one operand hierarchy or execute one directed pass. Ordinary pipeline publication remains all-or-nothing.

### 5.2 Narrow predecessor views

Consume owner-checked immutable views.

From Component 05 require:

- canonical edge count and enumeration by operand;
- canonical edge key, class, canonical directed representative, two reciprocal halfedges, endpoint vertices, incident triangles, source facets, shells, and role-specific provenance;
- canonical triangle count and enumeration by operand;
- canonical triangle key, oriented vertices, source facet, shell, and provenance;
- edge and triangle conservative-bound attachment IDs;
- source semantic and exact triangulation digests;
- artifact versions, owner tokens, and verified disposition; and
- constant-time or documented logarithmic checked lookup by strong ID.

From Component 03 require:

- finite AABB checked views and exact endpoint bit access;
- endpoint total-order key service with signed-zero policy;
- conservative edge and triangle bound reconstruction;
- checked conservative AABB union;
- closed overlap and definite separation with per-axis evidence;
- AABB containment verification;
- precision-ledger references for source features; and
- formula/provider/version validation.

From Component 01 require:

- context/operand owner validation;
- candidate strong-ID allocation after canonicalization;
- checked `add`, `multiply`, prefix, count-to-byte, and index conversions;
- resource reservations and reconciliation;
- cancellation checkpoints;
- deterministic execution partition descriptions;
- canonical failure arbitration;
- canonical bytes and SHA-256;
- diagnostics and replay builders; and
- transaction publication.

Do not expose mutable predecessor arrays, provider pointers, `std::any`, or unvalidated raw ordinals.

### 5.3 Downstream capability

Component 07 receives a narrow `candidate_stream_view<T,I>` supporting:

- artifact header/version/digest inspection;
- exact candidate count;
- canonical forward iteration;
- checked random access by `candidate_id`/ordinal;
- deterministic partition ranges;
- candidate key, directed role, edge ID/class, triangle ID, and witness lookup;
- immutable resolution of edge endpoints, canonical direction, reciprocal halfedges, triangle vertices/orientation, source facet, shell, and precision references; and
- no hidden traversal, allocation, mutation, or lazy cache.

Component 07 must not be able to request more candidates, rerun the hierarchy, or mutate candidate order.

## 6. Strong IDs, enums, keys, and closed schemas

### 6.1 Strong ID domains

Use the Component 01 `candidate_id` domain. Add distinct Stage 06 domains for:

- `broad_phase_edge_primitive_id`;
- `broad_phase_triangle_primitive_id`;
- `broad_phase_node_id`;
- `overlap_witness_id`;
- `candidate_partition_id`; and
- verifier-evidence IDs where generic evidence IDs are insufficient.

Do not alias these domains to `I`, `size_t`, raw array offsets, Component 05 IDs, or one another. Dense ordinals are checked storage details only.

### 6.2 Closed enums

Use explicit nonzero values:

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

enum class lbvh_node_kind : std::uint8_t {
    leaf = 1,
    internal = 2
};
```

Unknown values and zero fail. V1 ordinary candidates use only `not_filtered`; any exclusion value in an ordinary V1 pass is a policy mismatch.

### 6.3 Complete candidate key

Define a canonical key equivalent to:

```text
(
  directed_role,
  relation_family,
  complete canonical edge key,
  complete canonical triangle key,
  canonical_edge_class,
  candidate_domain_policy_version
)
```

The edge and triangle complete keys include owner and operand namespace through Component 05. Do not use only dense IDs in serialized identity. Provider version, node ID, traversal witness, thread count, and spatial key are excluded from semantic candidate identity; the artifact header records provider version separately.

Before duplicate validation sort by `(candidate_key, overlap_witness_key)`, where the witness key includes opposite-hierarchy operand, leaf node key, triangle spatial slot, per-axis overlap category, and witness-schema version. After validation, observable order is candidate-key order only. V1 requires exactly one witness per key.

## 7. Artifact and record schemas

### 7.1 Artifact header

`canonical_candidate_stream<T,I>` stores:

- context owner/token and both operand IDs;
- Component 06 provider, policy, schema, layout, traversal, codec, verifier, and key versions;
- required Component 01, 03, 04, and 05 versions/formulas;
- predecessor artifact IDs/digests;
- forwarded source semantic and exact triangulation digests;
- fixed provider constants;
- edge/triangle primitive counts by operand and edge class;
- hierarchy node/leaf/root/height counts by operand;
- directed-pass candidate counts;
- exact table ranges and checked byte lengths;
- precision capability/profile references;
- resource/statistics and verification-evidence ranges;
- deterministic partition table range;
- replay reference;
- primitive-table, hierarchy, candidate, evidence, and complete-artifact digests; and
- zeroed reserved fields.

No mutable workspace, caller pointer, provider object, stack, task buffer, or allocator handle may escape.

### 7.2 Edge primitive record

For every canonical Component 05 edge, store:

- Stage 06 primitive ID and canonical ordinal;
- owner and operand role;
- exact `manifold_edge_id` and complete edge key;
- `canonical_edge_class`;
- canonical directed representative halfedge;
- reciprocal halfedges and incident triangles;
- canonical endpoint manifold/source vertex IDs;
- source-edge or internal-diagonal role-specific provenance;
- source facets and shells for both uses;
- immutable Component 03 edge AABB record or canonical endpoint bits plus attachment ID;
- endpoint and aggregate precision-ledger references;
- candidate-domain inclusion disposition, which must be `included` in V1; and
- zero reserved fields.

Edge primitive order is Component 05 canonical edge order. Verify each undirected edge appears once and no twin halfedge creates a second primitive.

### 7.3 Triangle primitive record

For every Component 05 source triangle, store:

- Stage 06 primitive ID and canonical ordinal;
- owner and operand role;
- exact `manifold_triangle_id` and complete triangle key;
- canonical oriented vertex cycle;
- source facet, shell, group, and exact triangulation provenance;
- immutable Component 03 triangle AABB record or canonical endpoint bits plus attachment ID;
- vertex and aggregate precision-ledger references;
- three axis endpoint-pair keys;
- three dense axis ranks;
- complete rank-Morton key;
- final spatial ordinal; and
- zero reserved fields.

The spatial ordinal is provider-local and non-semantic. Canonical triangle identity remains authoritative.

### 7.4 LBVH node record

Every node stores:

- strong node ID and dense ordinal;
- operand;
- `lbvh_node_kind`;
- conservative node AABB;
- contiguous subtree primitive range `[first, first + count)` in spatial order;
- subtree primitive count;
- level-from-leaves and computed height range;
- canonical node key;
- role-specific leaf or internal payload; and
- zero reserved fields.

Leaf payload stores `first_primitive` and `primitive_count` in `[1,8]`. Internal payload stores exactly `left_child` and `right_child`. Child IDs are less than the parent ID; child primitive ranges are adjacent, nonempty, and exactly cover the parent range. Parent bounds are Component 03 checked unions.

Canonical node key:

```text
(operand, first_spatial_primitive, subtree_primitive_count,
 node_kind, level_from_leaves, node_layout_version)
```

### 7.5 Count-plan, witness, candidate, and partition records

For each queried edge, the private count-plan records role, edge primitive, expected node tests, leaf visits, primitive overlap tests, definite-separation prunes, uncertain retentions, emitted candidate count, checked output prefix, and digest contribution.

Each overlap witness records:

- witness ID/version and role;
- edge and triangle primitive/bound references;
- admitting leaf node and spatial slot;
- Component 03 per-axis overlap/separation categories;
- whether any comparison was uncertain;
- formula/provider references;
- `topological_filter_reason::not_filtered`;
- count-plan edge offset; and
- canonical witness key.

It must not contain an intersection point, side sign, residual, contact class, crossing multiplicity, or symbolic decision.

Each candidate stores final ID/ordinal, complete key, role/family, edge primitive/Component 05 edge/class, triangle primitive/Component 05 triangle, witness ID, provenance and precision references, provider/domain versions, optional partition reference, and zero reserved fields.

Create deterministic contiguous candidate partitions for Component 07 based only on final candidate ordinals and a frozen maximum-records-per-partition constant from the execution-policy profile. Thread count must not change boundaries. Each partition stores ID, first ordinal, count, first/last key summaries, checked byte range, and digest contribution.

## 8. Preflight, capacity, and resource model

### 8.1 Count and representability validation

Before building tables, validate with Component 01 checked arithmetic:

- `E_A`, `E_B`, `T_A`, and `T_B` fit all strong-ID and storage domains;
- `E_A * T_B` and `E_B * T_A` are representable as worst-case pair counts even if policy limits are lower;
- triangle leaf counts `ceil(T/8)` are representable;
- maximum node count is at most `2 * leaf_count - 1` for nonempty trees;
- hierarchy height, traversal stack, verifier queue, rank, and prefix counts are representable;
- every count-to-byte conversion for primitive, node, count-plan, candidate, witness, partition, codec, diagnostic, replay, and verifier storage is checked; and
- Component 07 candidate IDs can represent the final count.

A representability failure is distinct from a caller resource limit.

### 8.2 Resource classes and reservation phases

Account separately for:

- edge/triangle primitive records;
- axis-sort references and rank/spatial keys;
- leaf/internal node records;
- count-plan records;
- producer stack and verifier queue work;
- node and primitive comparisons;
- pre-dedup and final candidates;
- witnesses;
- sort/merge and partitions;
- canonical bytes and digests;
- diagnostics/replay; and
- persistent artifact bytes.

Reserve fixed tables/work before construction. Do not reserve worst-case quadratic candidate storage eagerly when output-sensitive counting can avoid it.

After both count traversals:

1. checked-sum per-edge counts in canonical role/edge order;
2. build exclusive prefixes;
3. validate total against entity/work limits;
4. compute exact candidate/witness bytes;
5. compute canonicalization/verifier bytes;
6. reserve all transactionally; and
7. only then emit candidates.

If reservation fails, publish nothing. Do not emit a prefix, lower precision, alter leaf capacity, or switch providers automatically.

Hard correctness limits cause typed failures. Advisory performance targets only affect reports and release qualification; they never prune candidates.

## 9. Primitive-bound preparation and validation

For each edge and triangle, obtain the Component 05 geometry attachment and Component 03 AABB. Validate owner, operand, feature ID, formula version, precision lineage, finite endpoint bits, and artifact digest before use.

Independently reconstruct producer-side:

- each edge AABB as the Component 03 coordinate-wise hull of its two bounded endpoints; and
- each triangle AABB as the hull of its three bounded vertices.

Compare exact canonical endpoint bits and formula references with the Component 05 attachment. A mismatch is an internal predecessor contradiction, not permission to choose the larger/smaller box.

All bounds are closed. Exact endpoint equality is overlap. A comparison reported uncertain retains the pair. Only `definitely_separated` may prune. Signed zero and subnormals follow Component 03 rules; Component 06 performs no independent normalization.

Inherited precision and bound-construction roundoff are already included by Component 03. Do not add `options.tolerance` to every axis. If a finite conservative bound or union cannot be represented, return the typed numerical disposition; never use infinity, clamp, use nominal bounds, or skip a primitive.

## 10. Rank-Morton key construction

### 10.1 Endpoint-pair keys and dense ranks

For each triangle and axis form:

```text
axis_endpoint_key =
(lower_endpoint_total_order_key, upper_endpoint_total_order_key)
```

Use Component 03 exact scalar bit/total-order services. Do not compute centers, extents, normalized coordinates, or floating grid cells.

For each operand and axis independently:

1. create references to all triangle primitives;
2. sort by `(axis_endpoint_key, complete triangle key)`;
3. scan in order;
4. assign the same dense rank to exactly equal endpoint-pair keys;
5. increment only when the pair changes; and
6. store the rank.

Rank zero is valid. Check every increment. The triangle key makes ordering total but does not force distinct ranks for identical bounds.

### 10.2 Interleaving and complete spatial order

For each axis compute the width required for its maximum dense rank. Use the maximum of those three widths as the operand rank width. Empty and one-distinct-rank operands have width zero.

Interleave bits from most significant to least significant in X/Y/Z order into fixed-width words large enough for three 64-bit ranks:

```text
append x_rank[bit]
append y_rank[bit]
append z_rank[bit]
```

Use explicit unsigned shifts/masks and canonical big-endian word significance. Do not use implementation-defined bitfields. Zero unused bits according to schema.

Sort triangles by:

```text
(rank_morton_key,
 x_axis_endpoint_key,
 y_axis_endpoint_key,
 z_axis_endpoint_key,
 complete triangle key)
```

Verify endpoint keys against AABB bits, dense-rank monotonicity/no gaps, interleaving, total order, duplicate freedom, and complete triangle coverage.

## 11. Triangle LBVH construction

### 11.1 Leaves

For `T == 0`, publish the canonical empty hierarchy with no nodes/root. Otherwise create leaves for consecutive ranges `[0,8)`, `[8,16)`, and so forth; the last leaf has `1..8` primitives. Compute leaf bounds by Component 03 checked union in ascending spatial order and assign leaf IDs by range order. Empty leaves are forbidden.

### 11.2 Bottom-up internal levels

Let the current level be the leaf ID sequence. While more than one node remains:

1. pair adjacent nodes `(0,1)`, `(2,3)`, etc. in current-level order;
2. create one parent per pair;
3. set its range to the exact adjacent union of child ranges;
4. compute its bound by checked union of left then right child;
5. assign parent IDs in pair order after all prior nodes;
6. carry one unpaired final existing node unchanged to the next level; and
7. repeat.

No unary node is serialized. Parent IDs exceed child IDs, proving acyclicity. The final ID is root. Union order is frozen because traces/bytes may record it.

Before traversal verify dense IDs, exact leaf coverage, leaf capacities, valid/distinct/backward children, adjacent child ranges, exact reconstructed unions, root range `[0,T)`, root containment, unique canonical node keys, node-count bounds, and canonical empty/singleton forms. Do not trust summary flags.

## 12. Directed traversal and conservative pruning

### 12.1 Canonical traversal order

Process roles in this order:

1. `a_edge_b_triangle`;
2. `b_edge_a_triangle`.

Within a role, query edges in Component 05 canonical order. This determines count-plan ordering and deterministic failure witnesses, not final IDs.

### 12.2 Producer depth-first algorithm

For each edge:

- if the opposite hierarchy is empty, record zero output;
- otherwise push root on a private preflighted stack;
- pop one node and call Component 03 definite-separation/closed-overlap on edge/node bounds;
- prune only if definitely separated;
- if retained internal, push right then left so left processes first;
- if retained leaf, compare edge bound with each triangle bound in ascending spatial slot order;
- retain overlap or uncertainty and count/emit one candidate; and
- record exact structural counters.

The count pass emits no records. It checks node-test, primitive-test, retained-pair, candidate, abstract-work, and cancellation limits at stable increments. Limit failure records current role, canonical edge, node/triangle, and pre/post counter values.

After checked prefix/reservation, repeat identical traversal for emission. Each edge writes only within its preassigned subrange. Reject overflow/underfill, work/category disagreement with the count plan, noncanonical references, or any uninitialized slot. The final output vector never grows during emit.

The only geometric prune is Component 03 definite separation of finite closed AABBs. Endpoint equality, overlap, or uncertainty retains. Do not duplicate comparisons with raw scalars.

Do not add plane-side pruning, SAT, segment/triangle distance, center/radius tests, normal cones, orientation tests, or heuristic thresholds in V1. Any future stronger prune needs a separately versioned bounded contract and independent verification; uncertainty must retain.

## 13. Candidate validation, canonicalization, and IDs

Before sorting validate every emitted record:

- role maps operands correctly;
- edge/triangle IDs exist in expected namespaces;
- edge class matches Component 05;
- internal-diagonal provenance is bookkeeping-only;
- complete key reconstructs exactly;
- witness references the correct leaf/primitive;
- primitive bounds are not definitely separated;
- filter reason is `not_filtered`; and
- precision/provenance references are total.

Sort by `(candidate_key, witness_key)` using a complete comparator over explicit fixed-width/strong-ID fields. Do not rely on hashes or stable sort.

Scan equal candidate-key runs. Generic code may be structured to choose a least witness for future replicated providers, but V1 requires run length exactly one. A longer run indicates duplicate leaf membership, triangle duplication, twin-edge enumeration, or repeated traversal and is an internal invariant failure; do not mask it.

Only after duplicate validation assign dense candidate IDs/ordinals in candidate-key order, rebuild every key, and construct deterministic partitions. Final order is independent of input presentation after canonicalization, spatial order, node/traversal order, tasks/workers, thread count, completion order, allocator, and hash behavior.

## 14. Provenance, diagnostics, statistics, and replay

Each candidate must let Component 07 recover without rerunning discovery:

- canonical directed edge endpoints;
- both reciprocal halfedges and oriented uses;
- source-edge or internal-diagonal identity;
- incident source facets/shells;
- canonical triangle vertices/orientation;
- triangle source facet/shell;
- exact triangulation lineage;
- edge/triangle conservative bounds; and
- precision-ledger references.

Record deterministic counters for edge counts by operand/class; triangle counts; distinct ranks; leaves/nodes/root/height; count/emit node tests, leaf visits, primitive tests, prunes, overlaps, uncertain retentions and candidates by role; duplicates; final candidates by role/class; maximum stack/verifier queue depth; resource reservations/peaks; cancellation progress; and codec bytes. Wall time is non-authoritative.

Failures and optional diagnostics include canonical keys, exact bound bits, per-axis categories, versions, limits/counters, hierarchy range evidence, predecessor digests, and replay identity. Core code returns structured diagnostics and never logs/formats locale-dependent text.

Replay reproduces primitive inclusion/order, bounds/formulas, endpoint keys/ranks/Morton words/spatial order, all nodes/ranges/root, count plans/counters, candidates/witnesses/IDs/partitions, bytes/digests, and primary failure. Thread count/schedule are non-semantic; provider/policy/version/fixed constants are semantic.

## 15. Canonical encoding and decode

Use Component 01 `CanonicalBytes`. Encode explicit tags/lengths for header/versions/constants, predecessor identities/digests, primitive tables, exact AABB endpoint bits/precision references, endpoint keys/ranks/Morton words/spatial order, nodes by ID, candidates by ID, witnesses by ID, partitions by ID, statistics/evidence, and replay references.

Never serialize raw structs, pointers, `std::any`, `size_t`, enum object representations, padding, or container capacities. Encode `T` endpoints by exact Component 03 bit pattern and its signed-zero policy; reject non-finite values before encoding.

Compute separate SHA-256 digests for primitive tables, A hierarchy, B hierarchy, candidate/witness tables, partitions, statistics/evidence, and complete artifact. The complete digest covers versions and predecessor digests.

Decode fail-closed: validate tags, lengths, versions, reserved fields, and count/byte relationships before allocation; reserve resources; decode privately; validate all owners/ranges/keys/references; reconstruct hierarchy/candidates; recompute digests; and publish only after the same independent verifier passes. Reject trailing bytes.

## 16. Independent verifier

`BroadPhaseVerifier` consumes immutable Stage 06 records and predecessor capabilities. It must not call producer hierarchy-build, DFS traversal, count/emit, canonicalization, or producer structural-check helpers as its sole truth. It may share checked arithmetic, Component 03 public AABB/total-order primitives, schemas, canonical bytes, and directly tested key comparators.

The verifier must:

1. independently enumerate Component 05 edges/triangles and apply V1 domain policy;
2. reconstruct bounds through Component 03 and compare complete primitive tables;
3. independently rebuild endpoint keys, dense ranks, Morton words, and spatial order;
4. independently rebuild leaves and bottom-up levels and compare every node/range/child/bound/key/ID/root;
5. traverse each reconstructed opposite hierarchy **breadth-first** with a queue, using Component 03 definite separation;
6. collect/sort expected candidate keys and compare exactly with published keys;
7. reconstruct every witness, role, class, provenance, ID, order, and partition;
8. re-encode semantic records and compare all digests/lengths; and
9. reject unknown, truncated, duplicated, reordered, stale, contradictory, or nonzero-reserved data.

Full independent breadth-first traversal is mandatory for V1 ordinary publication. Inability to verify is a typed resource failure, never permission to skip.

When both pair products are within a frozen exhaustive threshold, or normative tests request it, enumerate every directed edge/triangle pair without either hierarchy. Apply only V1 domain policy and public Component 03 definite separation. Compare complete key sets. Any missing key is release-blocking. Under V1 the emitted set should exactly equal all non-definitely-separated pairs; extras indicating a contradictory comparison also fail.

Verifier control flow must independently detect primitive loss/duplication, rank errors, hierarchy corruption, candidate omission/addition/reversal/duplication, forged witnesses, stale predecessor digests, and codec/digest mutations.

## 17. Transactionality, cancellation, and deterministic execution

All construction occurs inside one Stage 06 transaction. Hierarchies are private implementation tables within the final artifact; neither hierarchy may publish alone.

Poll cancellation at deterministic safe points during primitive batches, axis sorts/rank scans, spatial sorting, leaf/internal levels, hierarchy verification batches, each edge boundary and bounded node-test interval in count/emit traversals, prefix/reservation, candidate sort batches, ID/partition assignment, codec batches, verifier reconstruction/traversal, all-pairs batches, and final commit.

On cancellation all producers stop at safe points, all qualified tasks join, unpublished state is destroyed, leases return, and the result is `cancelled`. No partial stream is ordinary success.

Implement an executable serial semantic reference first. Parallel execution may partition canonical edge ranges; each task reads immutable inputs and writes private count plans or disjoint preassigned emit ranges. It must not allocate final IDs or mutate shared traversal state. Canonical failure arbitration occurs over complete failure keys. Final sort/ID/partition/verification must reproduce serial semantics, bytes, counters, and primary failure. Component 17 may optimize scheduling only through these boundaries.

## 18. Explicit behavior for edge cases

- **Both operands empty:** publish canonical empty primitive tables, empty hierarchies, empty stream, valid metadata/digests/evidence.
- **One operand empty:** build/validate the nonempty operand hierarchy if required by canonical provider bytes; both directed streams are empty because one role has no edges and the other no opposite triangles.
- **No edges or no opposite triangles:** corresponding pass is empty without fabricating candidates.
- **Singleton triangle hierarchy:** one leaf is root; no internal node.
- **Disjoint root bounds:** each queried edge prunes at root when definitely separated.
- **Exact-bound contact:** retained.
- **Nominal gap covered by uncertainty:** retained.
- **Nested solids without boundary contact:** stream may be empty; Component 09 handles containment, not Component 06.
- **Coincident bounds/features:** retain all identity-distinct required pairs.
- **Dense all-overlap:** quadratic output is valid; fail deterministically if true required output exceeds resource limits.
- **Coordinate-coincident but topologically distinct features:** remain distinct keys/candidates.
- **Non-finite/nonrepresentable conservative bounds:** typed failure, never skip/clamp.
- **Signed zero/subnormal/extreme finite scale:** follow Component 03 qualified bit/order/bound behavior.

## 19. Unit and known-answer tests

### 19.1 Bound and overlap tests

Cover ordinary and zero-extent edge/triangle AABBs, exact endpoint contact, one-ULP gaps/overlaps, signed zero, subnormals, adjacent values near large magnitudes, maximum finite values, large translations with small geometry, uncertainty larger than nominal gap, and union expansion boundaries. For each, test definite separation, retained overlap, uncertainty, non-finite rejection, and nonrepresentable union.

### 19.2 Rank-Morton known answers

Commit exact endpoint-key, dense-rank, active-width, multiword Morton, spatial-order, leaf, node, and root expectations for:

- empty/singleton operands;
- all equal bounds;
- monotone motion along each axis;
- axis ties broken by full triangle key;
- signed-zero endpoint pairs;
- subnormal/normal boundary values;
- extreme finite endpoint keys;
- duplicate coordinate values with distinct triangle identities; and
- input/canonical-order permutations.

Test every shift/word boundary and 32/64-bit index configurations. A slow bit-by-bit test oracle independently interleaves ranks.

### 19.3 Hierarchy tests

Verify leaf coverage/capacity, odd carries at multiple levels, parent IDs after children, exact adjacent ranges, union order, containment, root coverage, node-count bound, canonical serialization, and mutation rejection for gaps/overlaps/cycles/self/forward children/shrunken bounds/wrong root.

### 19.4 Traversal and count/emit tests

Hand-build small trees testing root prune, internal partial prune, leaf mixed retain/prune, all uncertainty, empty tree, exact contact, stack maximum, exact count prefixes, emit subranges, count/emit mismatch detection, and deterministic limit witnesses.

## 20. Candidate fixtures and exhaustive oracle

Commit hand-auditable expected complete candidate keys for:

- empty/empty and empty/nonempty;
- two disjoint tetrahedra;
- nested boxes without boundary interaction;
- one transverse triangle pair;
- vertex-, edge-, and face-touching boxes;
- equal boxes, same/opposite orientation where predecessor validity permits;
- thin crossing slivers;
- disconnected shell collections;
- coordinate-coincident topologically separate shells; and
- concave polygon facets with internal diagonals.

Expected sets distinguish both directed roles and edge semantic classes.

For every bounded fixture/generated small pair:

1. enumerate all canonical edges including internal diagonals exactly once;
2. enumerate every opposite source triangle in both roles;
3. reconstruct bounds independently;
4. apply Component 03 definite separation only;
5. compare complete expected and emitted key sets.

Run across `float`, `double`, `uint32_t`, `uint64_t`, all retained V1 policy/provider versions, empty/singleton forms, and production execution modes. Any missing expected key is a release blocker.

## 21. Provenance, determinism, adversarial, and mutation tests

Verify:

- one undirected edge with reciprocal halfedges appears once per role;
- source-edge versus internal-diagonal tagged payloads;
- internal diagonals remain non-source/non-symbolic/non-barrier;
- complete source-facet/shell/triangle provenance;
- duplicate coordinates with distinct IDs;
- equal numeric IDs in different operand namespaces;
- alternative legal triangulations of a source polygon; and
- global source subdivision preserving the solid.

Alternative triangulation may change bookkeeping candidates/provider bytes, but later canonical source relations and Boolean result must remain invariant.

Apply vertex/facet/shell/component permutations, build-order permutations, axis-tie cases, translations, power-of-two scales, axis permutations, operand exchange with role remap, thread counts 1/2/max, forced task delays/reversed merge, and repeated execution. For fixed provider/policy, primitive tables, hierarchies, candidates, IDs/order, counters, partitions, digests, and primary failure must be byte-identical.

Adversarial cases include exact boundary equality, one representable gap, overlapping uncertainty with nominal separation, long near-parallel geometry, huge boxes/tiny features, subnormal extents, signed-zero ordering, maximum finite values, bound expansion failure, many identical bounds, and coordinate-coincident distinct features.

Mutate valid artifacts by deleting/duplicating primitives, replacing an edge by its twin, changing edge class/ownership flags, changing/shrinking bounds, altering endpoint keys/ranks/Morton bits/spatial order, corrupting leaf ranges/capacity, changing children/ranges/order, creating cycles/self/forward links, shrinking node bounds, changing root/count plans/prefixes, omitting/adding/duplicating/reversing candidates, altering roles/IDs/classes/policies/families, forging witnesses/filter reasons, scrambling IDs/partitions/counters/resources, corrupting codec/reserved/digests, and substituting stale predecessor digests. Independent verification rejects every mutation; the all-pairs oracle detects every omitted candidate.

## 22. Fuzzing, shrinking, resources, cancellation, and performance

Generate valid exact-template manifold pairs varying shell count/nesting, triangle count/aspect ratio, concave facets/triangulations, clustering/separation, coordinate duplication without identity merge, ULP perturbations, inherited uncertainty, translations/scales, disconnected components, dense overlap, and limits. Compare all bounded cases with the all-pairs oracle. Serialize exact bits, predecessor digests, versions, counters, and replay on failure; shrink while preserving category.

For every resource kind test limit-minus-one, limit, and limit-plus-one: primitives, axis references, ranks/keys, leaves/nodes, count plans, traversal work, candidate count/bytes, witnesses, sort/partitions, codec, diagnostics/replay, and verifier/all-pairs work. Confirm no partial artifact and complete lease reconciliation.

Cancel during every checkpoint class including each directed count/emit pass and verifier traversal. Confirm tasks join, leases return, state is destroyed, primary failure is `cancelled`, and retry produces canonical bytes.

Use deterministic structural counters, not wall time alone, to gate:

- rank/key construction and hierarchy build within `O(T log T)` comparison growth;
- exact linear node-count bound;
- constant root work per queried edge for clearly separated root bounds;
- subquadratic primitive tests for large disjoint/localized fixtures;
- no ordinary sparse fixture performing the full pair product;
- output-sensitive growth with overlap density;
- memory proportional to primitives/nodes/count plans/final candidates; and
- full pair-product work only for all-overlap or documented provider worst cases.

Use checked architecture-independent counter ceilings. Changes require investigation/reviewed golden updates, not silent threshold inflation.

## 23. Implementation sequence and handoff gates

Implement in this exact dependency order:

1. register versions, enums, IDs, checkpoints, subcodes, resource kinds, and strict-target files;
2. define primitive/key/node/candidate/artifact schemas and complete comparators;
3. implement V1 domain policy and tests;
4. implement Component 03 capability adapters and bound reconstruction tests;
5. implement endpoint keys, dense ranks, Morton interleaving, and known answers;
6. implement spatial ordering and primitive-table verification;
7. implement leaf/internal hierarchy construction and golden tests;
8. implement producer structural verification;
9. implement serial count traversal and overlap tests;
10. implement checked prefixes and exact reservation;
11. implement serial emit traversal and count/emit reconciliation;
12. implement candidate validation, sorting, duplicate policy, IDs, and partitions;
13. implement immutable Component 07 views;
14. implement codec/digests/replay;
15. implement independent primitive/rank/hierarchy reconstruction;
16. implement independent breadth-first traversal and exact set comparison;
17. implement bounded exhaustive all-pairs oracle;
18. add mutation, provenance, alternative-triangulation, adversarial, resource, cancellation, fuzz/shrink, and performance suites;
19. expose parallel-ready private edge-range tasks without changing serial semantics;
20. run all supported scalar/index configurations and strict build profiles;
21. verify Component 07 adapters consume the stream without rerunning discovery; and
22. update `tracker.md` only after every gate below passes review.

Do not begin a future provider, omit internal diagonals, or add narrow-phase pruning while implementing V1.

## 24. Definition of done

Component 06 is complete only when all of the following are true:

- the V1 domain includes every canonical source edge and internal diagonal exactly once against all potentially overlapping opposite source triangles in both directed roles;
- no ordinary cross-operand topological filter is applied and every emitted filter reason is `not_filtered`;
- all bounds are Component 03 finite closed conservative AABBs reconstructed/validated from Component 05 lineage;
- tolerance is never universal broad-phase inflation;
- endpoint keys, dense ranks, Morton words, spatial order, leaf layout, node IDs, root, and canonical bytes are versioned/deterministic;
- both hierarchies contain every required triangle exactly once and every node bound contains descendants;
- producer checks reconstruct hierarchy unions/ranges rather than trusting summary flags;
- pruning occurs only on Component 03 definite separation;
- exact contact and uncertain comparisons are retained;
- serial count/prefix/emit obtains exact capacity before materialization and reconciles every edge count;
- candidate keys are complete, unique, canonical, and IDs are assigned only after canonicalization;
- V1 duplicate discovery is zero and any duplicate prevents publication;
- each candidate exposes complete immutable provenance, bounds, precision references, and non-narrow overlap evidence;
- empty, singleton, disjoint, nested, touching, coincident, dense-overlap, coordinate-duplicate, subnormal, signed-zero, large-translation, and extreme-finite cases have tested behavior;
- the independent verifier reconstructs primitives, ranks, hierarchy, breadth-first traversal, candidate set, codec, and digests without producer control flow;
- exhaustive all-pairs comparison finds no false negatives across bounded fixtures/generated small meshes;
- mutation tests reject every omitted candidate and corrupt primitive/hierarchy/witness/order/digest artifact;
- resource and cancellation failures publish no partial stream and reconcile all leases;
- structural gates prevent accidental all-pairs behavior in ordinary sparse/localized scenes while documenting true worst-case quadratic output;
- serial and qualified parallel schedules produce identical candidate semantics, bytes, counters, and primary failures;
- Component 07 consumes the immutable stream without rerunning broad-phase discovery or recomputing bounds;
- all production and normative-test code is strict portable C++17, self-contained in Ygor, and dependency-free; and
- `tracker.md` marks Component 06 complete only after the entire plan and qualification evidence are present.
