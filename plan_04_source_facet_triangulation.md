# Plan 04: Source Polygon Facet Triangulation and Provenance

## 0. Scope and non-negotiable constraints

Implement **only Component 04** from `component_04_source_facet_triangulation.md`. The stage accepts one immutable ordinary-pipeline-eligible `validated_operand<T,I>` from Component 02 and publishes one immutable, independently verified `source_triangle_complex<T,I>` for that operand.

The implementation must preserve the exact indexed source boundary, the committed Component 02 coherent geometry basis, source orientation, shell membership, source-feature identity, bounded geometry lineage, and complete provenance. V1 is one-ring, no-Steiner, no source-edge subdivision, no source-vertex removal/merge, and no degenerate published triangle.

This component must not:

- validate or repair arbitrary caller polygon soup;
- reread caller-owned `fv_surface_mesh` arrays;
- change Component 02 source vertex, corner, directed-edge, undirected-edge, facet, ring, shell, or presentation records;
- choose a different coherent realization, support plane, projection frame, or orientation convention than Component 02 committed;
- substitute nominal points when Component 02 committed a constructed coherent realization;
- merge, weld, delete, move, alias, or reorder source identities because coordinates are equal, close, collinear, or project to equal nominal values;
- subdivide a source edge or introduce a Steiner point in V1;
- publish a triangle whose bounded orientation is not definitely accepted;
- label an internal diagonal as a source edge, source-feature relation owner, source-edge broad-phase feature, symbolic owner, or winding barrier;
- use user tolerance as a general triangulation epsilon or spend cleanup budget;
- include runtime owner tokens/pointers in semantic keys, canonical order, canonical bytes, digests, replay, diagnostics, or output;
- call legacy `YgorMeshesBoolean{,2,3,4,5}` code, `fv_surface_mesh::convert_to_triangles`, constrained-Delaunay, monotone-decomposition, generic contour cleanup, polygon simplification, or Component 12 output triangulation as the production provider;
- call legacy adaptive predicates directly rather than through Component 03;
- use `long double`, transcendental angle ordering, randomization, exceptions for expected failure, unchecked raw vector predicates, or implementation-defined container order in authoritative decisions;
- publish a facet, provisional identity, resource lease, digest, or artifact before the operand transaction commits; or
- use any external, vendored, downloaded, optional, subprocess, runtime-invoked, geometry, arithmetic, hashing, serialization, testing, fuzzing, or concurrency dependency.

For a facet with `n` retained positions, V1 success requires exactly `n - 2` definitely oriented triangles and `n - 3` facet-local internal diagonals. Failure is preferable to deleting a retained source vertex or inventing topology.

Use Component 01 for owner validation, strong-ID domains, checked arithmetic, outcomes/errors, resources, cancellation, diagnostics, replay, canonical bytes, SHA-256, transactions, deterministic arbitration, and immutable publication. Use Component 02 as the sole source of normalized topology, source identities, shell semantics, coherent geometry basis, plane/projection/orientation records, coverage certificate, and caller mappings. Use Component 03 for every bounded point, projection, orientation, segment relation, point relation, local-cone test, area enclosure, plane residual, feature bound, and conditioning decision.

`tracker.md` records completion of this planning/review step, not implementation completion. Mark Component 04 complete only after this reviewed specification and plan are mutually consistent with Components 01-03 and the downstream contracts of Components 05-10 and 15-17.

## 1. Independent review conclusions and required corrections

The prior plan had a strong provenance, coverage, mutation, resource, and test foundation. The independent review found four material integration defects and one underspecified optimization boundary.

### 1.1 Remove runtime owner from canonical semantics

Component 01 separates deterministic `context_digest` from runtime `context_owner_token`; owner pointer/token values never enter ordering, bytes, diagnostics, replay, or output. The prior Component 04 plan incorrectly placed `owner` in triangle, diagonal, and ear candidate keys and ambiguously included it in artifact bytes.

Correct rule:

- every handle and record is owner-validated at runtime;
- owner mismatch remains a deterministic typed failure;
- canonical semantic keys begin with stable operand/source identities and version tags, never owner;
- runtime owner fields are excluded from canonical bytes and all digests;
- two semantically identical invocations with different runtime owner anchors produce identical semantic/exact artifact bytes; and
- cross-owner handles still fail validation despite byte-identical semantic content.

### 1.2 Make Component 02's coherent realization the explicit triangulation geometry basis

Component 02 may publish `nominal_embedded` or `constructed_coherent_realization`. The prior plan reused Component 02's plane/projection but asked Component 03 to project only the source bounded points, leaving it ambiguous whether topology-affecting ear and coverage predicates operated on nominal points or the committed shared realization.

Correct rule:

- each facet record exposes one immutable `facet_geometry_basis_ref` bound to the operand coherent certificate;
- `nominal_embedded` references nominal source points and their inherited enclosures;
- `constructed_coherent_realization` references the exact one-per-source-vertex realized points committed by Component 02, together with containment and source-lineage proofs;
- all Component 04 triangulation and coverage predicates use the committed geometry basis;
- source nominal bits and source precision lineage remain preserved separately as source provenance;
- consuming the realized basis is validation against the accepted representative, not a source edit or cleanup action; and
- missing or inconsistent basis data is a predecessor invariant failure, never a silent fallback.

### 1.3 Exclude internal diagonals from source-edge relation enumeration

The prior plan correctly stated that internal diagonals are not source features, but it did not explicitly prohibit Component 06 from treating them as canonical edge features. That would make candidate/relation sets depend on the arbitrary source triangulation and weaken the broad plan's re-triangulation invariance.

Correct rule:

- Component 04 marks internal diagonals `source_edge_candidate_eligible = false` by schema;
- Component 05 may expose them for halfedge adjacency and triangle traversal only;
- Component 06's canonical source-edge-versus-triangle domain enumerates original source undirected edges, not facet-local diagonals;
- triangle-local hits always retain authoritative source-facet and source-feature lineage; and
- tests substitute alternative legal triangulations and require equivalent Components 05-15 semantics.

### 1.4 Require verifier-owned source-region witnesses

