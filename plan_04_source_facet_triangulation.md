# Plan 04: Source Polygon Facet Triangulation and Provenance

## 0. Scope and non-negotiable constraints

Implement **only Component 04** from `component_04_source_facet_triangulation.md`. This component accepts one immutable, fully verified `validated_operand<T,I>` from Component 02 and publishes one immutable, independently verified `source_triangle_complex<T,I>` for that operand. It converts each accepted one-ring source facet into a deterministic no-Steiner triangle complex while preserving the exact source boundary, source orientation, shell membership, bounded planar region, and complete source-feature provenance.

This component must not:

- validate or repair arbitrary caller polygon soup;
- change Component 02 source vertex, corner, directed-edge, undirected-edge, facet, ring, or shell identities;
- merge, weld, delete, move, or alias distinct source vertices because coordinates are equal, close, collinear, or project to equal nominal values;
- subdivide a source edge or introduce a Steiner vertex in V1;
- publish a triangle whose bounded orientation is not definitely accepted under the source facet orientation;
- label an internal diagonal as a source edge or let it own source-feature contact semantics;
- infer adjacency, diagonal pairing, or provenance from coordinate equality or proximity;
- recompute a different authoritative source plane, source orientation, shell orientation, or source-edge pairing than Component 02;
- use user tolerance as a general-purpose triangulation epsilon;
- call `src/YgorMeshesBoolean{,2,3,4,5}*.{h,cc}`;
- call legacy `fv_surface_mesh::convert_to_triangles`, constrained-Delaunay, monotone-decomposition, contour cleanup, or polygon simplification APIs as the production provider;
- use `long double`, transcendental angle ordering, randomization, exceptions for expected failure, unchecked raw `vec2`/`vec3` predicates, or implementation-defined container order in authoritative decisions;
- use any external, vendored, downloaded, runtime-invoked, or optional triangulation, geometry, exact-arithmetic, hashing, serialization, testing, or concurrency dependency; or
- compile production or normative-test code outside the strict portable C++17 floating-point target established by Component 01.

V1 is a **no-Steiner, non-degenerate-output policy**. Every accepted facet with `n` retained ring positions must publish exactly `n - 2` definitely oriented triangles and exactly `n - 3` facet-local internal diagonals. If preserving every source boundary use would require a zero-area triangle, an uncertified diagonal, a topology edit, or a construction outside the inherited precision contract, fail with a typed deterministic geometry error rather than dropping a vertex or inventing a diagonal.

Use Component 01 for owner tokens, strong IDs, checked count/byte arithmetic, typed outcomes and errors, stages/checkpoints, resources, cancellation, diagnostics, transactions, replay, canonical bytes, SHA-256, deterministic failure arbitration, and immutable publication. Use Component 02 as the sole source of accepted normalized rings, source identities, edge pairing, shell membership, projection/plane/orientation evidence, and reversible caller maps. Use Component 03 for every topology-affecting projected coordinate, orientation, segment relation, point-in-triangle/polygon relation, area enclosure, plane residual, feature bound, and conditioning decision.

No failed, cancelled, partially triangulated, partially encoded, or verifier-rejected facet or operand may publish. Mark Component 04 complete in `tracker.md` only after Section 21 is fully satisfied.

## 1. Existing Ygor assessment and mandatory reuse decisions

### 1.1 Reuse unchanged only as narrow carriers

Reuse:

- `fv_surface_mesh<T,I>` only indirectly through the immutable Component 02 artifact; Component 04 must never reread caller mesh arrays as authority;
- `vec2<T>` and `vec3<T>` only as nominal coordinate carriers at audited interface boundaries;
- standard-library fixed-width integers, `std::array`, `std::vector`, `std::optional`, `std::variant`, `std::sort`, and deterministic heap/container machinery under Component 01 accounting; and
- all Component 01 and Component 03 contract, encoding, resource, bounded-arithmetic, and strict-target infrastructure already planned for the bounded Boolean subsystem.

Do not use raw `vec2`/`vec3` arithmetic, `operator==`, `operator<`, `Dot`, `Cross`, normalized directions, Euclidean distance, angle, or legacy epsilon helpers for an authoritative triangulation decision. Exact source IDs determine topology; Component 03 bounded predicates determine geometric eligibility.

### 1.2 Reuse Component 02 facet evidence instead of recomputing it

Component 02 already publishes, per normalized facet:

- the canonical orientation-preserving ring;
- one source corner and directed source-edge use per retained position;
- the accepted bounded support plane;
- a deterministic projection frame and orientation mapping;
- projected simplicity evidence;
- an accepted bounded orientation/area result; and
- validation-only decomposition evidence.

Component 04 must validate and consume these records. It must not independently choose a different plane, reverse a ring, or silently switch projection axes. The triangulation provider may request fresh projected bounded points and predicate evaluations from Component 03 using the stored frame/formula IDs, but those evaluations must reproduce the frozen Component 02 frame and accepted orientation convention.

Factor any pure bounded polygon helpers that Component 02's validation-only decomposition and Component 04 both require into a small internal `BoundedSourcePolygonKernel` with narrow inputs and no artifact ownership. The shared kernel may provide formula-dispatched projected orientation, segment relation, local-cone, and point-in-triangle queries. It must not choose ears, assign triangle IDs, publish provenance, or act as the independent Component 04 verifier. Keep Component 02 source-compatible and prove its regression suite remains unchanged.

### 1.3 Improve and reuse adaptive predicates only through Component 03

`YgorMeshesAdaptivePredicates` contains useful in-tree adaptive orientation machinery. Component 03 plans to place the audited exact-nominal expansion provider in the strict bounded target and expose enclosure-aware predicate results. Component 04 must use only that Component 03 capability. It must not call `orient_sign`, `incircle_sign`, or adaptive predicate functions directly.

Exact nominal sign is insufficient by itself: an ear, diagonal, triangle, or coverage relation is accepted only when the full bounded predicate disposition satisfies the prescribed Component 04 policy. Exact zero and uncertainty crossing zero remain distinct diagnostics.

### 1.4 Legacy constrained-Delaunay code is not a provider

`YgorMathConstrainedDelaunay.h/.cc` is useful as an algorithmic and regression reference, but its current implementation is fundamentally incompatible with Component 04 because it:

- deduplicates vertices by `(x,y)` coordinate keys;
- rejects or collapses coincident projected endpoints rather than preserving source identities;
- uses `long double` polygon areas;
- uses `atan2` to order adjacency;
- lifts points through raw floating arithmetic and a convex-hull path;
- returns a generic `fv_surface_mesh` without source-edge/internal-diagonal provenance;
- filters domains with nominal centroids and raw point-in-polygon tests;
- throws string diagnostics; and
- provides no owner/version checks, bounded uncertainty, transaction, replay, canonical artifact, or independent coverage proof.

Do not adapt its public function as a Component 04 entrypoint and do not route Component 04 through a temporary mesh. Isolated implementation ideas such as explicit constrained-edge bookkeeping, cavity guards, and post-triangulation edge maps may be re-expressed inside the bounded subsystem only when they use strong identities, Component 03 predicates, and the exact contracts below.

### 1.5 Legacy monotone decomposition is not a provider

`YgorMathMonotoneDecomposition.h/.cc` is also not suitable because it removes consecutive coordinate duplicates and collinear vertices, rejects reused coordinates, uses `long double` area and distance ordering, and is designed for generic nested 2D loops rather than one already validated source facet with immutable source identities. Its sweep and monotone-stack algorithms may be studied for a future provider version, but V1 must not depend on them or modify their behavior to satisfy Component 04.

### 1.6 Do not reuse generic mesh cleanup or triangulation

Do not call:

- `fv_surface_mesh::convert_to_triangles`;
- `remove_degenerate_faces`;
- `merge_duplicate_vertices`;
- `simplify_inner_triangles`;
- contour duplicate/extraneous-point removal;
- generic Delaunay or convex-hull triangulation; or
- any output-face triangulator planned for Component 12.

These APIs do not preserve the required source boundary-use identity, source-facet semantic lineage, or bounded evidence. Component 12 also has a materially different input contract: it triangulates post-Boolean polygonal cycles, possibly with holes and constructed vertices. Do not prematurely share an artifact schema or provider between Components 04 and 12. Pure predicate helpers may be shared later only after both contracts are preserved.

