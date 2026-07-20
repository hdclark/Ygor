# Plan 05: Canonical Halfedge Topology

## 0. Scope and non-negotiable constraints

Implement **only Component 05** from `component_05_canonical_halfedge_topology.md`. This component accepts the immutable, independently verified Component 02 and Component 04 artifacts and publishes one immutable `canonical_halfedge_operand<T,I>` per operand. The pair is wrapped as `canonical_source_manifolds<T,I>` for Component 06 and later stages.

The implementation must convert accepted source triangles into exact indexed topology. It must make triangle cycles, reciprocal edge pairing, vertex links, source-facet groups, shell groups, complete provenance, conservative geometry attachments, canonical identities, immutable queries, verification evidence, and deterministic bytes explicit. It must never infer topology from coordinates or repair predecessor topology.

The component must not:

- reread caller-owned `fv_surface_mesh<T,I>` arrays as authoritative data;
- call or adapt `src/YgorMeshesBoolean{,2,3,4,5}*.{h,cc}`;
- use mutable mesh repair, orientation repair, hole filling, remeshing, simplification, duplicate merging, or generic triangulation APIs;
- pair edges by coordinate equality, unordered coordinate pairs, distance, tolerance, spatial cells, hashes alone, or Morton codes;
- merge or alias distinct source vertex identities, including bit-identical coordinates and uncertainty envelopes;
- reverse, remove, replace, or retriangulate a Component 04 triangle;
- treat a facet-local diagonal as an original source feature, symbolic-contact owner, or classification barrier;
- merge distinct coplanar source facets by approximate plane or normal comparison;
- recompute source planes, source orientation, shell semantics, or authoritative triangle orientation through a different expression;
- use user tolerance as a topology epsilon;
- assign semantic IDs from pointer values, allocation order, unordered iteration, task completion order, worker number, or traversal accidents;
- publish mutable records, schedule-dependent lazy caches, partial pairing, incomplete maps, private workspace handles, or producer summary flags without reconstructible evidence;
- throw exceptions for expected contract, geometry, resource, cancellation, codec, or verification failure;
- use `long double`, fast-math, reassociation, implementation-defined serialization, unchecked object padding, or transcendental functions in authoritative decisions;
- introduce any external, vendored, downloaded, optional, or runtime-invoked dependency; or
- compile production or normative-test code outside the strict portable C++17 target established by Component 01.

Use Component 01 for owner tokens, strong IDs, checked arithmetic, typed outcomes, stage/checkpoint IDs, resource leases, cancellation, deterministic failure arbitration, diagnostics, replay, canonical encoding, SHA-256, transactions, and immutable publication. Use Component 02 as the sole authority for source vertices, source directed and undirected edges, facets, rings, shells, occupied-side semantics, and caller mappings. Use Component 03 for conservative bounds and any non-authoritative bounded geometric attachment. Use Component 04 as the sole authority for source triangles, oriented triangle cycles, edge-use roles, facet-local diagonal identities, source-facet semantic lineage, and exact triangulation lineage.

V1 is a deterministic sort-and-scan provider with an executable serial semantic reference. It uses contiguous immutable arrays, fixed triangle-local halfedge slots, provenance-keyed edge grouping, and exact topological fan traversal. Target complexity is `O(V + F + H log H)` time and `O(V + F + H + E)` persistent memory, where `H = 3F` and `H = 2E`. Component 17 may parallelize private proposal generation and verification, but canonical merge must reproduce the serial artifact and failure exactly.

No failed, cancelled, partially paired, partially encoded, or verifier-rejected operand may publish. Mark Component 05 complete in `tracker.md` only after every requirement in Section 20 is represented by an implementable instruction and qualification gate.

## 1. Existing Ygor assessment and mandatory reuse decisions

### 1.1 `fv_surface_mesh<T,I>` remains only a public carrier

`fv_surface_mesh<T,I>` is a mutable lowest-common-denominator container with vertex arrays, polygon index arrays, optional attributes, metadata, and an optional `involved_faces` convenience index. It does not provide immutable halfedges, reciprocal pairs, canonical source-feature IDs, edge semantic classes, source-facet/shell groups, cyclic vertex-link proofs, bounded geometry references, owner/version validation, transactional publication, or independent artifact verification.

`involved_faces` may be absent or stale and is not a cyclic fan. Do not wrap or trust it as Component 05 topology. Reuse `fv_surface_mesh`, `vec2`, and `vec3` only through already frozen predecessor boundaries or as nominal carriers. Do not modify the public mesh class into the Boolean engine's internal representation.

### 1.2 Existing verification helpers are references, not providers

`YgorMeshesVerification.h/.cc` contains useful small ideas such as ordered endpoint pairs, edge-use counts, and opposite-direction checks. It is insufficient because it reads mutable mesh arrays, identifies edges only by endpoint indices, does not preserve directed-use provenance or edge roles, does not construct twins or vertex links, reports booleans/exceptions, and has no owner, version, resource, transaction, digest, or independent-verifier contract.

Do not call `ClassifyEdges`, `HasConsistentOrientation`, `IsClosedManifold`, or `ValidateClosedTriangularMesh` from the Component 05 producer or verifier. Keep their public behavior unchanged. Similar map/count patterns may be reproduced only with Component 01 strong IDs and complete provenance keys.

### 1.3 Orientation and hole utilities are unsuitable

`YgorMeshesOrient.cc` and `YgorMeshesHoles.cc` create epsilon-based vertex representatives, can collapse nearby endpoints, derive adjacency from those representatives, reverse faces, or fill boundaries. These behaviors violate identity-only topology and the no-repair contract. Do not reuse or refactor their representative maps, edge maps, orientation propagation, boundary traversal, or repair logic into Component 05.

### 1.4 Refinement/remeshing adjacency is not a substrate

Refinement and remeshing routines build temporary endpoint maps and neighborhood sets for mutable operations. They rely on caller indices, local traversal, raw geometry, and operation-specific assumptions. Do not generalize those structures into Component 05. A purpose-built immutable schema is required.

### 1.5 Mandatory reuse

Consume and validate, rather than duplicate:

- Component 02 canonical source vertices, directed-edge uses, undirected edges, facets, rings, shells, occupied-side evidence, canonical keys, and reversible caller maps;
- Component 03 bounded points, precision lineage, bounded support/orientation references, conservative-bound constructors, and containment services; and
- Component 04 source triangles, local edge-use records, source-boundary/internal-diagonal labels, facet-local diagonal records, facet groups, semantic/exact triangulation digests, and triangle/facet bounds.

Do not create a second source-edge registry, shell classifier, plane model, orientation expression, or triangulation provenance model.

### 1.6 Permitted implementation machinery

Use standard-library C++17 deterministic facilities: fixed-width integers, `std::array`, `std::vector`, `std::optional`, `std::variant`, `std::sort`, `std::lower_bound`, deterministic merge, and private ordered maps where justified. Prefer sorted vectors and linear scans for grouping and verification. `std::unordered_*` may be used only for non-authoritative private lookup with full-key collision checks and deterministic sorting before any observable result.