The prior plan used Component 02 validation-decomposition witnesses extensively and allowed Component 04 to construct fallback witnesses from Component 02 decomposition records. These witnesses are valuable predecessor evidence but cannot be the only independent coverage path, especially if Component 02 and Component 04 share bounded polygon primitives.

Correct rule:

- Component 02 exposes stable coverage evidence and optional source-region witnesses, not private triangle identity as authority;
- producer checks may consume those witnesses as supplemental evidence;
- the Component 04 verifier generates or selects its own deterministic witness set from immutable ring and geometry-basis records using independent traversal/control flow;
- producer and verifier never trust a predecessor or producer `coverage=true` boolean; and
- boundary equality, diagonal pairing, dual topology, pairwise embedding, interior classification, witnesses, and area agreement all remain mandatory.

### 1.5 Specify complete incremental ear invalidation

The prior optimized ear loop said to recompute `p`, `q`, and conservatively affected candidates but did not define the required dependency closure. Removing one ear can unblock a distant candidate because the removed vertex or segment was its blocker; the new diagonal can invalidate another cached candidate. Updating only neighboring ears can leave the eligible set incomplete and produce traversal-dependent output.

Correct rule:

- the serial semantic reference performs a full canonical candidate rescan after every removal;
- the unrestricted production provider may optimize only with explicit candidate dependency/blocker records;
- every candidate records the active vertices, active segments, local adjacency generations, and conservative query results on which eligibility depended;
- removal of a vertex/segments, addition of the new segment, or generation/query-membership change invalidates every dependent candidate;
- newly unblocked candidates are recomputed as well as newly blocked candidates;
- a differential trace compares every optimized step and final bytes with the full-rescan reference; and
- if the provider cannot prove complete invalidation, it must use the full rescan and enforce a deterministic ring-size/work limit.

## 2. Existing Ygor assessment and mandatory reuse decisions

### 2.1 Reuse unchanged only as narrow carriers

Reuse:

- `fv_surface_mesh<T,I>` only indirectly through Component 01/02 immutable artifacts;
- `vec2<T>` and `vec3<T>` only as nominal carriers at audited interfaces;
- fixed-width integers and C++17 standard-library containers/algorithms under Component 01 accounting; and
- Component 01/03 bounded-subsystem infrastructure.

Do not use raw vector arithmetic, equality, ordering, dot/cross helpers, normalized directions, Euclidean distances, angles, or legacy epsilon helpers for authoritative identity, ordering, or acceptance.

### 2.2 Reuse Component 02 topology and geometry evidence

Component 02 publishes, per facet:

- canonical source ring, corners, and directed boundary uses;
- source facet/ring/shell IDs;
- coherent geometry certificate and one `facet_geometry_basis_ref`;
- accepted support plane, projection frame, orientation mapping, simplicity, and area evidence;
- validation-only coverage evidence and stable witness records; and
- reversible caller-source maps.

Component 04 validates and consumes these records. It does not reverse rings, independently fit planes, select another projection, construct a different coherent realization, or treat private Component 02 decomposition triangles as Component 04 source triangles.

Where Components 02 and 04 require identical low-level bounded polygon formulas, factor or reuse `BoundedSourcePolygonKernel` as a pure Component 03 adapter. It may provide formula-dispatched orientation, segment relation, local-cone, point-in-triangle, and point-in-polygon predicates. It must not choose ears, assign IDs, own artifacts, choose witnesses, or share producer/verifier control flow.

### 2.3 Improve and reuse adaptive predicates only through Component 03

The existing `YgorMeshesAdaptivePredicates` mathematical approach is useful, but Component 04 must consume only Component 03's strict-target, enclosure-aware predicate capabilities. Exact stored-coordinate sign is supplemental evidence; it never overrides inherited uncertainty or authorizes a zero/uncertain ear.

### 2.4 Constrained Delaunay is not a production provider

`YgorMathConstrainedDelaunay.h/.cc` is an algorithmic and regression reference, but its current implementation is unsuitable because it:

- deduplicates vertices by coordinate keys;
- rejects coincident projected constraint endpoints;
- uses `long double` polygon areas;
- uses `atan2` adjacency ordering;
- lifts points through raw floating arithmetic and a convex-hull path;
- returns a generic mesh without source-edge/internal-diagonal provenance;
- uses nominal filtering and generic string diagnostics; and
- lacks owner/version validation, coherent geometry-basis references, bounded uncertainty, transactions, deterministic artifact schemas, replay, and independent coverage proof.

Do not adapt its public entrypoint or route Component 04 through a temporary public mesh. Implementation ideas may be re-expressed only when they satisfy the bounded identity and evidence contracts.

### 2.5 Monotone decomposition is not a production provider

`YgorMathMonotoneDecomposition.h/.cc` removes coordinate duplicates and collinear vertices, rejects reused coordinates, uses `long double` area and raw coordinate ordering, reverses polygons, and targets generic nested 2D loops. It is not compatible with immutable source identities or the committed Component 02 geometry basis.

Its sweep concepts may inform a future provider version, but V1 must not depend on or modify it to satisfy Component 04.

### 2.6 Generic mesh triangulation and cleanup are not providers

Do not call:

- `fv_surface_mesh::convert_to_triangles`;
- `remove_degenerate_faces`;
- `merge_duplicate_vertices`;
- `remove_disconnected_vertices`;
- `simplify_inner_triangles`;
- contour duplicate/extraneous-point removal;
- generic Delaunay/convex-hull triangulation; or
- Component 12's output polygon triangulator.

Those APIs do not preserve the required source identities, coherent geometry basis, boundary-use provenance, bounded evidence, or semantic/exact digest separation. Component 12 also handles post-Boolean cycles, constructed vertices, and holes; only pure low-level predicate adapters may eventually be shared.

## 3. Exact file and target layout

Add or complete under `src/YgorMeshesBooleanBounded/`:

- `SourceTriangleComplex.h` — immutable artifact schema and checked read-only views;
- `SourceTriangulationTypes.h` — policy enums, geometry-basis references, edge roles, dependency records, coverage witness types, and stable constants;
- `SourceTriangulation.h/.cc` — typed entrypoint and fixed phase orchestration;
- `SourceFacetGeometryBasis.h/.cc` — Component 02 coherent-basis validation and Component 03 projected workspace materialization;
- `BoundedSourcePolygonKernel.h/.cc` — narrow pure bounded predicate adapters shared with Component 02 where appropriate;
- `SourceEarTriangulationReference.h/.cc` — full-rescan serial semantic reference;
- `SourceEarTriangulationIndexed.h/.cc` — optional optimized provider with explicit dependencies/blockers and equivalence traces;
- `SourceTriangleProvenance.h/.cc` — edge-role reconstruction, mappings, and semantic lineage;
- `SourceTriangulationCoverage.h/.cc` — producer coverage checks and evidence;
- `SourceTriangleComplexCodec.h/.cc` — owner-free canonical encoding/decoding and digests;
- `SourceTriangleComplexVerifier.h/.cc` — independent reconstruction, witness generation, coverage verification, and mutation rejection.

Extend existing bounded-subsystem registries instead of creating parallel registries:

- `ContractVersions.h`;
- Component 01 stage/checkpoint and failure-subcode registries;
- resource/diagnostic/replay registries only for genuinely new kinds; and
- the strict bounded Boolean CMake target.

Add normative tests under `tests/mesh_boolean_bounded/`:

- `TestSourceTriangulationUnit.cc`;
- `TestSourceTriangulationGeometryBasis.cc`;
- `TestSourceTriangulationProjection.cc`;
- `TestSourceTriangulationCoverage.cc`;
- `TestSourceTriangulationDependencies.cc`;
- `TestSourceTriangulationBoundarySharing.cc`;
- `TestSourceTriangulationCanonicalization.cc`;
- `TestSourceTriangulationAlternatives.cc`;
- `TestSourceTriangulationMutation.cc`;
- `TestSourceTriangulationProperties.cc`;
- `TestSourceTriangulationAdversarial.cc`;
- `SourceTriangulationFixtures.h/.cc`;
- `SourceTriangulationExactOracle.h/.cc`; and
- `GoldenSourceTriangulationV1.h`.

Register separate CTest cases for unit, geometry-basis, projection, coverage, dependency/equivalence, boundary sharing, canonicalization, alternatives, mutation, properties/fuzz, and adversarial/resource suites. Apply the strict floating target to all production and normative-test units. Keep implementation-only nodes, heaps, dependency maps, indexes, and temporary handles out of installed/public headers.

## 4. Stable versions, stages, checkpoints, and failure subcodes

### 4.1 Version registry

Add explicit nonzero V1 values for:

- source-triangulation provider and policy;
- facet geometry-basis reference schema;
- projected workspace formula set;
- bounded source-polygon kernel formula set;
- ear eligibility and complete-invalidation policy;
- reference and optimized provider trace schemas;
- source triangle, edge-use, and internal-diagonal schemas;
- facet coverage and verifier-witness schemas;
- source triangle complex artifact;
- canonical encoding; and
- independent verifier.

Unknown required versions, reserved values, nonzero reserved fields, or predecessor formula/version mismatches are typed failures. Runtime owner validation metadata is versioned as a runtime handle contract but excluded from semantic encoding.

### 4.2 Fixed checkpoints

Use the Component 04 stage reserved by Component 01. Define stable checkpoints in this order:

1. capability, runtime owner, operand, type, and predecessor-version validation;
2. predecessor geometry eligibility and coherent-basis validation;
3. count, ID-capacity, byte, resource, and work preflight;
4. projected geometry-basis materialization;
5. active-ring and source-boundary lookup construction;
6. initial full candidate evaluation;
7. deterministic ear-removal loop;
8. final triangle closure;
9. final-triangle edge-role/provenance proposal reconstruction;
10. producer combinatorial coverage verification;
11. producer bounded embedding/interior/witness/area verification;
12. facet semantic and exact-layout canonicalization;
13. operand cross-facet boundary consistency and artifact assembly;
14. independent whole-artifact verification;
15. canonical encoding/digests/replay/resource reconciliation; and
16. final cancellation poll and atomic transaction commit.

Do not renumber released checkpoints.

### 4.3 Required subcodes

Allocate a disjoint Component 04 subcode range covering at least:

**Capability/predecessor**

- unsupported policy/provider/type/formula;
- wrong/stale runtime owner;
- wrong operand or predecessor artifact;
- predecessor digest/version mismatch;
- topology-only predecessor ineligible;
- coherent geometry-basis record missing/inconsistent;
- nominal basis used for constructed-realization predecessor;
- realized point/source-lineage mismatch;
- source count/range/byte/ID overflow;
- malformed facet/ring/corner/edge/shell reference.

**Projection/workspace**

- unsupported projection/formula;
- projected bounded point unavailable or non-finite;
- projection/orientation convention mismatch;
- active-ring link or generation corruption;
- repeated retained source vertex identity;
- source-boundary lookup mismatch.

**Ear provider**

- ear orientation uncertain/reversed;
- local-cone result uncertain/outside;
- diagonal relation uncertain/crossing/overlapping;
- another active vertex on diagonal;
- point-in-ear relation uncertain or blocking;
- no certified ear due to bounded uncertainty;
- no legal ear for certified simple geometry;
- incomplete candidate dependency closure;
- reference/optimized provider trace mismatch;
- ear work guard/resource exceeded;
- final triangle uncertain/reversed/degenerate.

**Provenance/coverage**

- triangle/diagonal/edge-use count mismatch;
- source boundary use missing/duplicated/reversed/wrong endpoint;
- internal diagonal missing partner/same direction/cross-facet/third use;
- source/internal role contradiction;
- internal diagonal source-edge-candidate eligibility set true;
- triangle provenance incomplete;
- triangle dual disconnected;
- triangle outside source region;
- forbidden triangle crossing/positive-area overlap;
- verifier witness uncovered/multiply covered/uncertain;
- conservative area mismatch;
- canonical key duplicate/collision with unequal full bytes;
- semantic digest depends on internal layout;
- runtime owner entered canonical bytes;
- verifier rejection or digest mismatch;
- partial/private state reached publication boundary.