## 2. Exact file and target layout

Add these files under `src/YgorMeshesBooleanBounded/`:

- `SourceTriangleComplex.h` — immutable Component 04 artifact schema and checked read-only views;
- `SourceTriangulationTypes.h` — policy enums, edge-role records, temporary handles, coverage witness types, and Component 04 version constants;
- `SourceTriangulation.h/.cc` — top-level typed entrypoint and fixed phase orchestration;
- `SourceFacetProjection.h/.cc` — validation and materialization of Component 02 projection evidence through Component 03;
- `BoundedSourcePolygonKernel.h/.cc` — narrow pure bounded 2D predicate/formula adapters shared with Component 02 where appropriate;
- `SourceEarTriangulation.h/.cc` — deterministic no-Steiner ear-state machine and private facet result;
- `SourceTriangleProvenance.h/.cc` — boundary/internal edge labeling, canonical triangle/diagonal proposals, and reverse maps;
- `SourceTriangulationCoverage.h/.cc` — producer-side local coverage checks and evidence construction;
- `SourceTriangleComplexCodec.h/.cc` — canonical encoding/decoding and digest calculation; and
- `SourceTriangleComplexVerifier.h/.cc` — independent artifact reconstruction, coverage verification, and mutation rejection.

Extend existing bounded-subsystem registries rather than creating parallel registries:

- `ContractVersions.h` for all Component 04 schema/provider versions;
- Component 01 stage/checkpoint and error-subcode registries;
- Component 01 resource-kind/diagnostic/replay registries only where a genuinely new kind is required; and
- the strict bounded Boolean CMake target and strict floating-point compile helper.

Add tests under `tests/mesh_boolean_bounded/`:

- `TestSourceTriangulationUnit.cc`;
- `TestSourceTriangulationProjection.cc`;
- `TestSourceTriangulationCoverage.cc`;
- `TestSourceTriangulationBoundarySharing.cc`;
- `TestSourceTriangulationCanonicalization.cc`;
- `TestSourceTriangulationAlternatives.cc`;
- `TestSourceTriangulationMutation.cc`;
- `TestSourceTriangulationProperties.cc`;
- `TestSourceTriangulationAdversarial.cc`;
- `SourceTriangulationFixtures.h/.cc`;
- `SourceTriangulationExactOracle.h/.cc`; and
- `GoldenSourceTriangulationV1.h`.

Register separate CTest cases for unit/known-answer, projection, coverage, shared-boundary, canonicalization, alternative-triangulation, mutation, property/fuzz, and adversarial/resource suites. Apply the Component 01 strict floating-point target to every production and normative-test translation unit. Do not add network discovery or optional test dependencies.

Keep implementation-only ear nodes, active-ring links, candidate heaps, spatial indexes, and mutable maps out of installed/public headers. Templates must remain header-defined or be explicitly instantiated for the exact supported `float`/`double` and `uint32_t`/`uint64_t` combinations without accidental unsupported instantiation.

## 3. Stable versions, stages, checkpoints, and failure subcodes

### 3.1 Version registry

Add explicit nonzero V1 constants for:

- source-triangulation provider;
- source-triangulation policy;
- projected-facet workspace formula set;
- bounded source-polygon kernel formula set;
- ear eligibility/tie policy;
- source triangle record schema;
- source triangle edge-use schema;
- facet-local diagonal schema;
- facet triangulation/coverage record schema;
- source triangle complex artifact;
- source triangle complex canonical encoding; and
- source triangle complex verifier.

Zero is invalid/uninitialized. Unknown required versions, unsupported enum values, reserved bits set nonzero, or mismatched predecessor formula/version IDs are typed failures. Include all versions in artifact headers, per-facet policy records, canonical bytes, replay, diagnostics, and verifier checks.

### 3.2 Logical stage and fixed checkpoints

Use the Component 04 stage reserved by Component 01. Define stable checkpoints in this order:

1. capability, owner, operand, and predecessor-version validation;
2. operand/facet/ring count and representability preflight;
3. resource/work reservation;
4. facet projection-record validation and projected-point materialization;
5. active-ring and source-boundary proposal construction;
6. initial ear-candidate evaluation;
7. deterministic ear removal loop;
8. final triangle closure;
9. facet triangle/edge provenance proposal construction;
10. producer combinatorial coverage verification;
11. producer bounded geometric coverage verification;
12. facet canonical triangle/diagonal ordering and local digest;
13. operand artifact assembly;
14. independent artifact verification;
15. canonical encoding/digest/replay/resource finalization; and
16. pre-publication cancellation and transaction commit.

Do not renumber released checkpoints. Add future optional provider stages only in reserved gaps or under a new provider/schema version.

### 3.3 Required Component 04 subcodes

Allocate a disjoint Component 04 subcode range and define explicit values for at least:

- unsupported source-triangulation policy;
- wrong/stale context owner;
- wrong operand or predecessor artifact;
- predecessor version/formula mismatch;
- topology-only predecessor not eligible;
- source count/range/byte overflow;
- triangle or diagonal ID capacity exceeded;
- malformed normalized facet/ring reference;
- source corner or directed-edge map inconsistent;
- source shell/facet membership inconsistent;
- projection record missing or inconsistent;
- projected bounded point unavailable or non-finite;
- projected orientation convention mismatch;
- accepted source area/orientation evidence missing;
- active ring link corruption;
- repeated source vertex identity in normalized ring;
- boundary edge proposal mismatch;
- ear triangle orientation uncertain;
- ear triangle orientation reversed;
- candidate diagonal relation uncertain;
- candidate diagonal outside local cone;
- candidate diagonal intersects active boundary;
- candidate diagonal passes through another active vertex;
- point-in-ear relation uncertain;
- no certified ear due to bounded uncertainty;
- no legal ear for a supposedly valid simple facet;
- ear work guard exceeded;
- final triangle degenerate or uncertain;
- triangle count mismatch;
- internal diagonal count mismatch;
- source boundary use missing, duplicated, or reversed;
- internal diagonal use missing, duplicated, same-direction, or cross-facet;
- triangle provenance incomplete;
- source feature mislabeled as internal or vice versa;
- facet triangle dual disconnected;
- triangle interior definitely outside source facet;
- triangle pair has forbidden crossing or positive-area overlap;
- source boundary cancellation mismatch;
- coverage witness uncovered or multiply covered;
- conservative area agreement failed;
- canonical triangle/diagonal key collision;
- source triangle complex verifier rejection;
- source triangle complex digest mismatch; and
- partial facet result reached publication boundary.

Map predecessor owner/version/schema contradictions and producer/verifier contradictions to `internal_invariant_error`. Map bounded orientation/diagonal/coverage uncertainty that cannot be certified within inherited precision to `geometric_condition_exceeds_tolerance` unless Component 01 defines a more specific source-geometry category. Map representability to `index_overflow`, resource/work exhaustion to `resource_limit`, and cancellation to `cancelled`. Never convert an expected uncertifiable polygon into `internal_invariant_error` merely because the nominal coordinates look triangulable.

## 4. Top-level API and capability contract

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

`source_triangulation_capabilities` must expose only narrow owner-checked services for:

- Component 01 resources, cancellation, transactions, diagnostics, replay, canonical bytes, SHA-256, deterministic failure arbitration, and strong-ID publication;
- Component 02 immutable source vertex/facet/ring/corner/edge/shell lookup and validation-only facet evidence;
- Component 03 projected bounded points, bounded 2D orientation, segment relation, point relation, area reduction, plane/residual validation, conservative triangle bounds, and predicate evidence encoding; and
- Component 17 private facet-task creation/canonical merge hooks, implemented initially by a serial semantic reference.

Before reading facet records, validate:

- context and artifact owner equality;
- operand role equality;
- expected predecessor artifact and provider versions;
- source triangulation policy is the frozen V1 no-Steiner policy;
- strict arithmetic and Component 03 provider/formula IDs match the frozen context;
- the Component 02 artifact is fully geometry-validated and eligible for the ordinary Boolean pipeline;
- the transaction is open for the correct Component 04 stage; and
- no output ID domain is already populated for this operand/stage.

The function returns exactly one verified immutable artifact or one typed error. It never mutates Component 02/03 artifacts, never exposes private facet work, never logs an expected failure as control flow, and never throws an expected geometry/resource error.