## 2. Exact file and target layout

Add under `src/YgorMeshesBooleanBounded/`:

- `CanonicalHalfedgeTypes.h` — Component 05 policy enums, strong-ID aliases, closed record tags, stable version constants, temporary proposal types, and query result types;
- `CanonicalHalfedgeOperand.h` — immutable artifact records and checked read-only views;
- `CanonicalSourceManifolds.h` — immutable two-operand wrapper and cross-artifact validation;
- `CanonicalHalfedgeBuild.h/.cc` — typed entrypoint and fixed phase orchestration;
- `CanonicalHalfedgePreflight.h/.cc` — exact counts, representability, byte/work/resource preflight;
- `CanonicalHalfedgePairing.h/.cc` — provenance-key construction, deterministic edge grouping, reciprocal pairing, and canonical representatives;
- `CanonicalVertexFans.h/.cc` — incidence grouping, exact fan transition, canonical traversal, and producer fan evidence;
- `CanonicalFeatureGroups.h/.cc` — source-facet and shell group assembly plus total forward/reverse maps;
- `CanonicalGeometryAttachments.h/.cc` — conservative feature bounds and explicitly non-authoritative nominal attachments;
- `CanonicalHalfedgeCodec.h/.cc` — canonical encoding/decoding and digest calculation;
- `CanonicalHalfedgeVerifier.h/.cc` — independent reconstruction and mutation rejection; and
- `CanonicalHalfedgeQueries.h` — narrow immutable downstream query views with documented complexity.

Extend existing bounded-subsystem registries instead of creating parallel infrastructure:

- `ContractVersions.h` for all Component 05 schema/provider/codec/verifier/formula versions;
- Component 01 strong-ID, stage, checkpoint, error-subcode, resource-kind, diagnostic, and replay registries;
- Component 03 conservative-bound formula registry only where a genuinely new aggregate formula is required; and
- the strict bounded Boolean CMake target and floating-point qualification helper.

Add tests under `tests/mesh_boolean_bounded/`:

- `TestCanonicalHalfedgeUnit.cc`;
- `TestCanonicalHalfedgePairing.cc`;
- `TestCanonicalHalfedgeVertexFans.cc`;
- `TestCanonicalHalfedgeGroups.cc`;
- `TestCanonicalHalfedgeGeometry.cc`;
- `TestCanonicalHalfedgeCanonicalization.cc`;
- `TestCanonicalHalfedgeAlternativeTriangulation.cc`;
- `TestCanonicalHalfedgeMutation.cc`;
- `TestCanonicalHalfedgeProperties.cc`;
- `TestCanonicalHalfedgeAdversarial.cc`;
- `CanonicalHalfedgeFixtures.h/.cc`;
- `CanonicalHalfedgeExactOracle.h/.cc`; and
- `GoldenCanonicalHalfedgeV1.h`.

Register separate CTest cases for unit/cycle, pairing/provenance, vertex-link, groups/maps, geometry bounds, canonicalization/codec, alternative triangulation, mutation, properties/fuzz, and adversarial/resource/concurrency suites. Apply the strict target to every production and normative-test translation unit. No network discovery or optional test dependency is permitted.

Keep proposal records, sort workspaces, mutable pairing tables, private fan marks, diagnostic builders, and verifier scratch indexes out of installed/public headers. Templates must be header-defined or explicitly instantiated for exactly supported `float`/`double` and `uint32_t`/`uint64_t` combinations.

## 3. Stable versions, stages, checkpoints, and failures

### 3.1 Version registry

Add explicit nonzero V1 constants for:

- Component 05 provider and policy;
- internal vertex, triangle, halfedge, edge, fan, facet-group, shell-group, provenance-map, geometry-attachment, verification-evidence, operand-artifact, two-operand-wrapper, codec, and verifier schemas;
- triangle local-rotation policy;
- edge pairing-key policy;
- canonical directed-representative policy;
- vertex-fan transition/orientation policy;
- canonical ordering policy;
- topology, geometry, and artifact digest layouts; and
- any Component 03 aggregate-bound formula introduced for this stage.

Zero is invalid. Unknown required versions, unsupported enum values, nonzero reserved bits, and mismatched predecessor versions/formulas are typed failures. Encode all required versions into artifact headers, canonical bytes, diagnostics, and replay.

### 3.2 Fixed checkpoints

Use the Component 05 stage and stable checkpoints in this order:

1. context capability and strict-environment validation;
2. operand owner/version/digest validation;
3. predecessor count and representability preflight;
4. work and persistent-resource reservation;
5. internal vertex proposal construction;
6. triangle canonical-rotation proposal construction;
7. fixed triangle-halfedge cycle construction;
8. edge pairing-key generation;
9. deterministic edge proposal sort/grouping;
10. source-edge pair validation;
11. internal-diagonal pair validation;
12. canonical edge ordering, ID assignment, and twin closure;
13. vertex-incidence grouping;
14. canonical vertex-fan traversal;
15. source-facet group assembly;
16. shell group and total provenance-map assembly;
17. conservative geometry attachment construction;
18. producer structural verification;
19. canonical encoding and digest construction;
20. independent verification;
21. replay/resource reconciliation; and
22. final cancellation check and transaction commit.

Do not renumber released checkpoints. Add future provider-specific checkpoints only under a new version or reserved gaps.

### 3.3 Required failure subcodes

Allocate a disjoint Component 05 range with explicit values for at least:

- unsupported policy/version/formula;
- wrong, stale, cross-context, cross-operand, or cross-domain handle;
- predecessor artifact/digest/disposition mismatch;
- entity/count/byte/index/work overflow;
- persistent or temporary resource exhaustion;
- malformed source vertex, triangle, local edge-use, facet, shell, or provenance reference;
- repeated triangle vertex identity;
- triangle orientation or canonical-rotation mismatch;
- triangle cycle missing, non-closing, repeated, or wrong length;
- unknown/contradictory edge role;
- edge pairing key malformed;
- source-edge use missing, duplicated, same-direction, wrong-facet, wrong-shell, or wrong reciprocal source use;
- internal-diagonal use missing, duplicated, same-direction, cross-facet, or incorrectly source-feature eligible;
- edge run cardinality not two;
- reciprocal pair one-way, self-paired, endpoint-mismatched, or cross-edge;
- canonical directed representative mismatch;
- vertex incidence missing, duplicated, wrong-origin, or out of range;
- vertex fan open, repeated, disconnected, incorrectly oriented, or crossing a distinct vertex identity;
- facet/shell membership missing, duplicated, or inconsistent;
- forward/reverse provenance map non-total or non-bijective where required;
- geometry attachment missing, malformed, non-finite, or non-conservative;
- canonical key duplicate, ordering mismatch, or full-key collision mishandled;
- codec length/count/tag/reserved-field error;
- verifier rejection;
- resource reconciliation or abstract-work failure;
- cancellation; and
- internal construction invariant failure.