Map expected geometry uncertainty to `geometric_condition_exceeds_tolerance` or the reviewed source-geometry category, never `internal_invariant_error`. Map representability to `index_overflow`, resource/work exhaustion to `resource_limit`, cancellation to `cancelled`, and committed predecessor/producer-verifier contradictions to `internal_invariant_error`.

## 5. Top-level API and capability boundary

Implement an internal entrypoint conceptually equivalent to:

```cpp
template<class T, class I>
boolean_outcome<artifact_handle<const source_triangle_complex<T,I>>>
triangulate_source_operand(
    const artifact_handle<const validated_operand<T,I>>& operand,
    const boolean_context_view<T,I>& context,
    const precision_context_view<T>& precision,
    const source_triangulation_capabilities<T,I>& capabilities);
```

The capabilities expose only narrow owner-checked services for:

- Component 01 resources, cancellation, transactions, deterministic diagnostics/errors, replay, canonical bytes, SHA-256, strong-ID publication, and serial/Component 17 execution scopes;
- Component 02 immutable topology, coherent geometry-basis, plane/projection/orientation, source coverage, and presentation lookup;
- Component 03 bounded point/projection/orientation/segment/point/area/plane/bounds/predicate evidence and verifier dispatch; and
- Component 17 private facet-task and canonical-merge hooks, initially backed by the serial reference.

Before facet access, validate runtime owner equality, operand role, versions, strict arithmetic profile, ordinary geometry eligibility, coherent certificate disposition, source policy, and transaction state.

Support an empty validated operand. Publish a verified empty artifact with owner-valid runtime handle metadata, owner-free canonical bytes, correct predecessor links, zero ranges, and deterministic digests.

## 6. Frozen V1 geometry and topology policy

### 6.1 Geometry basis record

Define:

```cpp
enum class source_geometry_basis_kind : std::uint8_t {
    nominal_embedded = 1,
    constructed_coherent_realization = 2
};
```

A `facet_geometry_basis_ref` stores or references:

- basis kind;
- operand coherent-certificate ID/digest;
- source facet/ring/shell IDs;
- one point reference per retained source vertex;
- for constructed realization, realized-point IDs and source-envelope containment proofs;
- support plane, projection frame, orientation mapping, and formula versions;
- source nominal point and precision-lineage references for provenance;
- accepted source orientation/area evidence; and
- an owner-free semantic encoding.

Runtime owner is validated through handles, not encoded.

All Component 04 topology-affecting predicates use the basis points. Triangle records continue to identify source vertices; they do not create replacement vertex topology or cleanup displacements.

### 6.2 No-Steiner/non-degenerate requirements

For every facet with `n >= 3`:

- every triangle vertex is an existing retained source vertex;
- every retained source vertex appears in at least one triangle;
- exactly `n - 2` triangles and `n - 3` diagonals are produced;
- every source boundary use appears exactly once;
- every other directed triangle edge belongs to one opposite diagonal pair;
- every triangle orientation is definite and source-consistent; and
- no fallback removes a retained source identity.

### 6.3 Source versus internal edge semantics

Use a closed role enum:

```cpp
enum class source_triangle_edge_role : std::uint8_t {
    source_boundary = 1,
    facet_internal_diagonal = 2
};
```

Schema semantics:

- `source_boundary`: `source_feature_eligible=true`, `source_edge_candidate_eligible=true`, exact source directed/undirected edge IDs required;
- `facet_internal_diagonal`: `source_feature_eligible=false`, `source_edge_candidate_eligible=false`, `symbolic_owner_eligible=false`, `classification_barrier=false`, no source edge IDs.

These fixed semantics should be encoded by role, not mutable booleans where possible. Any contradictory serialized field is invalid.

## 7. Immutable artifact schema

### 7.1 Artifact header and semantic separation

`source_triangle_complex<T,I>` contains:

- runtime owner validation metadata outside canonical semantic bytes;
- operand role, type profile, artifact/provider/policy/formula/codec/verifier versions;
- predecessor validated-operand semantic digest and coherent-certificate digest;
- precision-context qualification reference;
- canonical arrays/ranges and dense artifact-local ID descriptors;
- source-semantic digest;
- exact-triangulation digest;
- replay/presentation digest/reference;
- canonical artifact digest committing semantic/exact/version content but not runtime owner;
- geometry eligibility inherited from Component 02; and
- exact resources/work statistics.

No artifact field borrows mutable caller or transaction-local memory.

### 7.2 Source vertex reference

Publish one entry per referenced Component 02 source vertex containing:

- `source_vertex_id`, operand, and source shell membership;
- exact nominal coordinate bits or immutable predecessor reference;
- source bounded-point and precision-lineage reference;
- coherent realized-point reference when the certificate uses one;
- presentation/caller provenance;
- incident source facet/triangle mappings; and
- owner-free semantic contribution.

Equal coordinates never merge entries.

### 7.3 Source triangle record

Each triangle contains:

- artifact-local `source_triangle_id`;
- operand, source facet/ring/shell IDs;
- three distinct source vertex IDs in accepted cyclic orientation;
- orientation-preserving canonical rotation used as the semantic key;
- three corresponding edge-use records;
- geometry-basis, plane, projection, orientation, and conservative bound references;
- source/caller provenance; and
- owner-free canonical key/digest contribution.

Triangle key:

```text
(operand,
 source_facet_id,
 min_orientation_preserving_rotation(v0,v1,v2),
 source_triangulation_policy_version)
```

Do not include runtime owner, ear order, allocation order, task ID, heap order, or caller facet position.

### 7.4 Triangle edge-use record

Common fields:

- role;
- origin/destination source vertex IDs;
- incident source triangle ID/local slot;
- operand, source facet/ring/shell;
- role-specific semantic key; and
- geometry-basis reference.

Source boundary fields:

- source directed-edge ID;
- source undirected-edge ID;
- source corner/ring position;
- reciprocal source use reference;
- presentation provenance.

Internal diagonal fields:

- facet-local diagonal ID;
- ordered endpoint source IDs;
- opposite triangle use;
- fixed non-source/non-candidate/non-owner/non-barrier semantics;
- no source edge ID.

### 7.5 Internal diagonal record

Store:

- artifact-local diagonal ID;
- operand, facet/ring/shell IDs;
- ordered endpoint source IDs;
- exactly two opposite triangle uses and two distinct incident triangles;
- proof neither use matches a source boundary use;
- construction predicate/evidence references;
- fixed role semantics; and
- owner-free canonical key.

Diagonal key:

```text
(operand,
 source_facet_id,
 min(endpoint0,endpoint1),
 max(endpoint0,endpoint1),
 source_triangulation_policy_version)
```

### 7.6 Facet triangulation and coverage record

For each source facet store:

- source IDs and canonical ring/corner/directed-use sequences;
- geometry-basis, support-plane, projection, orientation, and source area references;
- canonical triangle and diagonal member lists/ranges;
- exact counts;
- producer coverage evidence;
- independent verifier disposition/reference;
- semantic digest independent of internal layout;
- exact layout digest;
- resources/work; and
- caller reverse-map references.

The semantic digest includes source ring/boundary/shell/geometry-basis policy but excludes triangle/diagonal IDs and choices. The exact digest includes the selected layout and its evidence.

### 7.7 Dependency/equivalence trace records

For the optimized provider, keep transaction-local and test-visible trace records describing:

- candidate semantic key;
- active predecessor/successor generations;
- queried active vertex IDs;
- queried active segment IDs/generations;
- blocker/result records;
- invalidation reason;
- re-evaluation generation; and
- reference-provider comparison disposition.

These traces are diagnostic/replay evidence according to policy, not semantic artifact identity. Production artifacts need not retain the full trace after successful equivalence qualification unless the frozen diagnostic policy requires it.

## 8. Count, capacity, and resource preflight

For each facet with `n` positions preflight with checked arithmetic:

- triangles `n - 2`;
- diagonals `n - 3`;
- triangle edge uses `3(n - 2)`;
- source boundary uses `n`;
- internal edge uses `2(n - 3)`;
- identity `3(n - 2) == n + 2(n - 3)`.

Accumulate operand totals and reserve for:

- source/realized projected point references;
- active-ring nodes and generation counters;
- full-rescan candidate records or optimized candidate/dependency/blocker/index records;
- predicate/evidence references;
- triangle/edge/diagonal/provenance proposals;
- producer and verifier witness/pair indexes;
- canonical keys, maps, bytes, digests, diagnostics, replay; and
- persistent artifact storage.

Reject before work if any total exceeds strong-ID domains, `I` representability required by later adapters, `size_t`, byte/entity/work limits, or configured ring-size limits. Never allocate from unchecked multiplication.

Charge abstract work for every candidate evaluation, predicate, query result, invalidation, rescan, triangle emission, coverage relation, verifier relation, sort record, and encoding block.

## 9. Geometry-basis validation and projected workspace

### 9.1 Validate predecessor records

For each facet verify:

- facet/ring/shell/operand IDs and ranges;
- canonical ring length and uniqueness;
- one corner and source directed use per position;
- exact edge endpoints and previous/next links;
- coherent-certificate disposition and digest;
- one basis point per source vertex;
- realized-point containment/source-lineage evidence where applicable;
- accepted source orientation, plane, projection, simplicity, and area evidence;
- required stable source-coverage records; and
- no topology-only or unsupported certificate disposition.

Any contradiction is a predecessor invariant failure. Do not reinterpret or repair.

### 9.2 Materialize projected bounded points

Request projected points from Component 03 using the exact basis point, stored plane/frame, formula ID, and precision lineage. Store nominal projected carrier bits, finite enclosures, predicate handles, source vertex identity, and basis lineage separately.

For a constructed realization, verify each projected basis point corresponds to the committed realized point and remains inside its source precision envelope. Do not project the raw nominal source point as the ear authority.

### 9.3 Active ring and source-boundary table

Use a private array indexed by canonical ring slot with source vertex/corner/directed-use IDs, projected point handle, active flag, previous/next active slot, monotonic generation, and candidate state.

Build an exact identity-keyed boundary table from Component 02 directed uses. Every original consecutive pair resolves to exactly one boundary use. A nonconsecutive pair is internal even if coordinates coincide.

## 10. Serial semantic reference: complete-rescan bounded ear clipping

### 10.1 Candidate definition

For active `c` with predecessor `p` and successor `q`, candidate `(p,c,q)` is eligible only when:

1. `p,c,q` are distinct source IDs;
2. bounded orientation is definite and source-consistent;
3. when active count exceeds three, `p-q` is a valid non-boundary diagonal;
4. the diagonal is definitely in the interior local cones at both endpoints;
5. it has no prohibited relation with nonincident active boundary segments;
6. no other active source vertex lies on its enclosure;
7. no other active source vertex is inside or on the closed ear triangle; and
8. all predicate/formula/basis/owner references validate.

Uncertain is not eligible. Exact nominal zero does not authorize an ear. Definite geometric rejection and bounded uncertainty are recorded separately.

### 10.2 Candidate semantic key

Use:

```text
(operand,
 source_facet_id,
 oriented_triangle_key(p,c,q),
 ordered_diagonal_key(p,q),
 source_corner_id(c),
 policy_version)
```

Compare full keys. Exclude runtime owner, nominal geometry values, active array position, task ID, insertion order, and caller positions.

### 10.3 Full-rescan workflow

The serial reference executes:

1. validate active links;
2. while active count exceeds three:
   - poll cancellation at the stable boundary;
   - evaluate every active node in canonical source vertex/corner order against the current complete ring;
   - collect every eligible candidate in a full-key ordered set;
   - retain canonical minimum uncertainty and definite-rejection witnesses;
   - if no eligible candidate and uncertainty blockers exist, return the canonical `no certified ear` geometry failure;
   - if no eligible candidate and only definite rejections exist for a predecessor-certified simple facet, return a provider/predecessor contradiction;
   - select the least key;
   - re-evaluate it once against unchanged current generations;
   - append a private triangle proposal and proposed diagonal;
   - remove the ear node atomically and increment affected generations;
   - validate link reciprocity and charged work;
3. validate/emit the final triangle;
4. require exact counts; and
5. retain proposals privately for role reconstruction and coverage.