Support an empty validated operand. It produces a verified empty `source_triangle_complex` with valid versions, owner, operand, predecessor digest links, zero entity ranges, canonical bytes, and deterministic digest. Empty operands still pass owner/version/resource/codec/verifier checks.

## 5. Frozen V1 policy

Define and encode one V1 policy with these exact semantics:

- one simple source ring per facet;
- no source-edge subdivision;
- no Steiner vertices;
- no source vertex removal or merge;
- no degenerate published triangle;
- every triangle vertex is an existing canonical source vertex;
- every source boundary directed use appears exactly once as a triangle edge in the same direction;
- every other triangle edge is a facet-local internal diagonal with exactly two opposite uses;
- an ear is eligible only when all required bounded predicates are definite;
- exact nominal ties do not authorize a diagonal or orientation by themselves;
- among all currently eligible ears, choose the least complete V1 ear key;
- internal diagonal and triangle IDs are assigned only after the facet result passes producer coverage checks;
- per-facet work either completes fully or fails transactionally; and
- the operand artifact is published only after all facets complete and the independent verifier accepts the whole artifact.

Do not add a “best effort,” “fan triangulation,” “ignore collinear vertex,” “nominal fallback,” or “use legacy triangulator” option. Future provider versions may add deterministic monotone decomposition, bounded Steiner points, or explicit degenerate bookkeeping records only with new policy/schema versions and explicit downstream contracts.

## 6. Immutable artifact schema

### 6.1 Artifact header

`source_triangle_complex<T,I>` must contain:

- artifact, provider, policy, predicate-formula, encoding, and verifier versions;
- context owner and operand ID;
- predecessor `validated_operand` semantic digest and replay/source digest references;
- Component 03 precision-context/artifact qualification reference;
- canonical arrays/ranges described below;
- dense strong-ID range descriptors;
- total resource/work statistics;
- semantic facet digest, exact triangulation digest, replay digest, and canonical artifact digest;
- geometry-validation eligibility/status inherited from Component 02; and
- immutable predecessor handles or owner-checked references whose lifetime covers this artifact.

No artifact field may borrow mutable caller memory or transaction-local workspace.

### 6.2 Source vertex reference table

Publish one canonical reference entry for every Component 02 source vertex that appears in a facet ring. Each entry contains:

- canonical `source_vertex_id`;
- operand and owner;
- exact nominal coordinate bits or an immutable predecessor reference;
- Component 03 bounded-point/ledger reference;
- canonical incident source-facet range inherited from Component 02;
- caller reverse-map reference for diagnostics; and
- canonical digest contribution.

Do not duplicate a source vertex merely because several source triangles reference it. Do not include isolated non-semantic source vertices in triangle topology ranges, but retain operand-level predecessor linkage so diagnostics can still reach them.

### 6.3 Source triangle record

Each triangle record contains:

- canonical `source_triangle_id` assigned after verification;
- context owner and operand;
- canonical `source_facet_id`, `source_ring_id`, and `source_shell_id`;
- exactly three distinct canonical `source_vertex_id` values in accepted cyclic orientation;
- the orientation-preserving canonical rotation of that cycle used as the triangle key;
- exactly three triangle-edge-use records in corresponding local slots;
- accepted bounded projected orientation evidence;
- reference to the source facet plane/projection/orientation evidence;
- conservative Component 03 triangle bound;
- per-triangle source/caller provenance references;
- canonical key bytes/digest contribution; and
- no mutable adjacency or pointer field.

The triangle ID key is:

```text
(owner, operand, source_facet_id,
 min_orientation_preserving_rotation(v0,v1,v2),
 source_triangulation_policy_version)
```

Compare full key bytes. SHA-256 may accelerate lookup but never defines equality. A triangle key must not contain ear removal order, allocation order, task number, heap insertion order, or caller facet ordinal.

### 6.4 Triangle edge-use record

Use a closed tagged representation:

```cpp
enum class source_triangle_edge_role : uint8_t {
    source_boundary = 1,
    facet_internal_diagonal = 2
};
```

For every directed triangle edge `u -> v`, store common fields:

- role;
- origin and destination `source_vertex_id`;
- incident `source_triangle_id` and local slot;
- source facet/ring/shell/operand/owner; and
- canonical role-specific key.

For `source_boundary`, additionally store:

- exact Component 02 `source_directed_edge_id`;
- exact Component 02 canonical `source_undirected_edge_id`;
- source corner/ring position that emits the use;
- reciprocal source directed-edge use on the adjacent source facet; and
- caller edge/ring-position provenance.

For `facet_internal_diagonal`, additionally store:

- `source_facet_diagonal_id`;
- ordered endpoint pair by source vertex ID;
- opposite triangle edge-use identity or canonical proposal key until final IDs are assigned;
- a mandatory `source_feature_eligible = false` semantic bit fixed by schema, not mutable data;
- a mandatory `classification_barrier = false` semantic bit fixed by schema; and
- no source directed-edge or source undirected-edge ID.

Unknown roles and contradictory role-specific fields are verifier failures.

### 6.5 Facet-local internal diagonal record

Each internal diagonal record contains:

- canonical `source_facet_diagonal_id`;
- owner, operand, source facet, ring, and shell;
- ordered endpoint source vertex IDs;
- exactly two opposite directed triangle edge uses;
- exactly two incident source triangles;
- proof that neither use is a Component 02 source boundary use;
- bounded projected segment relation/inside evidence used during construction;
- `source_feature_eligible = false` and `classification_barrier = false` schema semantics;
- canonical key and digest contribution.

The canonical diagonal key is:

```text
(owner, operand, source_facet_id,
 min(endpoint0, endpoint1), max(endpoint0, endpoint1),
 source_triangulation_policy_version)
```

Assign dense diagonal IDs by sorting full keys after producer coverage verification. The same endpoint pair cannot appear twice in one simple facet triangulation.

### 6.6 Facet triangulation record

For every accepted source facet publish:

- source facet/ring/shell/operand/owner;
- retained ring length and canonical source vertex/corner/directed-edge sequences;
- inherited support-plane and projection-frame references;
- accepted source orientation and bounded source area evidence;
- contiguous or explicit canonical member triangle range/list;
- canonical member internal-diagonal range/list;
- exact triangle count `n - 2` and diagonal count `n - 3`;
- producer coverage evidence record;
- independent verifier disposition/reference;
- facet semantic digest independent of internal triangulation;
- exact triangulation digest including triangles and diagonals;
- resource/work statistics; and
- caller facet/ring reverse-map references.

The facet semantic digest must encode the authoritative source facet identity, source ring, source boundary uses, plane/projection/orientation policy, and shell provenance, but **exclude** triangle IDs, diagonal IDs, ear order, and exact internal diagonal choices. The exact triangulation digest includes those details. Downstream semantic ownership must use the source facet digest/ID, not the exact triangulation digest.

### 6.7 Coverage evidence record

Per facet store bounded, reconstructible evidence for:

- expected and observed `V`, `E_boundary`, `E_internal`, `F`, and directed edge-use counts;
- source-boundary directed-use multiset digest;
- internal-diagonal opposite-use multiset digest;
- triangle-dual connectivity root and canonical traversal digest;
- every triangle's accepted orientation predicate reference;
- conservative source polygon area enclosure;
- deterministic pairwise-reduced triangle area enclosure;
- area-difference enclosure;
- triangle-pair candidate count and forbidden-intersection audit digest;
- triangle interior witness classifications against the source ring;
- Component 02 validation-decomposition witness coverage classifications where available;
- worst conditioning/uncertainty witnesses; and
- producer coverage disposition.

Evidence is not a producer boolean alone. Every count, digest, predicate reference, and witness must be independently reconstructible from predecessor and triangle records.

### 6.8 Operand-level mappings and ordering

Publish:

- source facet to triangle range/list;
- source triangle to source facet/shell;
- source vertex to incident triangle list or canonical range index;
- source directed-edge use to exactly one triangle boundary edge use;
- source undirected edge to the two triangle boundary uses inherited from its two source facets;
- facet-local diagonal to its two triangle uses;
- triangle local edge slot to role-specific provenance;
- caller facet/ring/vertex positions to canonical triangle provenance where applicable; and
- canonical-order permutations if physical arrays are not already in canonical order.