Every failure records stage/checkpoint, owner, operand, least canonical offending predecessor/internal feature keys, expected and observed counts, relevant versions/digests, resource limits, bounded evidence where relevant, and deterministic replay payload. Parallel failures use Component 01 canonical failure arbitration.

## 4. Public entrypoint and capability boundaries

### 4.1 Typed entrypoint

Provide an internal entrypoint equivalent to:

```cpp
template <class T, class I>
stage_outcome<canonical_source_manifolds<T,I>>
build_canonical_source_manifolds(
    const boolean_context<T,I>& context,
    const validated_operands<T,I>& validated,
    const source_triangle_complexes<T,I>& triangles,
    const precision_context<T>& precision);
```

The exact wrapper naming may follow Component 01 conventions. The observable contract must validate both operand inputs before publication, build each operand in a private subtransaction, support either/both operands empty, join all work before rollback, select the same primary failure under every schedule, publish neither operand if either fails, and publish one immutable wrapper only after both independent verifiers pass.

A lower-level test function may construct one operand. Ordinary pipeline publication is all-or-nothing for the two-operand wrapper.

### 4.2 Narrow predecessor views

Define read-only owner-checked views instead of exposing mutable implementation classes. Component 05 requires:

From Component 02:

- canonical source vertex enumeration and bounded-point lineage;
- source directed-edge and reciprocal-use lookup;
- source undirected-edge lookup with exactly two uses;
- facet/ring boundary and caller provenance;
- shell membership, nesting, orientation, and occupied-side semantics;
- canonical feature keys/order; and
- artifact versions and digests.

From Component 04:

- source triangle enumeration and oriented vertex cycle;
- each triangle local edge use and closed role-specific provenance;
- facet-local diagonal records and opposite uses;
- source facet/shell membership and member lists;
- bounded orientation/support references;
- conservative triangle/facet bounds;
- source semantic and exact triangulation digests; and
- verified disposition.

From Component 03:

- bounded point/support lookup;
- conservative segment, triangle, and union bounds;
- containment tests;
- exponent-safe optional nominal affine calculations;
- formula/version validation; and
- precision-lineage comparison.

Every capability validates owner/domain/version. Do not index predecessor private arrays without checked views.

### 4.3 Published immutability

Published accessors accept current-artifact strong IDs, validate owner/domain/range in checked builds, never allocate or mutate hidden state, return stable values/references/spans, behave deterministically under concurrent reads, document complexity, distinguish absent optional data from malformed required data, and never expose implementation pointers as identities. Prefer eager immutable tables; no scheduling-dependent lazy cache is allowed.

## 5. Exact entity and artifact schemas

### 5.1 Strong ID domains

Add distinct domains for:

- `manifold_vertex_id`;
- `manifold_triangle_id`;
- `manifold_halfedge_id`;
- `manifold_edge_id`;
- `vertex_fan_id`;
- `source_facet_group_id`;
- `source_shell_group_id`;
- geometry attachment IDs; and
- Component 05 verifier-evidence IDs where generic evidence IDs are insufficient.

Do not alias these domains to `I`, `size_t`, or predecessor IDs. Dense ordinals are checked implementation details. V1 normally maps one Component 02 source vertex to one manifold vertex, but include an occurrence discriminator in the canonical key; V1 requires zero and rejects other values.

### 5.2 Artifact header

Each `canonical_halfedge_operand<T,I>` header stores:

- context owner/token, operand role, and operand ID;
- all Component 05 provider/policy/schema/codec/verifier versions;
- required Component 01-04 versions/formulas;
- predecessor artifact IDs/digests;
- forwarded source semantic and exact triangulation digests;
- exact entity counts and checked byte lengths;
- canonical table ranges;
- precision-ledger references;
- independent verification disposition/version;
- resource/statistics and replay references;
- topology digest, geometry-attachment digest, and complete artifact digest; and
- zeroed reserved fields.

The artifact owns or immutably references every table. No mutable caller/workspace reference may escape.

### 5.3 Internal vertex record

Store:

- strong ID and complete canonical key;
- owner/operand;
- exact source vertex ID;
- V1 occurrence discriminator zero;
- source shell and shell-group ID;
- nominal coordinate bits or immutable predecessor reference;
- Component 03 bounded-point/precision reference;
- caller provenance;
- canonical incident outgoing halfedge representative;
- fan ID and fan offset/count;
- source/internal map references;
- optional non-authoritative local scale; and
- zero reserved fields.

Canonical key:

```text
(owner, operand_role, source_vertex_id,
 occurrence_discriminator=0, vertex_schema_version)
```

Coordinates, caller ordinals, array positions, and bounds are excluded from identity. A nonempty accepted closed V1 operand may not publish a zero-incidence vertex.

### 5.4 Triangle record and local rotation

Store:

- strong ID and canonical key;
- exact source triangle ID;
- source facet/ring/shell/operand/owner;
- three manifold vertices in canonical orientation-preserving rotation;
- the corresponding source vertices;
- base halfedge and exactly three local slots;
- inherited orientation/support references;
- triangle-bound attachment;
- exact triangulation/caller provenance;
- facet-group and shell-group IDs; and
- zero reserved fields.

For oriented Component 04 cycle `(v0,v1,v2)` with local edge uses `(e0,e1,e2)`, compare the three orientation-preserving full keys:

```text
R0 = (vertex_key(v0), edge_use_key(e0),
      vertex_key(v1), edge_use_key(e1),
      vertex_key(v2), edge_use_key(e2))
R1 = (vertex_key(v1), edge_use_key(e1),
      vertex_key(v2), edge_use_key(e2),
      vertex_key(v0), edge_use_key(e0))
R2 = (vertex_key(v2), edge_use_key(e2),
      vertex_key(v0), edge_use_key(e0),
      vertex_key(v1), edge_use_key(e1))
```

Choose the least full key. Never consider reversed rotations. Reject repeated vertex identities or duplicate complete triangle keys.

### 5.5 Halfedge record and fixed layout

For each canonical triangle ordinal `t`, allocate three contiguous halfedges:

```text
h(t,0) = 3*t + 0
h(t,1) = 3*t + 1
h(t,2) = 3*t + 2
next(h(t,s)) = h(t,(s+1)%3)
previous(h(t,s)) = h(t,(s+2)%3)
```

Use Component 01 checked arithmetic before conversion to the storage ordinal. This layout is an implementation contract of V1 and must be versioned.

Each halfedge stores or supports constant-time derivation of:

- strong ID, triangle ID, local slot;
- origin and destination manifold/source vertex IDs;
- next and previous;
- reciprocal pair;
- undirected edge ID;
- closed edge semantic role;
- exact role-specific predecessor edge-use identity;
- source facet/shell/group IDs;
- bounded segment attachment; and
- complete provenance.

Destination is `origin(next(h))`; if redundantly stored, verify equality. Each cycle must close after exactly three steps and contain three distinct halfedges and vertices.

### 5.6 Closed edge semantic classes

Use a closed tagged representation:

```cpp
enum class canonical_edge_class : uint8_t {
    source_edge = 1,
    facet_internal_diagonal = 2
};
```