This reference is the normative semantic oracle for provider equivalence. It may be quadratic and is restricted by explicit ring-size/work limits in ordinary production if no optimized provider is qualified.

### 10.4 Collinear and coincident identities

Never remove a source vertex due to collinearity. A retained collinear vertex must appear in the triangle complex. Clip other ears until it participates in a definitely oriented triangle. If every legal completion requires an uncertain/zero triangle, fail.

Within-facet nominal coordinate coincidences are eligible only under a Component 02 constructed coherent realization that certifies distinct geometry and positive source-edge extent. Component 04 uses that basis and preserves the nominal source provenance without merging identities.

## 11. Optimized provider and complete dependency closure

### 11.1 Correctness rule

The optimized provider must produce the exact same selected ear sequence, triangles, diagonals, evidence dispositions, failure key, canonical bytes, and digests as the full-rescan reference for every input within the jointly supported domain.

### 11.2 Candidate dependency record

For each evaluated candidate record:

- candidate semantic key and local vertex generations;
- predecessor/successor IDs;
- proposed diagonal key;
- every active segment considered or conservatively queried;
- every active point considered or conservatively queried;
- definitive blockers and uncertainty blockers;
- conservative index node/query generation where used;
- eligibility disposition and evidence handles.

Maintain reverse maps from active vertex, active segment, and index/query generation to dependent candidates. Hash lookup is permitted only with full identity equality and deterministic sorted publication/tracing.

### 11.3 Mutation invalidation closure

Removing ear `c` deletes active segments `p-c` and `c-q`, deletes active point `c`, and inserts active segment `p-q`. Invalidate:

- candidates at `p`, `c`, and `q`;
- every candidate depending on `c` as a tested point/blocker;
- every candidate depending on either removed segment;
- every candidate whose diagonal/triangle conservative query overlaps the new segment or whose index query membership changes;
- every candidate whose predecessor/successor generation changed;
- every candidate whose cached predicate input or formula evidence became stale.

Recompute all invalidated active candidates in canonical key order. Also create/evaluate candidates that were previously absent or ineligible and become possible after blocker removal. Before choosing the next ear, prove every active node has one current disposition record and every eligible record is present in the ordered set.

### 11.4 Equivalence gate

Tests run reference and optimized providers step-by-step. At every iteration compare active ring, current candidate disposition multiset, least candidate, selected evidence class, emitted proposals, and work-accounting contract. Any mismatch is an invariant failure and disqualifies the optimized provider version.

## 12. Final edge-role reconstruction and provenance

Do not trust ear-history labels. Reconstruct every directed edge from final triangle cycles and match it against the exact Component 02 source-boundary table.

- Exact directed match: `source_boundary` with complete source edge/corner provenance.
- Otherwise: group by `(source_facet_id, unordered endpoint IDs)` as `facet_internal_diagonal`.

Require every source directed use exactly once, every internal group exactly twice with opposite directions and different incident triangles, no third use, no cross-facet pairing, no source edge ID on internal roles, and exact expected counts.

Sort full owner-free triangle and diagonal keys, validate duplicates, assign provisional dense artifact-local IDs through the Component 01 transaction, then resolve edge-use references. No ID becomes externally visible before final commit.

## 13. Producer verification

### 13.1 Combinatorial disk proof

Reconstruct from final proposals:

- every ring vertex used;
- exact V/F/E counts and `3F` identity;
- one source boundary use per ring position in source direction;
- two opposite uses per diagonal;
- closed oriented triangle cycles;
- one connected triangle dual with `F-1` internal adjacencies;
- exact boundary cycle equality including corner/directed-use identity; and
- no unclassified edge.

### 13.2 Orientation and geometry-basis proof

Re-evaluate triangle orientations through a producer-verification formula path distinct from cached candidate objects. Require definite source-consistent sign, correct basis/plane/projection references, and source precision eligibility.

### 13.3 Pairwise embedding audit

Using a deterministic conservative triangle-bound index, enumerate all potentially interacting triangle pairs in canonical pair order and classify them under the coherent geometry basis.

Permit only:

- the full shared internal diagonal and endpoints for paired adjacent triangles;
- the exact shared source identity for triangles sharing one source vertex; and
- no contact for identity-disjoint triangle interiors/boundaries.

Reject positive-area overlap, extra crossing, same-direction shared edge, unauthorized coordinate-coincident contact, or unresolved relation.

### 13.4 Inside-source checks

Construct deterministic bounded interior samples for every triangle and internal diagonal through Component 03. Classify against the original source ring using a producer-side bounded winding/ray method that does not call ear eligibility helpers. Require definite inside, with a finite prescribed alternate sample sequence for boundary-overlap cases.

### 13.5 Source-region witness coverage

Consume Component 02 stable witness records as supplemental evidence. Additionally construct a producer-owned deterministic source-region witness set from immutable source-ring/basis data through a control path independent of selected ear history. Classify each witness against output triangles and require exactly one positive-area containing triangle or a documented legal internal-diagonal boundary classification.

### 13.6 Conservative area agreement

Compute source polygon and canonical pairwise-reduced triangle area enclosures through separately dispatched Component 03 formulas. Require the difference enclosure to contain zero under the lineage consistency rule. This is secondary evidence and cannot override any topological, pairwise, interior, or witness failure.

## 14. Canonical ordering, encoding, and digests

### 14.1 Canonical arrays

Order:

- source vertex references by source vertex ID;
- facet records by source facet ID;
- triangles by full owner-free triangle key;
- diagonals by full owner-free diagonal key;
- edge uses by triangle ID/local slot;
- maps by semantic domain key;
- evidence by facet/witness/pair key.

Never serialize unordered-container iteration.

### 14.2 Digests

Maintain:

1. `source_semantic_digest`: predecessor semantic content, source ring/boundary/shell semantics, geometry-basis policy/reference; excludes internal layout and runtime owner;
2. `exact_triangulation_digest`: selected triangles/diagonals and layout-specific evidence; excludes runtime owner;
3. `replay_presentation_digest`: caller-source and diagnostic replay content according to Component 01 policy;
4. `artifact_digest`: versions plus semantic/exact canonical sections, excluding runtime owner.