All maps are immutable, owner-checked, total over their documented domains, range-checked, and independently verified. Hash iteration order must never enter publication.

## 7. Count, capacity, and resource preflight

For each facet with `n` retained ring positions, use checked arithmetic to preflight exactly:

- triangles: `n - 2`;
- internal diagonals: `n - 3`;
- triangle edge uses: `3 * (n - 2)`;
- source boundary edge uses: `n`;
- internal diagonal edge uses: `2 * (n - 3)`; and
- identity `3(n - 2) == n + 2(n - 3)`.

Accumulate operand totals with Component 01 checked helpers. Also preflight and reserve:

- projected bounded-point references;
- active-ring nodes and generation counters;
- candidate-ear heap entries, including a conservative stale-entry allowance;
- segment/vertex conservative index nodes;
- predicate/evidence references;
- temporary triangle and edge-use proposals;
- sort keys and canonical bytes;
- producer coverage pair candidates;
- independent verifier workspaces;
- diagnostics and replay payloads; and
- persistent artifact storage.

Do not allocate based on unchecked `size_t` multiplication. Reject before work when any total exceeds the strong-ID ordinal domain, `I` representability required by later adapters, host `size_t`, byte limits, entity limits, abstract work limits, or configured facet-ring limits.

Reserve work in deterministic blocks. At minimum charge:

- one base unit per ring position materialized;
- one unit per candidate orientation/local-cone predicate;
- one unit per candidate diagonal/active-edge relation;
- one unit per point-in-ear relation;
- one unit per heap pop/revalidation;
- one unit per emitted triangle/edge-use;
- one unit per producer coverage pair relation;
- one unit per verifier relation; and
- encoding/digest work by canonical byte block.

A pathological polygon must terminate through typed work/resource failure. Do not rely on an ad hoc loop counter disconnected from Component 01 accounting. A defensive loop guard may exist, but reaching it is an invariant error only after the charged theoretical maximum for the chosen algorithm has been validated.

## 8. Projection and ring workspace

### 8.1 Validate inherited facet evidence

For each facet, verify before triangulation:

- facet/ring/shell IDs belong to the operand and current owner;
- the ring has at least three retained positions and no repeated source vertex ID;
- one corner and one source directed-edge use exist per position;
- each directed use is `ring[i] -> ring[(i+1)%n]`;
- previous/next corner and edge-use links agree;
- the stored source orientation is definite;
- the projection record's axis/formula/orientation mapping is supported by Component 03; and
- the stored source plane, projected simplicity, and area evidence are present and digest-consistent.

A contradiction is a predecessor invariant failure. Do not reinterpret the ring to continue.

### 8.2 Materialize projected bounded points

Request one immutable projected bounded point per ring position from Component 03 using:

- the exact source bounded-point reference;
- the stored source plane and projection frame;
- the frozen projection formula ID; and
- the source precision lineage.

Store both the nominal projected `vec2<T>` carrier and its bounded enclosure/predicate handle. Preserve source vertex identity separately; equal nominal `(x,y)` values do not collapse entries or keys.

Validate that Component 03 reproduces the stored orientation mapping and that projected points remain finite/representable. If the projection cannot support required finite bounded predicates, return a typed geometry failure with facet, ring positions, axis/formula, bounds, and replay evidence.

### 8.3 Active-ring representation

Construct a private array indexed by canonical ring slot. Each node stores:

- source vertex/corner/directed-edge identities;
- immutable projected bounded-point handle;
- active flag;
- previous and next active slot;
- monotonic generation counter;
- cached candidate disposition/evidence handle; and
- no semantic ID based on its array position.

Initialize links as the canonical Component 02 ring cycle. Validate reciprocity before candidate evaluation. Removal only toggles one active node and relinks its two neighbors in private state. Source records remain immutable and are never deleted.

### 8.4 Source-boundary lookup

Build an identity-keyed map from ordered directed endpoint pair plus source facet/corner to the exact Component 02 directed edge-use. This map is used only to label triangle edges. Full identity keys determine equality; a hash may accelerate lookup but collisions must compare full keys.

Every original consecutive ring pair must resolve to exactly one source boundary use. Any nonconsecutive pair is not a source boundary, even when coordinates coincide or the same unordered endpoint pair appears in another facet.

## 9. Deterministic bounded ear triangulation

### 9.1 Ear candidate definition

For an active node `c` with active predecessor `p` and successor `q`, define candidate triangle `(p,c,q)` in the current source-ring direction. The candidate is eligible only when all of the following are definite:

1. `p`, `c`, and `q` are three distinct source vertex identities;
2. bounded projected orientation has the same sign as the accepted source facet orientation and excludes zero;
3. for active ring size greater than three, proposed diagonal `p-q` is not an existing source boundary edge of the current original ring;
4. the diagonal lies strictly in the polygon's interior local cones at `p` and `q` under the prescribed bounded formula set;
5. the diagonal has no prohibited relation with any active boundary segment not incident to `p` or `q`;
6. no other active source vertex lies on the diagonal enclosure;
7. no other active source vertex lies in or on the candidate ear triangle under the closed ear-exclusion convention; and
8. all required predicates are owner/version valid and their uncertainty is within the inherited source precision contract.

For the final three active nodes, no new diagonal is proposed; require only distinct identities, accepted definite orientation, and consistent existing boundary/diagonal labels.

An exact nominal zero with a bounded interval containing zero is not eligible. A definite opposite sign is reversed. An uncertain relation is recorded as a blocked-by-uncertainty candidate. A definite crossing/outside/contained-vertex relation is recorded as geometrically ineligible.

### 9.2 Local-cone test

Use an orientation-only local-cone test parameterized by accepted ring orientation. At a convex active vertex, the candidate ray must be definitely between the two incident rays. At a reflex or certified collinear active vertex, use the complement form and require the candidate not to lie in the definitely exterior wedge.

Use fixed formula IDs and a deterministic alternate formulation from Component 03 only when the primary bounded result is uncertain. If both remain uncertain, the candidate is not eligible. Do not use angle, normalized direction, slope division, or nominal midpoint as authority.

### 9.3 Active boundary relation test

Maintain a deterministic conservative 2D closed-AABB index over active boundary segments. It may retain inactive entries and filter them by generation/active flags, or rebuild at provider-specified deterministic thresholds. Its only correctness requirement is no false negative; exhaustive tests compare it with all active segments.

For diagonal `p-q`, enumerate candidate active segments in full canonical segment-key order. Skip only segments incident to `p` or `q` by source identity. Classify each through Component 03:

- definite disjoint: continue;
- authorized shared endpoint: only possible for skipped incident segments;
- proper crossing: candidate ineligible;
- collinear overlap or touch at a non-endpoint active vertex: candidate ineligible;
- uncertainty after the prescribed alternate formulation: candidate blocked by uncertainty.

No tolerance-expanded “almost disjoint” result is allowed.

### 9.4 Ear interior exclusion

Build a deterministic conservative point index over active projected point enclosures. Query the candidate triangle bound and examine returned active vertices in canonical source vertex order. Skip `p`, `c`, and `q` by identity.

Classify each remaining point against the closed candidate triangle using three bounded oriented-edge tests under the accepted orientation. A point definitely outside at one edge is excluded. A point definitely inside, on any triangle edge, or overlapping the diagonal enclosure makes the candidate ineligible. If classification remains uncertain after the one prescribed alternate formula set, mark the candidate blocked by uncertainty.

This closed exclusion convention prevents a diagonal from passing through a retained collinear boundary vertex and prevents overlapping ears. Coordinate equality never substitutes for identity checks.

### 9.5 Candidate key and selection

For every eligible candidate construct the complete key:

```text
(owner, operand, source_facet_id,
 oriented_triangle_key(p,c,q),
 ordered_diagonal_key(p,q),
 source_corner_id(c),
 policy_version)
```

`oriented_triangle_key` is the least orientation-preserving rotation of `(p,c,q)`. `ordered_diagonal_key` is the sorted endpoint pair. Compare full strong-ID/key tuples; do not use nominal geometry, current active-ring position, heap insertion sequence, or task order.

Maintain a min-heap of candidate records with node generation snapshots. Stale entries are discarded deterministically. Before selection, revalidate the popped candidate against current generations and predicates. The selected ear is always the least complete key among all currently eligible candidates.

To prove the heap has not hidden a smaller candidate, either:

- ensure every active node has exactly one current heap record and heap ordering uses the full key; or
- at each removal, maintain an explicit ordered eligible set keyed by the full candidate key.

Prefer the ordered-set design in the serial reference for simpler proof. A later optimized provider may use a heap only after equivalence tests against the serial ordered set.

### 9.6 Ear removal loop

Use this exact serial semantic workflow:

1. evaluate every active node in canonical source vertex/corner order;
2. insert all eligible candidates into the ordered candidate set;
3. while active count is greater than three:
   - poll cancellation at the stable loop boundary;
   - if the eligible set is empty, perform one complete canonical rescan of all active nodes to rule out stale bookkeeping;
   - if still empty and at least one candidate is blocked only by bounded uncertainty, return `no certified ear due to bounded uncertainty` using the canonical minimum uncertainty witness;
   - if still empty with only definite geometric rejections, return `no legal ear for a supposedly valid simple facet` as a predecessor/provider contradiction with complete evidence;
   - select the least candidate key;
   - revalidate it against current active state;
   - emit one private triangle proposal `(p,c,q)` with predicate evidence;
   - record proposed diagonal `p-q` when active count before removal is greater than three;
   - remove `c` from the private active ring;
   - invalidate candidate records for `p`, `c`, `q`, and any node whose conservative query set changed according to the index provider;
   - recompute at minimum `p` and `q`, plus all conservatively affected candidates, in canonical order;
   - validate active-link reciprocity and charged work; and
   - continue.
4. validate and emit the final three-node triangle;
5. require emitted triangle count `n - 2`; and
6. leave all proposals private for provenance and coverage validation.

The simplest correct V1 implementation may conservatively recompute all active candidates after each removal. This is acceptable for the serial reference but must be work-accounted and pass adversarial limits. The production provider should then add the conservative segment/point indexes and affected-candidate invalidation while retaining byte-for-byte equivalence to the serial reference. Do not weaken predicates to meet a performance target.

### 9.7 Collinear and coordinate-coincident identities

Never remove a source vertex because a bounded corner orientation is zero or uncertain. A collinear source vertex remains in the active ring and must appear in published triangles so both adjacent source boundary uses survive.

The algorithm may clip ears elsewhere until the collinear vertex participates in a definitely oriented triangle. If the final active state or all legal choices would require a zero/uncertain triangle, fail under the V1 non-degenerate-output policy. The error must name every retained source identity involved and prove no topology edit was performed.

Distinct source IDs with equal nominal projected coordinates remain separate nodes and separate provenance. If Component 03 can still certify a valid bounded triangulation from inherited evidence, publish it without merging. Otherwise return a typed geometry failure. Never choose one identity as a coordinate representative.

### 9.8 No Delaunay legalization in V1

Do not perform post-ear edge flips or Delaunay legalization in V1. They are unnecessary for source-facet semantic correctness, introduce additional tie policy, and complicate canonical provenance. The frozen least-ear policy uniquely determines the exact triangulation when all required predicates are definite.

A future provider may add deterministic quality optimization only under a new policy version, while preserving source boundary and semantic digest invariance and proving downstream Boolean equivalence.

## 10. Provenance construction and canonical ID assignment

### 10.1 Private proposal labeling

After ear generation, classify every directed triangle edge `u -> v` by identity:

- if it matches the exact Component 02 directed source-boundary use for the facet and ring, label it `source_boundary` with that complete provenance;
- otherwise label it `facet_internal_diagonal` with proposed key `(source_facet_id, min(u,v), max(u,v))`.

Do not decide this from whether the edge was created during a particular ear removal; reconstruct it from the final triangle edge multiset and Component 02 boundary-use table.

### 10.2 Boundary-use requirements

Sort all proposed source-boundary triangle uses by exact `source_directed_edge_id`. Require:

- every facet ring directed edge use appears exactly once;
- triangle direction equals the source ring direction;
- endpoints equal the Component 02 source IDs;
- facet/ring/shell/operand/owner agree;
- no internal edge carries a source directed-edge ID; and
- the adjacent source facet's reciprocal directed use remains referenced through Component 02 but is not paired inside this facet result.

Across the completed operand artifact, each Component 02 source undirected edge must therefore have exactly two triangle boundary uses, one from each source facet, with opposite endpoint direction. Component 04 records this cross-facet consistency but leaves halfedge pairing to Component 05.

### 10.3 Internal-diagonal requirements

Group all non-boundary directed triangle edges by full facet-local unordered endpoint key. Require each group to contain exactly two uses with:

- opposite directions;
- two distinct incident triangle proposals;
- the same source facet/ring/shell/operand/owner;
- no Component 02 source edge identity;
- no third use; and
- endpoints that are nonconsecutive in the original source ring.

Require exactly `n - 3` groups. Sort complete keys and assign dense `source_facet_diagonal_id` values through Component 01 only after the facet passes producer coverage verification.

### 10.4 Triangle canonicalization

Construct complete triangle keys from Section 6.3, sort them, reject duplicates, and assign dense `source_triangle_id` values through Component 01 only after facet coverage verification. Then resolve every triangle edge-use incident triangle ID and internal diagonal opposite-use reference.

If IDs are globally dense per operand, merge facet-local sorted proposal streams by full triangle/diagonal key in canonical facet order. Do not let parallel completion order assign IDs. The serial reference and Component 17 merge must produce identical IDs and bytes.

### 10.5 Total reverse provenance

For every triangle and edge use, retain enough references to diagnose:

- operand, shell, source facet, ring, source vertices, and source corners;
- source boundary directed/undirected edge when applicable;
- facet-local diagonal and opposite triangle use when applicable;
- caller facet index, caller vertex indices, and original ring positions through Component 02 maps;
- predicate formula IDs and worst bounded evidence; and
- exact triangulation/replay policy versions.

No caller ordinal may influence semantic canonical ordering, but all relevant caller locations must remain available in diagnostics and replay.

## 11. Producer-side facet verification

Complete all checks below before assigning final persistent IDs or exposing a facet result.

### 11.1 Combinatorial identities

Reconstruct from triangle proposals, not ear-state counters:

- `V == n` distinct source vertex IDs used and every ring vertex appears;
- `F == n - 2` triangles;
- source boundary groups `== n`;
- internal diagonal groups `== n - 3`;
- directed triangle edge uses `== 3F`;
- `3F == n + 2(n - 3)`;
- each triangle has three distinct vertices and one closed oriented cycle;
- each source boundary use appears exactly once in source direction;
- each internal diagonal appears exactly twice in opposite directions; and
- no edge proposal is unclassified.

Treat a count identity only as a supplement to per-record verification.

### 11.2 Triangle-dual connectivity and disk topology

Build a facet-local dual graph whose nodes are triangle proposals and whose edges are internal diagonals. Reconstruct adjacency from diagonal groups. Require:

- one connected dual component for nonempty facets;
- exactly `F - 1` internal-diagonal adjacencies;
- no self-loop or parallel duplicate diagonal group;
- traversal from the least triangle key visits every triangle exactly once; and
- the boundary cycle reconstructed from unpaired edge uses equals the Component 02 source ring exactly, including direction and corner identity.

This establishes the expected triangulated-disk combinatorics. Do not infer connectivity from shared coordinates.

### 11.3 Bounded triangle orientation

Re-evaluate each triangle's projected orientation through a producer-verification formula path distinct from the cached ear-candidate object. Require:

- interval excludes zero;
- sign equals accepted source facet orientation;
- exact nominal sign, when available, does not contradict the definite interval;
- source plane/projection references agree; and
- no triangle exceeds source precision eligibility.

Store the worst lower area margin and uncertainty witness.

### 11.4 Pairwise embedding audit

Build a deterministic conservative 2D triangle-bound index and enumerate potentially interacting triangle pairs in canonical pair-key order. The index must have no false negatives; exhaustive tests compare all pairs.

Classify each pair independently of ear insertion history:

- triangles sharing an internal diagonal may intersect exactly on that full opposite-use edge and its endpoints, but may not overlap with positive area or cross elsewhere;
- triangles sharing only a source vertex may meet only at that source identity;
- triangles with no shared source identity must be definitely interior-disjoint and boundary-disjoint;
- sharing two vertices without the corresponding single internal-diagonal group is invalid;
- coordinate-coincident but identity-distinct vertices do not authorize contact; and
- any unresolved relation after the prescribed alternate formula is a typed coverage uncertainty failure.