Unknown tags fail. For `source_edge`, store the exact Component 02 source undirected edge and the two source directed uses/facets. For `facet_internal_diagonal`, store exact source facet and Component 04 diagonal identity, and encode schema-fixed semantics:

- `source_feature_eligible = false`;
- `symbolic_contact_owner = false`;
- `classification_barrier = false`.

Do not store contradictory nullable fields. Prefer tagged role-specific payloads.

### 5.7 Pairing keys

Generate pairing keys only from authoritative identity:

Source edge:

```text
(owner, operand_role, source_edge_tag,
 source_undirected_edge_id, edge_pair_policy_version)
```

Internal diagonal:

```text
(owner, operand_role, internal_diagonal_tag,
 source_facet_id, source_facet_diagonal_id,
 edge_pair_policy_version)
```

Each halfedge proposal also carries its oriented source/manifold endpoints, incident triangle, local slot, exact predecessor edge-use, facet, and shell. Sort full proposals by pairing key, then a complete directed-use tie key. Every run must contain exactly two uses. Hashes may accelerate only after full-key comparison.

### 5.8 Edge record and canonical representative

Each edge stores:

- strong ID and complete key;
- class and role-specific predecessor identity;
- canonical endpoint pair ordered by full manifold vertex key;
- exactly two reciprocal halfedges and incident triangles;
- source facet/shell provenance for both uses;
- conservative segment bound;
- one canonical directed representative; and
- verification evidence.

Require endpoints to reverse exactly by identity. Choose the representative as the halfedge directed from the lower canonical vertex key to the higher key. Self-endpoints are invalid. This choice is independent of face orientation, memory order, and coordinates. Preserve both face-oriented halfedges separately.

### 5.9 Vertex incidence and fan storage

For an outgoing halfedge `h` at vertex `v`, define the oriented fan transition:

```text
fan_next(h) = pair(previous(h))
fan_previous(h) = next(pair(h))
```

Verify every intermediate halfedge exists and both transitions remain outgoing from `v`. These functions must be mutual inverses on the fan.

Group outgoing incidences by exact manifold vertex ID. Choose the least full halfedge key as canonical representative. Traverse `fan_next` until returning to the start. Require:

- no open transition;
- every visited halfedge has origin `v`;
- no repeated incidence before closure;
- closure after exactly the incidence count;
- all grouped outgoing incidences visited once;
- `fan_previous(fan_next(h)) == h` and converse;
- no second disjoint cycle; and
- no coordinate-coincident distinct vertex enters the fan.

Publish one canonical fan record per vertex with offset/count and the canonical sequence. This is exact topology, not geometric angle sorting.

### 5.10 Source-facet group record

For every Component 02 source facet publish:

- facet/ring/shell/operand/owner;
- canonical boundary ring and directed source-edge uses;
- canonical member triangle IDs;
- canonical member internal-diagonal IDs;
- inherited support plane, projection frame, orientation, and bounded evidence;
- caller provenance;
- source semantic digest and exact triangulation digest references;
- group bounds;
- canonical key/digest; and
- verifier evidence.

Member lists are sorted by complete key. Validate exactly against Component 04. Distinct coplanar facets remain distinct. Downstream traversal across an uncut internal diagonal can identify that both triangles remain in one source-facet semantic region.

### 5.11 Shell group record

For every Component 02 shell publish:

- shell/operand/owner;
- canonical member vertices, source facets, manifold triangles, and edges or immutable ranges/lists sufficient to reconstruct them;
- orientation, nesting, parity, and occupied-side provenance inherited from Component 02;
- conservative shell bound;
- canonical key/digest; and
- verifier evidence.

Component 05 must not recompute shell nesting or occupied side. It verifies membership consistency and forwards the accepted evidence.

### 5.12 Provenance and maps

Publish canonical immutable maps for:

- source vertex to exactly one V1 manifold vertex and inverse;
- source triangle to exactly one manifold triangle and inverse;
- source directed edge use to exactly one boundary halfedge;
- source undirected edge to one source-edge record and its two halfedges;
- facet-local diagonal to one internal edge and its two halfedges;
- triangle/local slot to halfedge and inverse;
- source facet to facet group/member triangles/member diagonals;
- source shell to shell group and member records;
- caller vertex/facet/ring positions to source/internal provenance where predecessor maps provide it; and
- canonical order permutations if physical arrays are not already canonical.

Maps are total over documented domains, sorted by full key, range/owner checked, and independently reconstructed. Counts or digests alone are not maps.

### 5.13 Geometry attachments

Required attachments:

- inherited bounded point per vertex;
- closed conservative segment bound per edge;
- closed conservative triangle bound per triangle;
- conservative source-facet group bound; and
- conservative shell bound.

All are constructed or validated through Component 03 and include inherited uncertainty, bound-construction rounding, formula/version IDs, and precision lineage. They must be finite and contain every permitted realization. Never shrink to nominal coordinates.

Optional non-authoritative attachments may include scaled area vectors, dominant axes, centroids, spatial keys, and local scales. Compute only with qualified exponent-safe Component 03 services. Mark availability explicitly. These values may guide acceleration/order only and may not decide topology or relations.

### 5.14 Independent-verification evidence

Store reconstructible evidence for triangle cycles, pairing runs, reciprocal endpoints, edge role/provenance, fan transitions and canonical traversal, facet/shell membership, map totals, bound containment inputs, canonical ordering, versions, and digest inputs. Producer booleans such as `all_edges_paired=true` are insufficient.

## 6. Exact count, capacity, and resource preflight

### 6.1 Count identities

From predecessor tables compute with Component 01 checked arithmetic:

- `V = source vertex occurrence count`;
- `F = source triangle count`;
- `H = 3F`;
- `B = source-boundary triangle edge-use count`;
- `D = internal-diagonal triangle edge-use count`;
- `E_s = source undirected edge count`;
- `E_d = facet-local diagonal count`;
- `E = E_s + E_d`.

Require:

```text
H = B + D
B = 2E_s
D = 2E_d
H = 2E
sum(vertex outgoing incidence counts) = H
```

Also require Component 04 triangle/facet-local identities and Component 02 source-edge counts to agree record by record. Empty operands require every count zero. Count equations supplement, never replace, per-record verification.

### 6.2 Representability

Before allocation prove all counts, products, offsets, IDs, canonical byte lengths, and temporary bounds fit:

- the relevant strong-ID ordinal domain;
- the chosen checked internal storage index;
- public `I` where a later adapter requires it;
- host `size_t`;
- Component 01 entity/byte/work limits; and
- codec count/length fields.

Never allocate from unchecked `size_t` arithmetic. In particular check `3F`, `2E`, prefix sums, fan offsets, map sizes, sort-key storage, verifier duplication, and canonical encoding sizes.

### 6.3 Resource reservations

Reserve and account for:

- persistent vertices, triangles, halfedges, edges, fans, groups, maps, bounds, evidence, canonical bytes, diagnostics, and replay;
- temporary proposals, full keys, sort buffers, run descriptors, incidence lists, fan marks, group builders, encoding buffers, and independent-verifier workspaces; and
- abstract work for proposal generation, comparison/sort, grouping, fan transitions, map reconstruction, bound containment, encoding, and verification.

Charge comparisons/work deterministically under Component 01 policy. Pathological inputs terminate through typed work/resource failure. Reconcile reserved versus actual resources before promotion; release every temporary lease on success, failure, cancellation, or rollback.

## 7. Predecessor validation

Before construction, independently validate:

1. context owner, operand, strict environment, policy, and supported versions;
2. Component 02 and 04 artifact IDs/digests and successful verifier dispositions;
3. every source vertex/facet/shell/triangle/edge-use ID owner/domain/range;
4. every triangle has three distinct source vertices, definite inherited orientation, support evidence, and exactly three local edge-use records matching its cycle;
5. every edge use is exactly one supported role;
6. every source-boundary use references the correct source directed and undirected edge, endpoints, facet, shell, and reciprocal source use;
7. every source undirected edge has exactly two opposite validated directed uses and exactly two Component 04 boundary uses;
8. every internal diagonal has exactly two opposite uses in one source facet and no source-edge identity;
9. Component 04 facet groups exactly partition triangles/diagonals;
10. Component 02 shell groups exactly cover the referenced vertices/facets and Component 04 triangle shell membership agrees; and
11. all inherited required bounded references are present, finite, version-valid, and owner-correct.

A contradiction in a committed predecessor artifact is `internal_invariant_error` attributed to the predecessor, with the least canonical conflicting records. Do not repair or reinterpret it.

## 8. Deterministic serial construction

### 8.1 Vertices

Enumerate source vertices in predecessor canonical order. Build one proposal per V1 source vertex with the canonical key from Section 5.3. Sort full keys if the predecessor view does not already prove the same order. Reject duplicates. Assign dense manifold vertex IDs only after validation.

### 8.2 Triangles and cycles

Enumerate source triangles, validate their edge-use cycles, compute the least orientation-preserving rotation, build full triangle proposals, sort, reject duplicates, and assign dense triangle IDs. Allocate exactly three contiguous halfedges per final triangle, with checked arithmetic. Fill origin, triangle, slot, next/previous, role payload, source provenance, and group references. Pair/edge IDs remain unresolved private handles until grouping.

### 8.3 Pairing proposal sort

Emit one pairing proposal per halfedge. Sort by:

```text
(pairing_key,
 origin_manifold_vertex_key,
 destination_manifold_vertex_key,
 triangle_key,
 local_slot,
 predecessor_edge_use_key)
```

Use full comparisons. Partition into maximal equal pairing-key runs. Every run must have cardinality two.

### 8.4 Source-edge runs

For each source-edge run require:

- both uses carry the same exact source undirected edge;
- they correspond to its two exact Component 02 directed uses;
- directed source and manifold endpoints reverse;
- incident source facets match the validated adjacency and are distinct when required by predecessor schema;
- operand and shell agree;
- neither use is an internal diagonal; and
- no third use exists.

### 8.5 Internal-diagonal runs

Require:

- same source facet and exact Component 04 diagonal;
- two distinct incident source triangles;
- reversed source/manifold endpoints;
- same operand/shell/facet group;
- neither use carries any source directed/undirected edge ID;
- fixed non-source/non-owner/non-barrier semantics; and
- no third use.

### 8.6 Edge IDs, pairs, and representatives

Build complete edge proposals from verified runs. Sort by complete edge key, reject duplicates, assign dense edge IDs, then resolve both halfedges' edge and reciprocal pair fields. Verify reciprocity immediately. Select the low-canonical-vertex to high-canonical-vertex halfedge as representative. Do not derive representative direction from coordinates or face order.

### 8.7 Vertex fans

Group outgoing halfedges by origin vertex using full IDs. Sort each group by full halfedge key only to choose the start and produce deterministic diagnostics; topology traversal itself uses `pair(previous(h))`. Traverse and verify as Section 5.9. Publish the resulting canonical sequence and vertex representative. A valid nonempty closed V1 source vertex must have one nonempty closed fan.

### 8.8 Facet and shell groups

Reconstruct facet membership from triangle records and internal-edge provenance; compare with Component 04 rather than copying counts. Reconstruct shell membership from source records and compare with Component 02. Sort member lists by complete key. Build forward and inverse maps, bounds, and group digests only after membership verification.

### 8.9 Empty operand

Publish a canonical empty operand with valid owner/operand/version/predecessor links, zero entity/table counts, empty canonical ranges, verified empty disposition, resource statistics, and deterministic digests. It must still pass version, capacity, owner, codec, and independent-verifier gates.

## 9. Conservative geometry attachments

### 9.1 Vertex points

Reference or copy the immutable Component 03 bounded point and exact nominal bit pattern for each source vertex. Verify owner, precision lineage, finiteness, and that no coordinate-based deduplication occurred.

### 9.2 Edge bounds

Construct each edge bound once from its two bounded endpoints through the frozen Component 03 segment-bound formula. Require a finite closed enclosure containing both endpoint enclosures and every permitted segment realization. The two halfedge uses reference the same edge attachment.

### 9.3 Triangle bounds

Use the three inherited bounded points and frozen Component 03 triangle-bound formula, or validate a compatible Component 04 bound. Require containment of all three points and consistency with inherited triangle precision/support. Do not shrink or recompute with an incompatible formula.

### 9.4 Facet and shell bounds

Compute deterministic conservative unions in canonical member order using a frozen pairwise reduction. Require each child bound contained. Empty groups are allowed only where predecessor semantics permit them. Overflow/non-finite unions fail; never clamp smaller.

### 9.5 Optional nominal data

If acceleration requires nominal area vectors, centroids, axes, or scales, use Component 03 exponent-safe scaled arithmetic, record formula/version and availability, and test overflow/extreme exponents. Absence is not malformed unless the selected downstream provider contract requires that attachment. No optional value is authoritative.

## 10. Producer-side structural verification

Before encoding or publication, reconstruct from private records rather than trusting counters:

### 10.1 Entities and cycles

- dense IDs are unique, owner/domain/range correct, and match canonical order;
- every source vertex/triangle maps exactly once;
- every triangle has exactly three distinct vertices and halfedges;
- next/previous are total mutual inverses and close after three steps;
- halfedge endpoints agree with the triangle cycle;
- local roles/provenance exactly match Component 04.

### 10.2 Pairing

- every halfedge belongs to exactly one edge;
- every edge has exactly two uses;
- pair is total, symmetric, non-self, and remains in the same edge;
- endpoints reverse exactly;
- role-specific predecessor identities agree;
- source/internal classes never mix; and
- representative direction follows the frozen key policy.

### 10.3 Vertex links

Independently regroup outgoing halfedges, rerun transition traversal, verify closure/inverses/coverage, compare stored canonical sequences, and reject open, duplicated, or multiple cycles.

### 10.4 Groups and maps