The codec must have an explicit negative test that changing only runtime owner leaves all canonical bytes/digests unchanged.

### 14.3 Alternative legal triangulation adapter

Provide a test-only builder accepting an independently supplied legal triangle set. It runs the same geometry-basis validation, role reconstruction, producer coverage, provisional ID assignment, codec, and independent verifier. It bypasses no invariant. Different layouts share the semantic digest but differ in exact digest when diagonals differ.

## 15. Independent `SourceTriangleComplexVerifier`

### 15.1 Independence boundary

The verifier receives immutable Component 02/03 predecessors, the proposal, and narrow Component 01 services. It must not call:

- ear candidate evaluation;
- active-ring mutation;
- optimized dependency/blocker logic;
- producer edge grouping or dual traversal;
- producer witness selection;
- producer cached coverage booleans/counts/digests; or
- producer ID assignment helpers.

It may share schemas, strong-ID types, owner-free canonical-byte primitives, and low-level Component 03 predicate dispatch.

### 15.2 Verifier workflow

Independently:

1. validate runtime owner through handles but prove owner is absent from canonical sections;
2. validate all versions, operands, predecessor digests, geometry-basis links, reserved fields, and dense ranges;
3. reconstruct each source ring and every triangle directed edge;
4. reclassify source boundary versus internal role from Component 02 identities;
5. rebuild boundary and diagonal groups;
6. reconstruct triangle dual and exact source boundary cycle;
7. recompute counts, Euler identities, and source vertex coverage;
8. re-evaluate triangle orientations and plane/basis consistency;
9. run an independent triangle-pair index and forbidden-interaction audit;
10. construct verifier-owned triangle/diagonal interior samples;
11. construct verifier-owned source-region witnesses from ring/basis records using independent control flow;
12. classify Component 02 supplemental witnesses where available;
13. recompute source and triangle area enclosures with verifier dispatch/reduction;
14. rebuild owner-free keys, arrays, maps, semantic/exact bytes, and digests;
15. compare all stored maps/evidence/digests against reconstruction;
16. verify internal diagonals are source-feature/source-edge-candidate/symbolic-owner ineligible and non-barriers;
17. verify resource statistics are conservative and no private handle escaped; and
18. return one deterministic verified disposition or typed rejection.

### 15.3 Mutation rejection

Reject every mutation required by the component specification, including owner-in-key/bytes, geometry-basis substitution, source/internal role changes, internal-diagonal candidate eligibility, incomplete maps, crossing/overlap with preserved area sum, witness manipulation, key/order/ID changes, predecessor digest changes, unknown versions, resource underreporting, and private-handle leakage.

## 16. Downstream contract for Components 05-10 and 15

Expose narrow immutable views for Component 05 to enumerate source vertices, triangles, facet groups, shell groups, edge uses, geometry bases, plane/orientation evidence, conservative bounds, semantic/exact digests, and provenance.

Component 05 must be able to pair:

- source boundary halfedges by Component 02 source undirected-edge ID; and
- internal halfedges by source facet plus diagonal ID.

Expose separate ranges:

- `source_edge_feature_range`: original source undirected edges only, eligible for Component 06 edge-versus-triangle enumeration;
- `facet_internal_diagonal_range`: bookkeeping adjacency only, ineligible as source features.

Components 05-10 must preserve authoritative source-facet/source-edge/source-vertex lineage. A relation discovered against a generated triangle may use the triangle ID for local acceleration/cache layout, but semantic ownership and event lineage cannot be keyed solely by triangle or internal-diagonal identity.

Component 15 must be able to reconstruct source boundary preservation, geometry-basis lineage, semantic/exact digest separation, and owner-free canonical encoding independently.

## 17. Resources, cancellation, transactions, and Component 17

Use Component 01 leases/reservations for all projected workspaces, ring/candidate/dependency/index state, predicates/evidence, proposals, canonical keys/bytes, persistent records, verifier work, diagnostics, and replay.

Poll cancellation at deterministic boundaries: facet start, projected-point chunks, full candidate scans, before each ear mutation, dependency invalidation batches, coverage pair/witness batches, canonical sort/encode boundaries, verifier batches, and immediately before commit. Do not poll mid-predicate or halfway through active-ring relinking.

Use one operand-stage transaction with private facet arenas/subtransactions. Facet success never publishes. Rollback releases all temporary state, provisional ID reservations, proposed leases, bytes, and private diagnostics except the selected canonical error/replay payload.

Keep the serial full-rescan semantic reference executable. Component 17 may run private facet tasks but workers receive immutable inputs and private outputs only. Canonical merge assigns final IDs in full-key order, reruns cross-facet source-edge consistency, invokes the verifier, and commits transactionally. Worker counts must produce byte-identical artifacts, digests, diagnostics, and failures.

## 18. Tests

### 18.1 Known answers and geometry-basis tests

Commit fixtures for triangles, convex/concave polygons, simultaneous ears, thin/narrow geometry, retained collinear chains, threshold ears, mixed scales, signed zero/subnormals, projection ties, nominally coincident identities with constructed realization, and expected predecessor/Component 04 failures.

For every success record canonical source ring, basis kind/reference, projection/formula IDs, selected ear sequence as diagnostic evidence, triangles/diagonals, edge roles, coverage summaries, semantic/exact digests, and golden bytes.

### 18.2 Owner separation tests

Run identical semantic inputs under distinct runtime context owners. Require identical keys, arrays, canonical bytes, digests, and deterministic diagnostics. Cross-owner handle use must fail. Mutate the codec or key builder to include owner and require test failure.

### 18.3 Dependency-closure tests

Include distant unblocking by removed point, distant unblocking by removed segment, invalidation by inserted diagonal, conservative query-membership changes, stale evidence generations, and multiple simultaneous eligibility changes. Compare each optimized iteration with the full-rescan reference. Mutation operators delete one reverse dependency or skip one invalidation and must be detected.

### 18.4 Coverage and boundary tests

Independently reconstruct every obligation in Sections 13 and 15. Include area-preserving gap/overlap mutations, disconnected triangle islands, wrong boundary directions, internal diagonal third use, coordinate-coincident identity-distinct unauthorized contact, and geometry-basis substitution.