Use bounded segment relations and oriented-side tests. Do not rely on centroid separation or area sums.

### 11.5 Triangle-inside-source checks

For every triangle choose deterministic interior sample barycentric fractions from a frozen rational sequence, beginning with `(1/3,1/3,1/3)`. Construct samples with Component 03 bounded affine operations. Reject a sample that overlaps a triangle boundary too broadly to classify; try the next prescribed fraction.

Classify each certified triangle-interior sample against the original source ring with a producer-side bounded winding/ray method that does not call ear eligibility helpers. Require definitely inside. A sample outside or unresolved after the prescribed finite sequence prevents publication.

Additionally verify each internal diagonal's certified interior sample lies definitely inside the source ring, excluding its endpoints. This guards against a noncrossing but exterior diagonal caused by a local-cone/provider defect.

### 11.6 Source coverage witnesses

Consume Component 02 validation-decomposition evidence through a verifier-safe read-only view. For every stored validation-decomposition interior witness:

- classify it against all candidate output triangles using a deterministic conservative index;
- require exactly one definite containing triangle interior, or an explicitly documented boundary-sharing classification when the witness lies on a legal internal diagonal;
- reject zero coverage, multiple positive-area coverage, or unresolved classification; and
- encode the mapping only as verification evidence, not source semantics.

If Component 02 does not retain sufficient witness samples under the selected artifact version, construct a deterministic independent witness set from its validation-only decomposition records before triangulation. Do not use Component 04 ear triangles to generate the only coverage samples.

### 11.7 Conservative area agreement

Compute:

- the source polygon bounded signed area using the inherited Component 02 formula/evidence;
- each triangle bounded signed area through an independently dispatched Component 03 formula;
- the deterministic pairwise-reduced triangle area sum in canonical triangle-key order; and
- the bounded difference between source area and triangle sum.

Require the difference enclosure to contain zero and satisfy the provider's conservative roundoff/lineage consistency rule. A definite nonzero mismatch fails. Area equality is secondary evidence only and cannot override boundary, overlap, connectivity, or witness failures.

### 11.8 Facet transaction boundary

A facet result becomes eligible for operand assembly only after Sections 11.1-11.7 pass. Keep it in private transaction-owned storage. If any facet fails, discard all facet results for the operand and choose the canonical minimum typed failure across completed private tasks. Never publish earlier facets as a partial complex.

## 12. Canonical ordering, semantic invariance, and digests

### 12.1 Canonical facet order

Process or merge facets in canonical `source_facet_id` order. Serial execution should follow this order. Parallel execution may compute private facet results in any schedule, but canonical merge and error arbitration use full facet/feature keys.

### 12.2 Canonical arrays

Publish:

- source vertex references ordered by `source_vertex_id`;
- facet triangulation records ordered by `source_facet_id`;
- triangles ordered by full source triangle key;
- internal diagonals ordered by full facet-local diagonal key;
- triangle edge uses ordered by incident triangle ID then local slot;
- provenance maps ordered by source identity/domain key; and
- coverage evidence ordered by source facet then stable witness/pair key.

No unordered-container iteration is serialized.

### 12.3 Three distinct digests

Maintain separate digests:

1. **source semantic digest** — predecessor semantic digest plus source facet/ring/edge/shell semantics; excludes exact triangulation;
2. **exact triangulation digest** — includes exact triangle and internal diagonal choices, predicate/formula versions, and coverage evidence; and
3. **replay/presentation digest** — includes original caller ordering/bit provenance and diagnostic replay details according to Component 01 policy.

The artifact canonical digest commits to all required versions and both semantic/exact digests. Downstream source-feature ownership, coincident contact policy, and alternative-triangulation equivalence must key through source IDs/semantic digest. Caches specifically tied to triangle layout may use the exact triangulation digest.

### 12.4 Alternative legal triangulations

Provide a test-only artifact builder that accepts an independently supplied legal triangle list for one validated source facet, runs the same provenance construction, producer coverage checks, canonical ID assignment, codec, and independent verifier, and marks the provider as a test adapter version.

The adapter must not bypass any invariant. It exists to prove that Components 05-15 use source-facet/source-feature lineage rather than internal diagonal choice. Alternative legal triangulations may have different exact triangulation digests and triangle IDs, but must have identical source semantic digests and equivalent downstream Boolean results.

## 13. Independent `SourceTriangleComplexVerifier`

### 13.1 Independence boundary

The verifier receives only immutable predecessor artifacts, the proposed source triangle complex, owner-checked Component 03 verification capabilities, and Component 01 resource/diagnostic services. It must not call:

- ear candidate evaluation;
- active-ring mutation;
- producer boundary/diagonal grouping helpers;
- producer dual traversal;
- producer cached coverage booleans;
- producer triangle/diagonal ID assignment helpers; or
- a method that merely returns stored counts/digests as truth.

It may share strong-ID definitions, canonical-byte primitives, immutable schemas, and low-level Component 03 predicate dispatch. Implement separate record traversal, grouping, adjacency reconstruction, point/ring classification, and codec traversal.

### 13.2 Verifier workflow

For the whole operand artifact, independently:

1. validate all versions, owner/operand IDs, predecessor digest links, reserved fields, and dense ID ranges;
2. reconstruct every facet ring from Component 02 and verify facet records reference it exactly;
3. reconstruct each triangle's directed edges from its vertex cycle;
4. reclassify each edge as source boundary or internal by matching against Component 02 boundary uses;
5. rebuild source-boundary use groups and internal-diagonal groups from scratch;
6. require all per-edge direction, count, facet, shell, and endpoint invariants;
7. rebuild triangle-dual connectivity and exact source boundary cycle;
8. recompute triangle counts, diagonal counts, Euler identities, and source vertex coverage;
9. independently re-evaluate bounded triangle orientations and source-plane consistency;
10. build an independent conservative triangle-pair index and rerun forbidden-overlap/intersection checks;
11. independently construct triangle/diagonal interior witnesses and classify them against the source ring;
12. independently map Component 02 validation-decomposition witnesses into the proposed triangles;
13. recompute source and triangle area enclosures with verifier formula dispatch/reduction;
14. rebuild canonical triangle/diagonal keys, dense ordering, maps, semantic digest, exact digest, and artifact bytes;
15. compare every stored digest and map against reconstruction;
16. verify resource/statistics records are conservative and no active reservation/private handle escaped; and
17. return one deterministic verified disposition or typed rejection.

### 13.3 Mutation rejection

The verifier must reject at least:

- deleting or duplicating a triangle;
- rotating a triangle inconsistently with its edge slots;
- reversing one triangle;
- replacing one vertex ID;
- shrinking or substituting orientation evidence;
- changing a triangle's source facet or shell;
- deleting, duplicating, reversing, or relabeling a source boundary use;
- labeling an internal diagonal as a source edge;
- labeling a source edge as internal;
- deleting one internal-diagonal partner;
- making both diagonal uses the same direction;
- pairing diagonal uses across facets;
- creating a disconnected triangle island;
- introducing a crossing or positive-area overlap while preserving area sum;
- moving a coverage witness mapping;
- changing triangle or diagonal canonical order/ID;
- replacing a predecessor digest or owner;
- altering a policy/formula/version field;
- changing a semantic or exact digest byte;
- adding unknown records or nonzero reserved bits; and
- leaking a private/temporary handle into the artifact.

Every mutation must produce a deterministic Component 04 verifier failure with the least canonical offending feature key.

## 14. Downstream contract for Component 05 and later stages

Publish narrow immutable query views sufficient for Component 05 to:

- enumerate source vertices, source triangles, facet groups, shell groups, and edge uses in canonical order;
- retrieve a triangle's three source vertices and accepted orientation evidence;
- retrieve each triangle edge's role without consulting coordinates;
- retrieve exact Component 02 source directed/undirected edge identities for boundary uses;
- retrieve facet-local diagonal identity and opposite use for internal edges;
- move across an internal diagonal while preserving authoritative source-facet identity;
- distinguish source semantic digest from exact triangulation digest;
- retrieve conservative triangle/source-facet bounds; and
- obtain complete source/caller provenance for diagnostics.