Rebuild facet and shell membership from entity provenance. Verify exact partition/coverage, group lists, inherited boundary/support/orientation/nesting evidence, and every forward/reverse map. Caller maps may not influence semantic ordering.

### 10.5 Bounds

For each edge, triangle, facet, and shell, reconstruct required child inputs and verify closed containment through Component 03. Verify finite encoding, formula versions, precision lineage, and optional attachment semantics.

### 10.6 Canonical order and transaction eligibility

Recompute full keys and require strictly increasing canonical tables or exact canonical permutations. Validate all reserved fields, range descriptors, resource statistics, no temporary handles, and no active private lease. Only a producer-verified private artifact may proceed to codec and independent verifier.

## 11. Canonical ordering, digests, and codec

### 11.1 Physical order

Publish, preferably physically:

- vertices by complete vertex key;
- triangles by complete triangle key;
- halfedges by triangle ID then local slot;
- edges by complete role-specific edge key;
- fans by vertex key with incidences in canonical traversal order;
- facet/shell groups by predecessor canonical identity;
- maps by domain/source key; and
- evidence/attachments by owner feature key and evidence kind.

No unordered iteration is serialized.

### 11.2 Digests

Maintain distinct digests:

1. **source semantic digest** — forwarded Component 02/04 source feature/facet/shell semantics, excluding internal diagonal choice where Component 04 defines semantic invariance;
2. **exact topology digest** — exact triangles, local rotations, halfedges, pairs, internal diagonals, fans, groups, and maps;
3. **geometry attachment digest** — bounded points/bounds, precision lineage, formula versions, and optional nominal availability/data;
4. **replay/presentation digest** — caller ordering/bit provenance and diagnostic policy fields; and
5. **complete artifact digest** — header versions plus all required canonical sections.

Source ownership in later stages must use source identities/semantic digest, not only exact triangle topology.

### 11.3 Encoding

Encode field-by-field with Component 01 canonical primitives: explicit widths/endian, enum tags, lengths, option discriminants, floating bit patterns, strong IDs, and zero reserved fields. Never serialize native structs, pointers, padding, capacity, map tree layout, or hash buckets. Decoder checks every length/count before allocation, rejects unknown required tags/versions and trailing required-section bytes, then runs the independent verifier before accepting an artifact.

Freeze golden V1 bytes for empty, tetrahedron, triangulated box, concave polygon-derived facet, disconnected shells, cavity/island, duplicate-coordinate identities, and high-valence fixtures.

## 12. Independent `CanonicalHalfedgeVerifier`

### 12.1 Independence boundary

The verifier receives immutable Component 02/03/04 artifacts, the proposed Component 05 artifact, and Component 01 verification/resource services. It must not call producer helpers for triangle rotation, pairing-key grouping, edge ID assignment, fan construction, group assembly, map construction, canonical ordering, or producer cached booleans/digests.

It may share strong-ID/schema definitions, canonical byte primitives, and low-level Component 03 containment services. Implement separate traversal, grouping, sorting, fan reconstruction, membership reconstruction, map comparison, and encoding traversal.

### 12.2 Verifier workflow

Independently:

1. validate owner, operand, versions, reserved fields, predecessor links/digests, counts, and ranges;
2. rebuild the expected source-vertex and source-triangle domain;
3. independently compute least orientation-preserving triangle rotations;
4. reconstruct each triangle's three directed edges from vertices/local source edge uses;
5. reclassify edge role from Component 04 tagged provenance;
6. rebuild role-specific edge groups and require exactly two reversed uses;
7. rebuild expected edge keys/order/IDs, pairs, and representatives;
8. reconstruct all outgoing incidences and fan cycles with an independently written traversal;
9. rebuild facet/shell groups from provenance and predecessor artifacts;
10. rebuild every forward/reverse map;
11. independently verify all required bound containment and formula/precision references;
12. recompute all canonical keys, arrays/permutations, digests, and bytes;
13. compare stored data against reconstruction field by field;
14. verify resource statistics are conservative and no temporary/private handle escaped; and
15. return one deterministic verified disposition or least-key typed rejection.

### 12.3 Mutation rejection

Required mutations include:

- remove/duplicate/reorder a vertex or triangle;
- reverse or inconsistently rotate one triangle;
- alter a triangle local slot, source facet, shell, or orientation reference;
- delete/add a halfedge or assign four halfedges to a triangle;
- break next/previous closure;
- alter edge class or role-specific provenance;
- delete/duplicate a pairing use, make pair one-way/self, pair same-direction/equal endpoints, mix source/internal classes, pair diagonals across facets, or add a third use;
- alter canonical representative;
- change a halfedge origin/destination/edge/triangle;
- split one vertex fan, join two identity-distinct fans, create an open fan, duplicate an incidence, or change fan order/representative;
- alter facet/shell membership, boundary, occupied-side evidence, or group digest;
- omit/duplicate/change any forward/reverse provenance entry;
- merge coordinate-coincident vertices;
- shrink/mislabel/substitute a bound or precision/formula reference;
- permute a canonical array without updating maps;
- alter a version, reserved field, length, count, digest, or canonical byte; and
- forge producer counts/summary evidence while incidence remains corrupt.

Every mutation must be rejected from reconstruction, with deterministic least offending feature and stable subcode.

## 13. Immutable query contract

Provide bounded deterministic, allocation-free reads for later stages.

### 13.1 Vertex queries

- source identity, operand, shell, caller provenance;
- bounded point/precision reference;
- canonical outgoing representative;
- fan span in oriented order; and
- exact source/internal mapping.

### 13.2 Triangle queries

- three vertices/halfedges in canonical orientation;
- source triangle/facet/shell;
- inherited orientation/support and bound;
- facet/shell group; and
- exact triangulation/source semantic lineage.

### 13.3 Halfedge/edge queries

- origin/destination, next/previous, pair, triangle, edge;
- edge class by identity alone;
- canonical endpoints and directed representative;
- both incident halfedges/triangles/facets;
- source edge or internal diagonal provenance;
- whether crossing stays in one source facet; and
- conservative segment bound.

### 13.4 Relationship and group queries

By exact IDs determine whether two triangles share a vertex, edge, source facet, or shell; whether an edge is incident to a triangle; facet member triangles/internal diagonals/boundary; shell membership/occupied-side evidence; and all required bounds/digests.

Queries must not perform coordinate equality, tolerance lookup, dynamic allocation, schedule-dependent caching, or hidden recomputation of authoritative geometry. Document expected `O(1)`, `O(log n)`, or bounded span traversal complexity.

## 14. Alternative legal triangulation semantics

Integrate Component 04's test-only alternative legal triangulation builder. For each accepted alternative:

- run the full Component 05 producer, codec, and independent verifier;
- allow exact triangle, internal-diagonal, halfedge, edge, fan ordering, and exact topology digest to differ where triangulation differs;
- require forwarded source semantic digest, source vertices/edges/facets/shells, caller provenance, and occupied-side semantics unchanged;
- require every internal diagonal remains non-source/non-owner/non-barrier;
- require source boundary edge records and source-feature queries equivalent; and
- feed each variant to Component 06 test adapters and later available stages to prove Boolean source semantics do not depend on diagonal choice.