Adjacent source-facet fixtures cover different projections, long edges, high valence, signed-zero endpoints, subnormal offsets, and presentation permutations; require exactly two opposite triangle boundary uses per source undirected edge.

### 18.5 Metamorphic and alternative triangulation tests

Apply ring/vertex/facet/shell permutations, exactly representable translation, power-of-two scale, axis permutation, corrected whole-shell orientation reversal, worker/task schedule permutation, and owner-anchor changes.

Enumerate legal alternative triangulations for small exact fixtures. Require semantic digest invariance, exact digest differentiation, Component 05 equivalent source-facet groups, Component 06 internal-diagonal exclusion, and equivalent Components 07-15 results.

### 18.6 Exact oracle, fuzzing, and shrinking

Use the Component 16/shared test-only arbitrary-precision integer/rational oracle for exact orientation, segment relations, legal triangulation enumeration, least-ear sequence, coverage, and witnesses. Compare every definite Component 03 result with exact truth.

Fuzz exact rational simple polygons mapped to supported `T`, coherent-realization perturbations, ring size/reflex/collinear/narrow-channel patterns, scale/exponent/projection ties, signed zero/subnormals, and uncertainty thresholds. Serialize and deterministically shrink every unexpected crash, hang, nondeterminism, false acceptance, wrong failure category, reference/optimized mismatch, producer/verifier disagreement, or exact-oracle disagreement.

### 18.7 Performance/resource/cancellation qualification

Track projected points, candidate evaluations, dependency edges, invalidations, rescans, predicates, pair/witness audits, triangles, diagonals, sort records, bytes, leases, and work.

Require linear persistent output storage, no accidental quadratic persistent memory, near-linear or `O(n log n)` ordinary optimized behavior, bounded reference-domain use, deterministic resource failure for adversarial cases, and byte identity across providers/workers.

Test limit-minus-one/exact/plus-one for ring size, facet count, projected points, candidate/dependency records, predicate evidence, pair/witness audits, IDs, verifier work, bytes, diagnostics, and replay. Inject cancellation/allocation/provider/codec failures at every checkpoint and prove zero partial publication or leaked reservations.

## 19. Implementation sequence and handoff gates

Implement in this order; each gate keeps the branch buildable and tested:

1. add Component 04 versions, roles, basis kinds, checkpoints, subcodes, and uniqueness tests;
2. add immutable schemas/read-only views with runtime owner validation separated from semantic encoding;
3. add exact count/capacity/resource preflight and empty artifact path;
4. add Component 02/03 capability views and coherent geometry-basis validation;
5. factor/audit pure bounded polygon adapters without changing Component 02 semantics;
6. add projected workspace and exact source-boundary lookup;
7. implement full-rescan serial ear candidate evaluation and least-key selection;
8. add triangles/convex polygons, then concave/reflex/collinear/thin/threshold cases;
9. reconstruct final edge roles and complete provenance from triangle cycles;
10. add combinatorial disk/boundary/dual producer verification;
11. add producer orientation, pairwise embedding, inside-source, independent witness, and area checks;
12. add owner-free key sorting, provisional ID assignment, facet/operand maps, and semantic/exact digests;
13. add canonical codec, golden bytes, replay, and explicit owner-exclusion tests;
14. implement the independent verifier with separate grouping/traversal/witness/index code;
15. add the complete mutation suite and require zero surviving required mutations;
16. add exact-oracle, deterministic fuzzing/shrinking, and permanent regressions;
17. add shared-source-edge and presentation/metamorphic tests;
18. add alternative-triangulation adapter and downstream semantic-invariance tests;
19. add explicit candidate dependency/blocker schemas and optimized provider;
20. prove step-by-step and byte-for-byte equivalence with the full-rescan reference;
21. add Component 17 private task/canonical merge hooks while retaining the serial reference;
22. integrate read-only Component 04 contracts into Component 05 test providers, including separate source-edge and internal-diagonal ranges;
23. run full Component 01-03 regressions under every supported strict build;
24. run debug/release, sanitizer, worker-count, resource, cancellation, replay, and fault-injection qualification matrices; and
25. mark Component 04 complete in `tracker.md` only after this specification/plan review is committed.

Do not weaken predecessor contracts, change legacy public triangulator behavior, or add an external dependency to satisfy a gate.

## 20. Definition of done

Component 04 is complete only when:

- scope is limited to source facet triangulation and provenance;
- the Component 02 coherent geometry basis is explicit, owner/version/digest validated, and used for all topology-affecting triangulation and coverage predicates;
- source nominal bits and precision lineage remain preserved without treating a constructed realization as cleanup or source topology mutation;
- runtime owner tokens remain necessary for handle safety but absent from all semantic keys, bytes, digests, replay, deterministic diagnostics, and output;
- V1 publishes exactly `n - 2` definite triangles and `n - 3` same-facet diagonals or a typed deterministic failure;
- every retained source vertex and source directed boundary use appears exactly as required;
- every internal diagonal has two opposite uses and fixed non-source/non-candidate/non-owner/non-barrier semantics;
- Component 06 cannot enumerate internal diagonals as source-edge features;
- the full-rescan serial reference is executable and authoritative;
- the optimized provider maintains complete dependency closure and is step-by-step/byte-for-byte equivalent;
- canonical IDs/arrays are full-key collision-safe and independent of source order, traversal, allocation, task, worker, and owner anchor;
- semantic digests are independent of legal internal triangulation while exact digests commit to the selected layout;
- producer and independent verifier reconstruct disk topology, boundary equality, diagonal pairing, orientation, pairwise embedding, source interior, verifier-owned witness coverage, Component 02 supplemental witness coverage, and conservative area agreement;
- every required mutation is deterministically rejected with the least canonical offending feature;
- alternative legal triangulations preserve Component 05-15 source-feature semantics and final Boolean topology;
- ordinary performance, adversarial bounded termination, linear persistent storage, and resource/cancellation/transaction/replay/codec/fault-injection gates pass;
- Component 01-03 regressions remain passing under strict targets; and
- all production and normative-test code is portable C++17, standard-library-only, and dependency-free.