Components 05-10 must treat `source_facet_id` as authoritative relation lineage across internal diagonals. Candidate/event ownership must never be keyed solely by `source_triangle_id`. A relation discovered against one generated triangle must retain source facet, source edge, and source vertex lineage so another legal triangulation does not change ownership.

The Component 04 artifact does **not** pair source boundary halfedges across facets, build vertex fans, expose mutable adjacency, or guarantee a final halfedge manifold. Component 05 owns those tasks and defensively verifies Component 04 first.

## 15. Resources, cancellation, transactions, and Component 17 boundary

### 15.1 Resources

Use exact Component 01 leases for:

- projected facet points;
- active-ring nodes;
- candidate records and conservative indexes;
- predicate/evidence records;
- private triangle and edge-use proposals;
- sort/canonical key storage;
- producer pair/witness coverage checks;
- persistent source triangle/diagonal/provenance records;
- verifier indexes/workspaces;
- diagnostics/replay; and
- canonical bytes/digests.

Reconcile reserved versus actual counts before promotion. Persistent leases transfer only with the immutable artifact. All temporary leases release on facet completion, failure, cancellation, or rollback.

### 15.2 Cancellation

Poll cancellation at deterministic boundaries:

- facet range start;
- projected-point materialization chunks;
- initial candidate evaluation chunks;
- each ear-removal iteration before private mutation;
- candidate relation batches;
- producer coverage pair/witness batches;
- facet canonical sort/encode boundary;
- verifier facet/pair/witness batches; and
- immediately before artifact commit.

Do not poll in the middle of a Component 03 scalar primitive or halfway through relinking an active ring. Finish the local atomic mutation, then discard private state at the next checkpoint.

### 15.3 Transactions

Use one Component 04 operand-stage transaction containing private facet subtransactions or arenas. Facet success does not publish. Any error rolls back:

- all private active-ring and candidate state;
- all unassigned triangle/diagonal proposals;
- all persistent-ID reservations not committed;
- all canonical byte buffers;
- all temporary and proposed persistent resource leases; and
- all partial diagnostics except the canonical selected replay/error payload.

Publication is a single immutable artifact handle after independent verification and pre-commit cancellation polling.

### 15.4 Deterministic failure arbitration

When private facet tasks discover independent failures, encode complete failure keys including stage/checkpoint, operand, facet, ring/corner/edge/vertex identities, relation pair, subcode, and predicate evidence key. Select the lexicographically least complete key through Component 01. Thread completion order never selects the error.

### 15.5 Component 17 concurrency boundary

Implement and keep executable a serial semantic reference. A future parallel provider may:

- assign immutable canonical facet ranges to workers;
- materialize private projected workspaces;
- run private ear triangulation and producer verification;
- return private triangle/diagonal proposals and failure records; and
- avoid all shared mutable ID allocation and artifact vectors.

Component 17 must merge successful facet proposals in canonical facet/key order, assign final IDs, rerun operand-level cross-facet boundary consistency, invoke the independent verifier, and commit transactionally. Supported worker counts must produce byte-identical artifacts, digests, diagnostics, and selected failures.

## 16. Known-answer and projection tests

### 16.1 Hand-auditable facets

Commit fixtures and expected V1 triangle/diagonal keys for:

- a triangle;
- convex quadrilaterals and higher polygons;
- concave arrow/L shapes with multiple reflex vertices;
- polygons with several simultaneously valid ears;
- long thin polygons;
- boundedly collinear boundary chains;
- nearly collinear ears just inside and outside certainty;
- large translations with small local features;
- mixed-magnitude coordinates;
- signed-zero and subnormal coordinates;
- projection-axis ties;
- exact nominal coordinate coincidences with distinct source IDs where Component 02/03 evidence permits certification; and
- corresponding uncertifiable coincident/degenerate cases with exact expected typed failures.

For each successful fixture commit:

- canonical source ring and boundary uses;
- selected projection/formula IDs;
- exact ear sequence as diagnostic evidence, not semantic identity;
- canonical triangles and internal diagonals;
- boundary/diagonal provenance;
- orientation/coverage evidence summaries;
- semantic and exact digests; and
- golden canonical bytes.

### 16.2 Projection reuse tests

Verify Component 04:

- uses the exact Component 02 projection axis/formula/orientation mapping;
- rejects a mutated projection record;
- preserves source orientation under X/Y/Z coordinate-drop frames;
- handles exact tie rules identically across ring/facet permutations;
- never uses `atan2`, normalized directions, or raw area sign;
- preserves signed-zero source bits in provenance; and
- produces identical projected predicate traces across supported strict builds.

### 16.3 Triangular source facet fast path

A source triangle still passes the full contract. It may use a bounded fast path only if it:

- validates all predecessor records;
- re-evaluates accepted orientation through Component 03;
- labels all three source boundary uses;
- publishes zero internal diagonals;
- runs producer and independent coverage checks appropriate to one triangle;
- assigns canonical ID/order through the normal path; and
- produces the same bytes as the general algorithm.

Do not bypass verification merely because `n == 3`.

## 17. Boundary, coverage, and invariance tests

### 17.1 Shared source-edge tests

Construct adjacent source facets sharing an edge under:

- ordinary coordinates;
- reversed caller vertex/facet arrays with canonical remapping;
- different facet projection axes;
- long edges and small adjacent features;
- duplicate coordinate values elsewhere;
- signed-zero endpoint components;
- subnormal offsets; and
- high-valence source vertices.

Require each facet to preserve its exact directed source use, and require operand-level Component 04 verification to find exactly two opposite triangle boundary uses for the Component 02 source undirected edge. No coordinate snapping or cross-facet triangulation coordination is permitted or needed.

### 17.2 Independent coverage tests

For every fixture independently reconstruct:

- source boundary cycle;
- internal diagonal opposite-use groups;
- triangle dual connectivity;
- Euler/count identities;
- all triangle pair relations;
- triangle/diagonal interior-in-polygon classifications;
- validation-decomposition witness coverage; and
- conservative area agreement.

Tests must not call producer ear-selection, active-ring, or provenance grouping helpers. Add fixtures where total triangle area is preserved despite a duplicated/overlapping triangle and a missing region; the verifier must reject them through topology/pair/witness checks.

### 17.3 Ring and source-array metamorphisms

Apply:

- caller ring rotation;
- caller facet permutation;
- caller vertex-array permutation with index remapping;
- disconnected shell permutation;
- exactly representable translation;
- exact power-of-two scale within the same qualified predicate disposition;
- axis permutation with remapped orientation convention;
- whole-shell orientation reversal followed by valid Component 02 policy correction in test fixtures; and
- serial/private-task scheduling permutations.

Under the frozen unique V1 policy, canonical source triangle complex bytes and digests must be identical whenever the transformed input has the same semantic canonical source artifact. Presentation/replay maps may differ only in the documented caller-location fields.

### 17.4 Alternative-triangulation tests

For polygons with multiple legal triangulations:

- enumerate alternatives with a test-only exact rational oracle for small `n`;
- feed each through the test adapter and full verifier;
- require identical source semantic digests and different exact triangulation digests when diagonals differ;
- pass each artifact to Component 05 test adapters and later available components;
- verify source-facet relation lineage and final Boolean topology are unchanged;
- include cross-operand intersections that cross different internal diagonals; and
- prove internal diagonals never become winding/connectivity barriers or source symbolic owners.

Until later components exist, provide compile-time/mock capability tests that assert they request source-facet/source-feature IDs rather than only triangle IDs.

## 18. Mutation, fuzzing, and exact-oracle tests

### 18.1 Mutation suite

Implement every mutation in Section 13.3. Additionally mutate:

- the least-ear policy version while retaining the same triangles;
- one triangle local edge slot without changing its vertex cycle;
- one caller reverse-map reference;
- one predicate formula ID;
- one conservative triangle bound so it excludes a source point enclosure;
- one area-reduction order descriptor;
- one `source_feature_eligible`/`classification_barrier` semantic bit;
- one resource statistic below reconstructed use; and
- one canonical byte length/count field.

Require deterministic independent rejection for every mutation.

### 18.2 Exact rational test oracle

Use the in-tree test-only arbitrary-precision integer/rational facilities planned by Component 03/16, or a Component 04-local temporary copy until the shared test target exists. Production must not link it.

For bounded small integer/rational simple polygons, the oracle must:

- compute exact orientation and segment relations;
- enumerate all legal no-Steiner triangulations for small `n`;
- determine the exact V1 least-ear sequence;
- verify exact triangle interiors are disjoint;
- verify exact union boundary equals the source ring;
- verify exact area equality; and
- classify producer/verifier witnesses exactly.

Compare Component 03 bounded outcomes with exact truth and require no definitely wrong acceptance.

### 18.3 Deterministic fuzzing and shrinking

Generate valid simple one-ring polygons from exact rational templates, then map to supported `T` and apply controlled ULP perturbations. Stratify:

- ring size;
- convex/reflex patterns;
- collinear run length;
- narrow channels and ear blockers;
- coordinate exponents and translations;
- signed zero/subnormal values;
- projection axis/tie conditions;
- duplicate nominal coordinate identities with separately tracked topology; and
- precision enclosure sizes around predicate thresholds.

Every generated case first passes the Component 02 test provider or records an expected predecessor rejection. For Component 04 cases, compare the serial provider, independent verifier, and exact oracle where bounded.

Serialize exact source bits, IDs, policies, limits, predicate evidence, and failure key. Shrink while preserving the failure category using deterministic operations: remove a nonessential ring vertex, reduce integer/rational magnitude, reduce ULP perturbation, shorten collinear runs, and simplify reflex structure. Store every fixed bug as a permanent golden regression.

## 19. Performance and resource tests

### 19.1 Structural counters

Expose test-only counters for:

- projected points materialized;
- candidate evaluations;
- orientation/local-cone predicates;
- diagonal/segment relations;
- point-in-ear relations;
- candidate invalidations/rescans;
- emitted triangles/diagonals;
- coverage pair candidates/relations;
- verifier pair candidates/relations;
- canonical sort records; and
- bytes/work/resources reserved and committed.

Counters are diagnostic only and do not affect semantic bytes unless the frozen replay policy explicitly includes them.

### 19.2 Expected behavior

Require:

- exact linear output storage in `n`;
- no accidental quadratic persistent memory;
- near-linear or `O(n log n)` candidate work for ordinary convex and mildly concave fixtures after the indexed provider is enabled;
- deterministic bounded work for adversarial ear-blocking and long-collinear patterns;
- explicit resource/work failure before unbounded rescanning;
- no hidden recursion proportional to untrusted ring size unless stack depth is preflighted and bounded; and
- byte-identical output between the serial reference and optimized provider.

The serial full-rescan reference may be quadratic and remain enabled for equivalence tests on bounded sizes. It must not be selected as the unrestricted production provider for large rings without a documented resource policy.

### 19.3 Limit boundaries

For every resource and major work class, test limit-minus-one, exact limit, and limit-plus-one. Cover:

- facet count;
- maximum ring size;
- total projected point count;
- triangle/diagonal ID capacity;
- candidate records;
- predicate evidence;
- pair-audit candidates;
- verifier work;
- canonical bytes; and
- diagnostic/replay bytes.

Require deterministic failure keys, no leaked leases, no partial artifact, and successful replay of the failure.

### 19.4 Cancellation and fault injection

Inject cancellation before, during, and after every checkpoint in Section 3.2. Inject allocation/codec/provider failures at transaction boundaries. Require:

- all worker/private state joined and destroyed;
- all temporary and proposed persistent resources released;
- no IDs or artifact handles published;
- no active reservations remaining;
- canonical failure selection independent of schedule; and
- predecessor artifacts unchanged.

## 20. Implementation sequence and handoff gates

Implement in this order. Do not begin a later gate while an earlier gate lacks its tests:

1. Add Component 04 versions, policy enums, checkpoints, subcodes, and compile-time uniqueness tests.
2. Add immutable artifact/record schemas with owner/version/range validation but no producer.
3. Add exact count/capacity preflight and empty-operand artifact path.
4. Add the narrow Component 02/03 capability views and projection-record validation.
5. Factor/audit `BoundedSourcePolygonKernel` without changing Component 02 semantics; run all Component 02/03 regressions.
6. Add projected facet workspace and exact source-boundary lookup.
7. Add the simple serial active-ring implementation and bounded ear candidate evaluator.
8. Add deterministic least-key selection and ear removal for triangles/convex polygons.
9. Add concave, reflex, collinear, thin, and threshold fixtures with typed uncertainty failures.
10. Add private triangle/edge-use proposal reconstruction from final triangles.
11. Add source-boundary and internal-diagonal grouping/provenance checks.
12. Add producer combinatorial disk/boundary/dual verification.
13. Add producer independent orientation, pairwise embedding, interior, witness, and area checks.
14. Add canonical triangle/diagonal ID assignment, facet records, semantic/exact digests, and operand assembly.
15. Add canonical codec, decode validation, golden bytes, and replay.
16. Implement the independent verifier with separate traversal/grouping/index/classification code.
17. Add the complete mutation suite and prove zero surviving required mutations.
18. Add the exact-rational legal-triangulation oracle and deterministic fuzz/shrinking corpus.
19. Add shared-source-edge and source-array/ring metamorphic tests.
20. Add the alternative-triangulation test adapter and source-semantic invariance tests.
21. Add conservative candidate indexes and affected-candidate invalidation; prove exact equivalence to the serial reference.
22. Add Component 17 private facet-task/canonical merge hooks while keeping the serial reference executable.
23. Integrate the read-only Component 04 query contract into Component 05 test providers without implementing Component 05.
24. Run Component 01, 02, and 03 full regression suites under every supported strict build.
25. Run debug/release, sanitizer, deterministic worker-count, resource, cancellation, and replay qualification matrices supported by the repository.
26. Update `tracker.md` only after every Section 21 item is demonstrated by the completed plan and its implementation handoff criteria.

Each gate must leave the branch buildable and must not weaken a predecessor contract, change legacy public triangulator behavior, or introduce an external dependency to make a test pass.

## 21. Definition of done

Component 04 is complete only when all of the following are true:

- only the Component 04 scope is implemented and no legacy Boolean source is called;
- V1 is frozen as one-ring, no-Steiner, no source-edge subdivision, no source vertex removal/merge, and no degenerate published triangle;
- Component 02 source plane, projection, orientation, ring, edge, shell, and caller maps are validated and reused rather than recomputed inconsistently;
- every topology-affecting geometric decision uses Component 03 bounded predicates under the strict qualified C++17 arithmetic model;
- every accepted facet with `n` ring positions publishes exactly `n - 2` definitely oriented triangles and `n - 3` internal diagonals or returns a typed deterministic failure;
- every source ring vertex identity appears in the triangle complex and coordinate equality never merges identities;
- every source directed boundary edge use appears exactly once in the same direction with complete Component 02 provenance;
- every internal diagonal has exactly two opposite uses within one source facet and is identity-marked non-source, non-owner, and non-barrier;
- triangle and diagonal IDs/arrays are canonical, full-key collision-safe, and independent of ring rotation, source-array order, allocation, traversal, task, and worker schedule;
- facet semantic digests are independent of legal internal triangulation while exact triangulation digests commit to the selected V1 triangulation;
- producer verification reconstructs disk combinatorics, boundary equality, diagonal pairing, dual connectivity, triangle orientation, pairwise embedding, source interior, independent coverage witnesses, and conservative area agreement;
- independent verification repeats those obligations without calling producer ear/grouping/traversal helpers or trusting producer counts/booleans/digests;
- every required mutation is rejected deterministically with the least canonical offending feature;
- known-answer, exact-oracle, metamorphic, alternative-triangulation, shared-boundary, fuzz/shrink, adversarial, and performance suites satisfy the specification;
- legal alternative triangulations preserve source semantic identity and cannot change Component 05/later source-feature semantics or Boolean topology;
- ordinary convex/mildly concave facets meet the documented structural performance target, adversarial facets terminate within work/resource limits, and persistent memory remains linear;
- resource, cancellation, overflow, wrong-owner, stale-handle, unsupported-version, fault-injection, codec, and transaction tests prove zero partial publication and zero leaked reservations;
- the serial semantic reference remains executable and optimized/parallel providers are byte-identical under supported configurations;
- Component 01-03 regression suites remain passing under strict targets;
- production and normative-test code are portable C++17, standard-library-only, and use no external dependency; and
- `tracker.md` marks component 4 complete only after this fully complete implementation plan has been added and reviewed.