Do not require byte identity between genuinely different legal triangulations. Require semantic equivalence under a frozen comparison view.

## 15. Resources, cancellation, transactions, and concurrency

### 15.1 Transactions

Use one Component 05 stage transaction containing two private operand subtransactions. Construction order cannot grant partial publication. Persistent leases transfer only with the final wrapper. If either operand fails, select the canonical failure, join all work, discard both private candidates, release all temporary/proposed persistent resources, and publish nothing.

### 15.2 Deterministic cancellation points

Poll at:

- operand start;
- predecessor validation chunks;
- proposal generation chunks;
- before/after deterministic sorts;
- edge-run batches;
- pair-resolution boundary;
- vertex-incidence and fan batches;
- facet/shell group batches;
- geometry-attachment batches;
- producer verification table batches;
- codec section boundaries;
- independent verifier table/group/fan/bound batches; and
- immediately before commit.

Do not poll halfway through a local atomic record fill, pair closure, or fan transition. Finish the private local operation, then discard at the next checkpoint. Cancelled runs expose no partial artifact or leaked lease.

### 15.3 Component 17 boundary

Keep an executable serial semantic provider. Parallel execution may:

- generate immutable vertex/triangle/halfedge proposals in private canonical ranges;
- locally validate proposals;
- sort private runs and merge by full canonical key;
- verify disjoint canonical partitions privately; and
- return private failures/evidence.

Parallel tasks may not assign final semantic IDs, mutate shared published records, pair across tasks without canonical merge, depend on worker floating environment without qualification, or expose completion order. Final global sort/group, ID assignment, pair closure, fan reconstruction, canonical encoding, failure arbitration, and transaction commit follow frozen deterministic semantics. Prove byte/failure/resource-accounting equivalence for thread counts 1, 2, and configured maximum plus forced schedule permutations.

### 15.4 Fault injection

Inject allocation, reservation, sort, codec, bound-provider, verifier, worker-start, worker-join, and cancellation failures at every transaction boundary. Require no active reservations, no published IDs/handles, no unjoined worker, predecessor artifacts unchanged, and deterministic replayable failure.

## 16. Test and validation plan

### 16.1 Entity and cycle tests

Cover empty operands, tetrahedra, triangulated boxes, polygon-derived facets, high-valence vertices, disconnected shells, cavities/islands, canonical triangle rotation, every accessor, and index capacity boundaries. Independently reconstruct triangle cycles and exact endpoint/provenance agreement.

### 16.2 Pairing/provenance tests

Fixtures must include ordinary source edges between facets, several internal diagonals in one concave facet, adjacent coplanar facets that remain separate, coordinate-identical distinct vertex IDs, repeated coordinate patterns across shells, different legal facet diagonal orientations, and IDs near capacity. Verify role discovery without geometry and exact two-use opposite pairing.

### 16.3 Vertex-link tests

Independently reconstruct links for valence 3, 4, high valence, multiple facets around a vertex, diagonals incident to source vertices, coordinate-coincident vertices in separate components, cavity/island shells, and all canonical input permutations. Inject open fans, duplicate incidences, bow-ties, disconnected cycles, wrong twin transitions, and cross-identity links.

### 16.4 Geometry-bound tests

For vertex/edge/triangle/facet/shell bounds cover exact coordinates, one-ULP uncertainty, signed zero, subnormals, large translations with small features, extreme finite exponents, long thin triangles, zero-width axes, and coordinate-coincident distinct features. An in-tree exact/higher-precision test oracle verifies containment. Shrinking a bound below any permitted realization must fail.

### 16.5 Canonicalization/metamorphic tests

Apply vertex-array permutation with remapped indices, facet/triangle/shell permutation, ring rotation, legal alternative triangulation, exact power-of-two scale, exactly representable translation, axis permutation, global orientation reversal with corrected solid policy, worker counts, task partitions, and delayed/reversed schedules.

Where Component 04 defines unique canonical triangles, Component 05 bytes must be identical. Where legal triangulations differ, source-semantic queries/digests and later Boolean semantics must remain equivalent even if exact topology differs.

### 16.6 Codec tests

Golden encode/decode for all schema classes. Test truncation at every byte, count/length overflow, unknown tag/version, nonzero reserved bits, duplicate/missing sections, trailing bytes, malformed optional discriminants, bad IDs, altered floating bit patterns, and digest substitution. Decode never publishes before independent verification.

### 16.7 Mutation tests

Implement every Section 12.3 mutation as a targeted mutator. Require zero surviving required mutations, stable rejection subcodes, least canonical offending feature, and rejection from reconstructed incidence rather than counts/checksums alone.

### 16.8 Exact oracle, properties, fuzzing, shrinking

Use test-only in-tree arbitrary-precision integer/rational facilities planned by Components 03/16, never production-linked. Generate closed triangular manifolds and polygon-derived source complexes from exact templates. The oracle independently computes endpoint identity incidence, edge multiplicity/direction, vertex links, Euler/handshake identities, and exact coordinate containment for sampled permitted realizations.

Fuzz topology-preserving subdivisions, facet triangulations, shell count/nesting, valence, source permutations, duplicate coordinates without identity merge, ULP perturbations, exponent ranges, and capacity boundaries. Also structurally mutate valid artifacts. Serialize exact bits, IDs, versions, limits, and failure key; shrink by removing nonessential shells/facets/subdivisions, reducing valence/magnitude/ULP offsets, and simplifying mutations while preserving category. Store fixed bugs permanently.

### 16.9 Resource/cancellation/concurrency tests

For every resource/work class test limit-minus-one, exact limit, and limit-plus-one. Cancel at each checkpoint. Run serial and parallel builds with delayed tasks, reversed partitions, and different worker counts. Accepted outputs must be byte-identical; failures must be category/key-identical. No run may leak resources or expose partial data.

### 16.10 Structural performance tests

Use counters, not wall time alone, to establish:

- linear proposal/entity expansion;
- `O(H log H)` or better deterministic grouping;
- `O(H)` total fan traversal;
- no all-vertex, all-triangle, or all-edge pair scans;
- linear persistent memory;
- bounded verifier duplication/work; and
- deterministic accounting across thread counts.

Add large disjoint-shell, high-valence, many-triangle-per-facet, and adversarial canonical-key fixtures. Fail qualification if comparison/work counters exceed documented bounds absent a typed resource exit.

## 17. Diagnostics, replay, and observability

Every failure report must contain enough structured data to identify:

- context/operand/stage/checkpoint and versions;
- source and internal feature IDs/keys;
- expected/observed role, endpoints, counts, pair/fan transition, group membership, or map entry;
- predecessor artifact/digest references;
- relevant bound/formula/precision evidence;
- resource reservation/use/limit; and
- deterministic replay payload.

Expose structural statistics for vertices, triangles, halfedges, edge classes, fan valence distribution, facet/shell groups, sort comparisons, fan transitions, bound operations, verifier work, bytes, and resources. Statistics are diagnostic and do not change semantic bytes unless the frozen replay policy explicitly includes them.

Replay must reconstruct exact input bit patterns/options/versions, rerun Component 05 without caller mesh access, and reproduce the same artifact digest or primary failure. Redact only according to Component 01 policy without changing failure arbitration.

## 18. Integration boundaries

### 18.1 Component 04

Component 05 consumes Component 04 exactly as specified. It does not alter source triangles or diagonal choices. Compile-time/query contract tests must ensure no implementation falls back to coordinates or only triangle IDs when source facet/edge lineage is required.

### 18.2 Component 06

Component 06 receives canonical vertices, triangles, undirected edges, directed representatives, both incident halfedges, exact edge class, facet/shell provenance, and conservative bounds. Provide a test adapter that enumerates the candidate-domain inputs and proves each undirected edge enters at most once through its canonical representative while both face uses remain queryable. Do not implement BVH/candidate enumeration here.

### 18.3 Components 07-10

Preserve source-facet/source-feature lineage across internal diagonals. A later relation/event/classification may not treat an internal diagonal as a source owner or barrier. Queries must let later stages distinguish topology adjacency from source semantic adjacency without geometry.

### 18.4 Components 11-15

Component 05 is source-side topology only. Do not reuse its records as mutable output topology. Components 11-14 create separate output-occurrence/edge/cycle/triangulation/cleanup/public artifacts. Component 15 may use source lineage but independently reconstructs final public topology.

### 18.5 Legacy APIs

Do not change legacy mesh verification, orientation, hole, refinement, remeshing, or Boolean behavior. Any future extraction of generic utilities requires separate proof of source compatibility and no weakening of strong identity semantics.

## 19. Implementation sequence and handoff gates

Implement in this order; each gate includes tests and leaves the branch buildable:

1. Add Component 05 versions, enums, strong-ID domains, checkpoints, subcodes, resources, and uniqueness tests.
2. Add immutable schemas and checked views with no producer.
3. Add exact count/capacity/resource preflight and empty path.
4. Add narrow Component 02/03/04 capability views and predecessor validation.
5. Add internal vertex proposals and total vertex maps.
6. Add independent orientation-preserving triangle rotation and canonical proposals.
7. Add fixed triangle-major halfedge cycles and unit tests.
8. Add role-specific pairing keys and deterministic proposal sort.
9. Add source-edge grouping/pairing and provenance checks.
10. Add internal-diagonal grouping/pairing and fixed non-source semantics.
11. Add canonical edge order/IDs, pair closure, and representatives.
12. Add producer cycle/pair verification and pairing mutations.
13. Add vertex incidence and `pair(previous(h))` transitions.
14. Add canonical fan traversal/storage and fan mutations.
15. Add facet/shell groups and total provenance maps.
16. Add conservative geometry attachments.
17. Add optional nominal attachments with explicit availability.
18. Add complete producer structural verification.
19. Add canonical order, codec, digests, decode validation, and golden bytes.
20. Implement the independent verifier with separate reconstruction code.
21. Add complete mutation suite and prove zero survivors.
22. Add exact-oracle/property generators, fuzzing, shrinking, and regressions.
23. Add duplicate-coordinate, high-valence, shell, extreme-bound, and canonical metamorphic suites.
24. Add Component 04 alternative-triangulation integration and semantic equivalence tests.
25. Add Component 06 query/candidate-domain adapter without implementing Component 06.
26. Add structural counters and prove `O(V + F + H log H)`/linear-memory gates.
27. Add Component 17 private task/canonical merge hooks while retaining serial equivalence.
28. Run Component 01-04 full regressions under supported strict builds.
29. Run debug/release, sanitizer, codec, owner/version, overflow, resource, cancellation, fault-injection, replay, worker-count, and schedule matrices.
30. Update `tracker.md` only after this plan addresses every Component 05 requirement and handoff gate.

## 20. Definition of done

Component 05 is complete only when all of the following are true:

- only Component 05 scope is implemented and no ignored legacy Boolean source is called;
- existing Ygor mesh/orientation/hole/refinement/verification code is used only as documented carriers or regression references, not as the provider;
- Component 02/04 owner, version, digest, source incidence, triangle, facet, shell, orientation, and provenance contracts are defensively verified;
- checked count identities establish `H=3F`, `H=2E`, `B=2E_s`, `D=2E_d`, and total outgoing incidence `H`;
- every V1 source vertex occurrence maps exactly once without coordinate welding;
- every source triangle maps exactly once through a canonical orientation-preserving rotation;
- every triangle owns exactly three triangle-major halfedges in a closed oriented cycle;
- every halfedge belongs to exactly one triangle and one role-specific undirected edge;
- every source edge pairs from exact Component 02/04 identities with two opposite uses;
- every internal diagonal pairs only within one source facet with two opposite uses and remains non-source/non-owner/non-barrier;
- pairing is total/symmetric, endpoints reverse exactly, and every edge has the stable low-to-high canonical representative;
- every vertex has one independently verified closed oriented fan covering all incidences once;
- coordinate-coincident distinct vertices never enter each other's maps, edges, or fans;
- facet groups preserve exact boundary/support/orientation/member/provenance semantics without merging coplanar facets;
- shell groups preserve exact membership/nesting/occupied-side evidence without recomputation;
- all forward/reverse maps are total, canonical, owner-checked, and independently reconstructed;
- all required bounds are finite, closed, conservative, formula-versioned, and independently containment-checked;
- optional nominal geometry is exponent-safe, explicitly non-authoritative, and deterministically available/absent;
- IDs, arrays, fan sequences, groups, maps, bytes, and failures are independent of caller order, allocation, hashing, tasks, workers, and schedule;
- source semantic identity remains invariant under legal retriangulation while exact topology digest commits to changed layout;
- immutable queries give Component 06/later stages bounded deterministic exact incidence, edge class, provenance, groups, and bounds without mutable caches;
- producer verification reconstructs cycles, pairing, fans, groups, maps, bounds, and canonical order before encoding;
- independent verification repeats all obligations without producer grouping/pairing/fan/canonicalization helpers or trust in counts/booleans/digests;
- every required structural, provenance, bound, codec, owner/version, and digest mutation is rejected with stable least-key failure;
- unit, exact-oracle, golden, duplicate-coordinate, high-valence, shell, alternative-triangulation, metamorphic, fuzz/shrink, adversarial, resource, cancellation, concurrency, codec, and performance suites pass;
- serial V1 remains executable and optimized/parallel providers are byte- and failure-identical;
- structural counters prove `O(V + F + H log H)` or better construction, `O(H)` fan traversal, linear persistent storage, and no accidental all-pairs scans;
- resource, overflow, cancellation, fault-injection, transaction, and replay tests prove zero partial publication and zero leaked reservations;
- Component 01-04 regressions remain passing under strict targets;
- production and normative-test code are portable standard-library-only C++17 with no external dependency; and
- `tracker.md` marks component 5 complete only after this fully complete implementation plan has been added and reviewed.
