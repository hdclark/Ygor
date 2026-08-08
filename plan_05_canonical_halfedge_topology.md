# Plan 05: Canonical Halfedge Topology

## 0. Scope and fixed V1 result

Implement only Component 05 from `component_05_canonical_halfedge_topology.md`.

The stage accepts the immutable, independently verified Component 02 and Component 04 artifacts plus Component 01 and Component 03 capabilities. It publishes exactly one immutable:

```cpp
canonical_source_manifolds<T,I>
```

containing one:

```cpp
canonical_halfedge_operand<T,I>
```

for each operand.

V1 is a deterministic sort-and-scan provider with:

- contiguous immutable arrays;
- one Component 05 vertex per represented Component 04 source-vertex identity;
- one orientation-preserving canonical record per Component 04 source triangle;
- exactly three triangle-major halfedges per triangle;
- provenance-keyed source-edge and internal-diagonal grouping;
- reciprocal pairs closed only after complete run validation;
- exact fan traversal through `pair(previous(h))`;
- explicit source-facet and source-shell groups;
- committed-geometry-basis-aware conservative bounds;
- owner-free canonical semantic keys and bytes;
- a separately implemented independent verifier; and
- an executable serial semantic reference that every optimized or parallel provider must reproduce.

Target complexity is:

```text
time:       O(V + F + H log H)
persistent: O(V + F + H + E)
```

where:

```text
H = 3F
H = 2E
```

No failed, cancelled, partially paired, partially encoded, resource-exhausted, or verifier-rejected artifact may publish.

## 1. Non-negotiable architecture constraints

The implementation must:

- treat exact indexed incidence and provenance as authoritative topology;
- consume only immutable predecessor artifacts and controlled capabilities;
- preserve distinct source identities even when nominal coordinates, committed realized coordinates, and bounds are bit-identical;
- preserve Component 04 triangle orientation and exact local edge-role alignment;
- pair source edges and internal diagonals from authoritative identities, never coordinates;
- preserve exact nominal source bits and the Component 02/04 committed coherent geometry basis as separate lineages;
- make source-facet and shell semantics queryable without geometry;
- keep internal diagonals available as bookkeeping topology while preventing them from becoming source-feature owners;
- use runtime owner tokens only for lifetime and handle validation;
- exclude raw runtime owner tokens from all canonical semantic material;
- preflight every count, byte, offset, work unit, and index conversion;
- build privately and publish atomically only after independent verification;
- remain strict portable C++17; and
- use no external, vendored, downloaded, optional, or runtime-invoked dependency.

The component must not:

- reread caller-owned `fv_surface_mesh<T,I>`;
- call a legacy Boolean implementation as the provider;
- use coordinate equality, distance, tolerance, spatial cells, Morton codes, or hashes alone to create or merge topology;
- import Component 01 isolated non-semantic vertices;
- reverse, omit, replace, or retriangulate a Component 04 triangle;
- repair missing or inconsistent predecessor topology;
- choose a new source geometry realization, plane, projection, shell orientation, or occupied side;
- silently replace a committed constructed coherent realization with nominal geometry;
- merge approximately coplanar source facets;
- make a facet-local internal diagonal a source owner, symbolic owner, winding barrier, or retained output feature;
- hide Component 06 candidate policy in a generic edge `eligible` flag;
- assign semantic IDs from runtime owner tokens, pointers, source ordinals, allocation order, hash iteration, worker number, task completion, or traversal accidents;
- serialize native structs, padding, pointers, container capacity, runtime owner tokens, or unordered layout;
- throw exceptions for expected contract, geometry-basis, resource, cancellation, codec, or verification failure; or
- allow production or normative-test translation units to inherit the parent project's unsafe fast-math behavior.

## 2. Independent review findings and mandatory decisions

### 2.1 Runtime owner tokens are validation metadata, not canonical identity

The previous Component 05 plan placed `owner` inside vertex and pairing keys. That conflicts with Components 01, 02, and 04, which require build-instance- and invocation-anchor-independent canonical semantics.

Correct V1 rule:

- records and handles carry or reference an owner token for validation;
- semantic keys exclude the owner token;
- canonical strong-ID encodings contain stable domain, operand role where applicable, schema version, and canonical ordinal or semantic key;
- canonical bytes and all semantic digests exclude the owner token;
- deterministic failure arbitration excludes raw owner values;
- wrong-owner access still fails before dereference; and
- semantically identical invocations under different owner anchors produce identical Component 05 semantic artifacts.

Any implementation following an owner-bearing key from an earlier draft is non-conforming.

### 2.2 The represented vertex domain is not the raw source array

Component 02 declares isolated snapshot vertices non-semantic and Component 04 hands off only represented topology. Therefore:

```text
V = number of distinct Component 04 source-vertex identities referenced by source triangles
```

not `source_snapshot.vertices.size()`.

The preflight, maps, empty path, group membership, bounds, codec, verifier, and tests must all use the represented domain. A source-snapshot vertex with no accepted facet use must not appear in Component 05.

### 2.3 Nominal provenance and committed coherent geometry basis must remain distinct

Components 02 and 04 may accept either:

- `nominal_embedded`; or
- `constructed_coherent_realization`.

Component 05 must retain exact nominal source bits for provenance while using the committed basis and inherited enclosures for required geometry attachments. It must never silently fall back to nominal points.

### 2.4 Internal diagonal semantics require separate axes

Component 04 correctly says an internal diagonal is not a source feature. Component 06 V1 deliberately includes internal diagonals as bookkeeping broad-phase edges. These statements are compatible only if Component 05 exposes separate properties.

Do not store one generic eligibility bit. Store exact class and fixed source-semantic properties; let Component 06's versioned domain policy decide candidate visibility.

### 2.5 Existing Ygor code requires a reuse audit, not blind reuse or blanket duplication

The following assessment is mandatory before implementation:

- `fv_surface_mesh<T,I>` remains the public carrier, not the internal topology.
- `YgorMeshesVerification.h/.cc` contains useful exact-index ideas: ordered endpoint pairs, edge-use counts, and opposite-direction audits. Its public routines are not Component 05 providers because they read mutable public arrays, lack role/provenance domains, do not build twins or fans, use booleans/exceptions, and have no owner/version/transaction/verifier contract.
- `YgorMeshesBoolean5.cc` contains halfedge-shaped records, but it makes a long-double, fixed-grid snap coordinate the topology key and clamps large values. That violates the broad plan's identity and bounded-geometry model. Its arrangement/output topology is not a source-manifold substrate.
- orientation, hole-filling, refinement, remeshing, constrained-Delaunay, and related adjacency code use mutable topology, coordinate representatives, geometry-angle ordering, repair behavior, or unrelated semantics.

Before adding a duplicate low-level exact-index utility, inspect whether a small pure helper can be extracted or generalized without weakening its existing callers. A permissible extraction must:

- be independent of `fv_surface_mesh`;
- operate on typed exact IDs or caller-provided complete keys;
- perform no coordinate comparison or repair;
- have deterministic ordering and checked arithmetic;
- preserve existing public behavior;
- be usable without sharing producer and independent-verifier high-level control flow; and
- receive direct unit tests.

If those conditions are not met, document the rejection and implement the bounded-subsystem utility locally. Do not retrofit a deficient legacy provider merely to claim reuse.

## 3. Existing Ygor assessment and reuse boundaries

### 3.1 Public mesh carrier

Use `fv_surface_mesh<T,I>`, `vec2<T>`, and `vec3<T>` only through already frozen predecessor capabilities or as nominal value carriers. Do not mutate `fv_surface_mesh` into an internal halfedge data structure.

`involved_faces` is optional, mutable, presentation-derived, and not a cyclic fan. Ignore it.

### 3.2 Verification helpers

Do not call these as Component 05 producer or independent-verifier authorities:

- `ClassifyEdges`;
- `HasConsistentOrientation`;
- `IsClosedManifold`; or
- `ValidateClosedTriangularMesh`.

A newly extracted low-level ordered-edge or exact-direction audit helper may be shared only if it satisfies Section 2.5. Producer grouping and independent-verifier grouping must remain independently implemented.

### 3.3 Legacy Boolean 5

Do not call, adapt wholesale, or copy the control flow of `YgorMeshesBoolean5`.

Its snap-key topology, fixed `1e-12` grid, `long double`, coordinate clamping, coordinate-based output vertex identity, monolithic mutable arrangement, and exception-oriented behavior are incompatible.

Permitted use is limited to:

- non-normative regression fixtures;
- benchmark comparison;
- studying names or record shapes without copying semantics; or
- extracting a tiny pure non-geometric utility that independently passes the Section 2.5 gate.

### 3.4 Repair and triangulation utilities

Do not reuse coordinate-representative maps, orientation repair, hole filling, remeshing, generic triangulation, angle-sorted adjacency, or mutable neighborhood code. Component 05 is a faithful source-topology representation, not a repair stage.

### 3.5 Mandatory predecessor reuse

Consume rather than duplicate:

From Component 01:

- owner-checked strong-ID domains and publication;
- checked count, byte, offset, and index arithmetic;
- typed stage outcomes and stable subcodes;
- transactions, immutable publication, resource leases, cancellation, and deterministic error arbitration;
- canonical primitives, SHA-256, diagnostics, and replay; and
- strict floating/build environment qualification.

From Component 02:

- represented canonical source vertices;
- source directed and undirected edges;
- normalized source rings and facets;
- shells, nesting, orientation, and occupied-side evidence;
- caller/source-position maps;
- coherent geometry-basis disposition and source point realization references; and
- source semantic digest lineage.

From Component 03:

- bounded points and precision lineage;
- conservative segment, triangle, and union bounds;
- finite/containment checks;
- qualified optional nominal calculations; and
- formula/version validation.

From Component 04:

- canonical source triangles and oriented cycles;
- local edge-role records;
- source-boundary uses;
- facet-local diagonal identities and opposite uses;
- source-facet groups;
- support, projection, orientation, geometry-basis, and bound references;
- source-semantic and exact-triangulation digests; and
- independently verified disposition.

Do not create a second source-edge registry, shell classifier, plane model, projection chooser, geometry realization, precision ledger, triangulation provenance system, canonical codec, hash, or transaction framework.

## 4. File and target layout

Add under `src/YgorMeshesBooleanBounded/`:

- `CanonicalHalfedgeTypes.h` — closed enums, tags, schema constants, strong-ID aliases, complete semantic key types, and query result types;
- `CanonicalHalfedgeOperand.h` — immutable operand artifact records and checked read-only views;
- `CanonicalSourceManifolds.h` — immutable two-operand wrapper and cross-artifact validation;
- `CanonicalHalfedgeBuild.h/.cc` — typed stage entrypoint and fixed phase orchestration;
- `CanonicalHalfedgePreflight.h/.cc` — represented-domain, count, capacity, byte, work, and lease preflight;
- `CanonicalHalfedgePairing.h/.cc` — role-specific key generation, deterministic sort/group, pair validation, edge construction, and representatives;
- `CanonicalVertexFans.h/.cc` — exact incidence grouping, transition traversal, canonical fan records, and producer checks;
- `CanonicalFeatureGroups.h/.cc` — source-facet/shell group construction and total maps;
- `CanonicalGeometryAttachments.h/.cc` — committed-basis-aware bounds and non-authoritative nominal attachments;
- `CanonicalHalfedgeCodec.h/.cc` — canonical owner-free semantic encoding/decoding and digest calculation;
- `CanonicalHalfedgeVerifier.h/.cc` — independent reconstruction and mutation rejection;
- `CanonicalHalfedgeQueries.h` — immutable downstream views with documented complexity; and
- `CanonicalHalfedgeReuseAudit.md` — implementation-time record of each relevant legacy utility, reuse/extraction decision, incompatibility, and tests.

Extend existing bounded-subsystem registries:

- `ContractVersions.h`;
- Component 01 strong-ID, stage, checkpoint, error-subcode, resource-kind, diagnostic, replay, and transaction registries;
- Component 03 formula registry only for genuinely new aggregate bounds; and
- strict CMake target and explicit-instantiation lists.

Add tests under `tests/mesh_boolean_bounded/`:

- `TestCanonicalHalfedgeUnit.cc`;
- `TestCanonicalHalfedgePairing.cc`;
- `TestCanonicalHalfedgeVertexFans.cc`;
- `TestCanonicalHalfedgeGroups.cc`;
- `TestCanonicalHalfedgeGeometryBasis.cc`;
- `TestCanonicalHalfedgeGeometryBounds.cc`;
- `TestCanonicalHalfedgeOwnerSeparation.cc`;
- `TestCanonicalHalfedgeCanonicalization.cc`;
- `TestCanonicalHalfedgeAlternativeTriangulation.cc`;
- `TestCanonicalHalfedgeBroadPhaseAdapter.cc`;
- `TestCanonicalHalfedgeCodec.cc`;
- `TestCanonicalHalfedgeMutation.cc`;
- `TestCanonicalHalfedgeProperties.cc`;
- `TestCanonicalHalfedgeAdversarial.cc`;
- `TestCanonicalHalfedgeResourcesCancellation.cc`;
- `TestCanonicalHalfedgeStructuralPerformance.cc`;
- `CanonicalHalfedgeFixtures.h/.cc`;
- `CanonicalHalfedgeExactOracle.h/.cc`; and
- `GoldenCanonicalHalfedgeV1.h`.

All production and normative-test translation units use the strict C++17 bounded-Boolean target. No network discovery or optional test package is permitted.

Keep proposal types, mutable sort/group tables, fan marks, builders, diagnostics, verifier scratch, and mutators out of installed/public headers.

## 5. Versions, stages, checkpoints, and failures

### 5.1 V1 versions

Allocate nonzero versions for:

- provider and policy;
- represented-vertex-domain policy;
- runtime-owner/semantic-separation policy;
- geometry-basis forwarding policy;
- internal-diagonal semantic policy;
- vertex, triangle, halfedge, edge, fan, facet-group, shell-group, map, geometry-attachment, evidence, operand, and wrapper schemas;
- triangle rotation;
- edge pairing keys;
- canonical directed representative;
- fan transition/orientation;
- canonical ordering;
- topology, geometry, replay/presentation, and complete semantic digest layouts;
- codec and verifier; and
- any new Component 03 aggregate-bound formula.

Zero, unknown required versions, nonzero reserved bits, incompatible predecessor versions, and incompatible formulas are typed failures.

### 5.2 Stable checkpoints

Use these stable checkpoints in order:

1. context capability and strict-environment validation;
2. operand owner/version/digest validation;
3. represented-domain, count, and representability preflight;
4. work and persistent-resource reservation;
5. vertex proposal construction;
6. triangle canonical-rotation proposal construction;
7. fixed triangle-halfedge cycle construction;
8. role-specific edge pairing-key generation;
9. deterministic edge proposal sorting and grouping;
10. source-edge pair validation;
11. internal-diagonal pair validation;
12. canonical edge ordering, ID assignment, and reciprocal closure;
13. vertex-incidence grouping;
14. canonical vertex-fan traversal;
15. source-facet group assembly;
16. shell group and total map assembly;
17. committed-basis-aware geometry attachment construction;
18. producer structural verification;
19. owner-free canonical encoding and digest construction;
20. independent verification;
21. replay and resource reconciliation; and
22. final cancellation check and transaction commit.

Do not renumber released checkpoints. Add future provider checkpoints under a new version or reserved range.

### 5.3 Required subcodes

Reserve stable subcodes for every distinction in Component 05 Section 5, including:

- owner leakage into semantic material;
- represented-domain mismatch and isolated-vertex leakage;
- geometry-basis substitution or mismatch;
- internal-diagonal semantic contamination; and
- candidate-visibility/source-ownership conflation.

Every failure records stable stage/checkpoint, operand, least canonical semantic feature key, expected and observed contract, versions/digests, bounded evidence, resource state, and replay reference.

Raw runtime owner values are not part of deterministic canonical fields or the primary arbitration key.

## 6. Public entrypoint and capability views

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

The exact naming may follow Component 01 conventions.

The entrypoint must:

- validate both operands before publication;
- use private operand subtransactions inside one stage transaction;
- support either or both operands empty;
- join all work before rollback;
- choose the same primary failure under every schedule;
- publish neither operand when either fails; and
- publish only after both independent verifiers accept.

Define narrow owner-checked read-only predecessor views. Never index predecessor private arrays directly.

Required Component 02 view capabilities:

- represented source-vertex enumeration;
- source directed-edge and reciprocal lookup;
- source undirected-edge lookup;
- facet/ring boundary;
- shell membership, nesting, orientation, and occupied side;
- geometry-basis disposition and per-vertex realized-point reference;
- source canonical keys and semantic digests; and
- reversible caller/source-position maps.

Required Component 04 view capabilities:

- source triangle enumeration and oriented vertex cycle;
- local edge-use tagged provenance;
- source boundary and internal diagonal records;
- facet groups and shell membership;
- support/projection/orientation and geometry-basis references;
- conservative triangle/facet bounds;
- semantic/exact triangulation digests; and
- verifier disposition.

Required Component 03 view capabilities:

- bounded source point and committed realized point lookup;
- conservative segment, triangle, and union bounds;
- finite and containment tests;
- exponent-safe optional nominal operations;
- formula/version validation; and
- precision-lineage comparison.

Published Component 05 views never allocate or mutate hidden state and never expose implementation pointers as identity.

## 7. Exact entity and artifact schemas

### 7.1 Strong ID domains

Add distinct domains for:

- `manifold_vertex_id`;
- `manifold_triangle_id`;
- `manifold_halfedge_id`;
- `manifold_edge_id`;
- `vertex_fan_id`;
- `source_facet_group_id`;
- `source_shell_group_id`;
- geometry attachment IDs; and
- verifier-evidence IDs where generic evidence IDs are insufficient.

Do not alias them to `I`, `size_t`, or predecessor IDs.

A runtime checked handle conceptually contains:

```text
(owner_validation_token, strong_domain, canonical_ordinal)
```

Its canonical semantic encoding contains:

```text
(strong_domain_version, operand_role_if_applicable, canonical_ordinal)
```

The owner token is never encoded semantically.

### 7.2 Artifact header

Separate the header into:

1. **runtime validation header**, excluded from canonical semantics:
   - owner token/reference;
   - invocation lifetime anchor;
   - private publication state.

2. **canonical semantic header**:
   - operand role;
   - Component 05 versions;
   - required Component 01-04 semantic versions/formulas;
   - predecessor semantic artifact IDs and digests, excluding owner anchors;
   - represented-domain and entity counts;
   - canonical table ranges and checked byte lengths;
   - geometry-basis disposition summary and lineage digests;
   - verifier disposition/version;
   - resource/statistics schema references;
   - source-semantic, exact-topology, geometry-attachment, replay/presentation, and complete semantic digests;
   - zero reserved fields.

Canonical codec and digests encode only the second category plus canonical tables.

### 7.3 Vertex record and key

Store:

- strong ID;
- complete semantic key;
- operand role;
- source vertex ID;
- V1 occurrence discriminator zero;
- shell and shell-group ID;
- exact nominal coordinate bits;
- committed geometry-basis disposition and point/reference;
- Component 03 bounded point and precision lineage;
- caller/source-position provenance;
- canonical outgoing halfedge;
- fan ID and range;
- map references;
- optional non-authoritative local scale; and
- zero reserved fields.

V1 semantic key:

```text
(operand_role,
 source_vertex_id,
 occurrence_discriminator = 0,
 vertex_schema_version)
```

No owner token, coordinate, caller ordinal, array position, bound, or pointer enters the key.

### 7.4 Triangle record and canonical rotation

Store:

- strong ID and semantic key;
- source triangle ID;
- source facet/ring/shell;
- three Component 05 vertices in canonical orientation-preserving rotation;
- corresponding source vertices;
- exactly three triangle-major halfedges;
- inherited support, projection, orientation, geometry-basis, and bound references;
- exact triangulation and caller provenance;
- facet/shell groups; and
- zero reserved fields.

For Component 04 cycle `(v0,v1,v2)` with edge uses `(e0,e1,e2)`, compare only:

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

Choose the least complete key. Never consider reversed rotations.

Triangle semantic key:

```text
(operand_role,
 source_triangle_id,
 chosen_orientation_preserving_rotation_key,
 triangle_schema_version)
```

### 7.5 Halfedge layout

For canonical triangle ordinal `t`:

```text
h(t,0) = 3*t + 0
h(t,1) = 3*t + 1
h(t,2) = 3*t + 2

next(h(t,s))     = h(t,(s+1)%3)
previous(h(t,s)) = h(t,(s+2)%3)
```

Use checked arithmetic before every conversion.

Each halfedge stores or derives:

- strong ID, triangle, and local slot;
- origin and destination Component 05/source vertex IDs;
- next and previous;
- reciprocal pair;
- undirected edge;
- closed edge class and role-specific predecessor payload;
- source facet/shell/groups;
- geometry-basis-aware segment attachment; and
- complete provenance.

### 7.6 Closed edge payloads

Use a tagged representation:

```cpp
enum class canonical_edge_class : std::uint8_t {
    source_edge = 1,
    facet_internal_diagonal = 2
};
```

`source_edge` payload:

- Component 02 source undirected edge;
- its two directed uses;
- incident source facets;
- fixed `source_feature_owner = true`.

`facet_internal_diagonal` payload:

- Component 04 source facet;
- exact facet-local diagonal;
- fixed `source_feature_owner = false`;
- fixed `symbolic_contact_owner = false`;
- fixed `classification_barrier_inside_source_facet = false`;
- fixed `retained_surface_feature = false`.

Do not store `broad_phase_eligible`. Component 06 evaluates its versioned candidate-domain policy from `edge_class`.

### 7.7 Pairing keys

Source edge:

```text
(operand_role,
 source_edge_tag,
 source_undirected_edge_id,
 edge_pair_policy_version)
```

Internal diagonal:

```text
(operand_role,
 internal_diagonal_tag,
 source_facet_id,
 facet_local_diagonal_id,
 edge_pair_policy_version)
```

Each proposal also carries complete directed endpoints, triangle key, local slot, predecessor edge use, facet, and shell.

Sort by pairing key followed by a complete directed-use tie key. Every maximal run must contain exactly two uses.

### 7.8 Edge record and representative

Store:

- strong ID and complete semantic key;
- class and role payload;
- canonical endpoint pair ordered by full vertex key;
- exactly two halfedges and incident triangles;
- both source-facet uses;
- geometry-basis-aware conservative segment bound;
- canonical directed representative; and
- evidence.

Require exact reversed endpoints. Choose the representative directed from lower vertex key to higher vertex key. A self-edge is invalid.

### 7.9 Fan record

For outgoing `h` at vertex `v`:

```text
fan_next(h)     = pair(previous(h))
fan_previous(h) = next(pair(h))
```

Group outgoing halfedges by exact vertex ID. Choose the least full halfedge key as start. Traverse `fan_next`.

Require:

- every transition exists and remains at `v`;
- mutual inverse transitions;
- no repeated incidence before closure;
- closure after exactly the grouped incidence count;
- complete coverage;
- no second cycle; and
- no coordinate-coincident distinct vertex in the fan.

Publish one canonical fan sequence per represented vertex.

### 7.10 Facet group

For each Component 02 source facet store:

- facet/ring/shell/operand;
- canonical boundary ring and directed source-edge uses;
- canonical member triangles and internal diagonals;
- inherited support, projection, orientation, geometry basis, and evidence;
- caller provenance;
- source-semantic and exact-triangulation digest references;
- conservative group bound;
- owner-free semantic key/digest; and
- evidence.

Member lists sort by complete key. Distinct coplanar facets remain distinct.

### 7.11 Shell group

For each Component 02 shell store:

- shell/operand;
- represented member vertices, source edges, facets, triangles, and Component 05 edges or reconstructible ranges;
- inherited orientation, nesting, parity, and occupied side;
- coherent geometry-basis lineage;
- conservative shell bound;
- owner-free semantic key/digest; and
- evidence.

Do not recompute shell semantics.

### 7.12 Maps

Publish sorted immutable maps for:

- represented source vertex to one V1 Component 05 vertex and inverse;
- source triangle to one Component 05 triangle and inverse;
- source directed edge use to one boundary halfedge;
- source undirected edge to one source-edge record and two halfedges;
- facet-local diagonal to one internal edge and two halfedges;
- triangle/local slot to halfedge and inverse;
- source facet to facet group, triangles, diagonals, and boundary;
- source shell to shell group and members;
- caller/source positions to semantic provenance where provided; and
- canonical-order permutations if arrays are not physically canonical.

There is no map entry for isolated non-semantic source-snapshot vertices.

### 7.13 Geometry attachments

Required per vertex:

- exact nominal bits;
- input precision lineage;
- geometry-basis disposition;
- immutable committed realized-point/reference;
- proof/reference that the committed point lies inside the source enclosure; and
- Component 03 bounded point.

Required aggregate bounds:

- segment per edge;
- triangle per triangle;
- union per facet group;
- union per shell.

All bounds include inherited uncertainty, committed basis, bound-construction rounding, formula/version, and precision lineage. They must be finite and containment-verified.

Optional nominal data is explicitly non-authoritative and absent unless required by a selected downstream acceleration policy.

## 8. Count, capacity, and resource preflight

Compute:

```text
V   = represented source vertex count
F   = source triangle count
H   = 3F
B   = source-boundary triangle edge-use count
D   = internal-diagonal triangle edge-use count
E_s = source undirected edge count
E_d = facet-local diagonal count
E   = E_s + E_d
```

Require:

```text
H = B + D
B = 2E_s
D = 2E_d
H = 2E
sum(outgoing incidence counts) = H
represented_vertex_domain =
    distinct(source vertices referenced by all Component 04 triangles)
```

For an empty operand require all represented entity counts zero, even if the Component 01 snapshot contains isolated finite vertices.

Before allocation prove fit in:

- every strong-ID ordinal domain;
- checked internal storage index;
- public `I` where later required;
- host `size_t`;
- codec count and length fields;
- configured entity, byte, work, candidate-adapter, and diagnostic limits; and
- temporary and independent-verifier workspace budgets.

Reserve persistent and temporary resources transactionally. Charge sorting comparisons and fan transitions deterministically.

## 9. Predecessor validation

Before construction, independently validate:

1. context, strict environment, runtime owner, operand, policy, and versions;
2. Component 02/04 artifact IDs, semantic digests, exact-triangulation digest, and verifier dispositions;
3. represented source-vertex domain equality;
4. no isolated snapshot vertex appears in the handoff;
5. every triangle has three distinct represented vertices and one definite inherited orientation;
6. every triangle has exactly three local edge uses aligned with its oriented cycle;
7. every edge use has exactly one supported class;
8. every source-boundary use matches Component 02 source directed/undirected edge, endpoints, facet, shell, and reciprocal use;
9. every source edge has exactly two opposite Component 04 boundary uses;
10. every internal diagonal has exactly two opposite same-facet uses and no source-edge identity;
11. facet groups exactly partition triangles and diagonals;
12. shell groups exactly cover represented features;
13. every represented point, support, projection, and triangle references the same committed geometry basis;
14. a `constructed_coherent_realization` is complete and never replaced by nominal geometry;
15. every inherited bound/reference is finite, owner-correct at runtime, formula-valid, and semantic-lineage compatible; and
16. raw owner tokens are absent from predecessor semantic keys and encoded digests where predecessor contracts require exclusion.

Do not repair a contradiction.

## 10. Deterministic serial construction

### 10.1 Build represented vertices

Derive the represented source-vertex set from Component 04 triangle cycles. Compare it exactly with Component 02/04 handoff maps.

Build one proposal per represented source vertex using the owner-free key from Section 7.3. Sort complete keys if predecessor order is not already proven identical. Reject duplicates. Assign dense IDs only after validation.

Attach nominal bits, geometry-basis reference, precision lineage, shell, caller provenance, and unresolved fan fields.

### 10.2 Build triangles and fixed cycles

Validate each Component 04 triangle and choose the least orientation-preserving rotation.

Sort triangle proposals by complete semantic key, reject duplicates, assign dense IDs, then allocate exactly three triangle-major halfedges.

Fill origin, triangle, local slot, next, previous, role payload, source provenance, support/orientation/geometry-basis reference, and group references. Pair and edge fields remain private unresolved handles.

### 10.3 Generate and group pairing proposals

Emit one proposal per halfedge.

Sort by:

```text
(pairing_key,
 origin_vertex_key,
 destination_vertex_key,
 triangle_key,
 local_slot,
 predecessor_edge_use_key)
```

Use full comparisons. Partition maximal equal pairing-key runs. Every run must contain exactly two uses.

### 10.4 Validate source-edge runs

Require:

- same source undirected edge;
- exact two Component 02 directed uses;
- reversed source and Component 05 endpoints;
- correct incident facets;
- common operand and shell;
- source-edge class on both uses; and
- no third use.

### 10.5 Validate internal-diagonal runs

Require:

- same source facet and exact Component 04 diagonal;
- distinct incident triangles;
- reversed endpoints;
- common operand, shell, and facet group;
- no source directed or undirected edge ID;
- fixed bookkeeping/source-ownership properties; and
- no third use.

Do not assign candidate visibility.

### 10.6 Assign edges, close pairs, and choose representatives

Build complete edge proposals from verified runs. Sort by semantic edge key, reject duplicates, assign dense edge IDs, resolve both halfedges' edge and pair fields, and verify reciprocity immediately.

Choose low-vertex-key to high-vertex-key representative.

### 10.7 Build vertex fans

Group outgoing halfedges by exact vertex ID. Sort each group by full halfedge key only to select canonical start and diagnostics.

Traverse topology with `pair(previous(h))`; do not angle-sort geometry.

Publish canonical fan sequences only after closure, inverse, and coverage checks pass.

### 10.8 Build facet and shell groups

Reconstruct memberships from final entity provenance and compare record-by-record with Components 02 and 04.

Sort member lists by complete semantic key. Preserve support, projection, orientation, nesting, occupied side, semantic digests, and geometry-basis lineage.

### 10.9 Build bounds

For each represented vertex validate the committed basis reference and bounded point.

Construct each edge bound once from its endpoint enclosures and committed points through Component 03.

Construct or validate each triangle bound through the compatible frozen formula.

Build facet and shell union bounds in a frozen deterministic reduction order. Verify every child contained.

No fallback to nominal geometry is permitted.

### 10.10 Empty operand

Publish a canonical empty operand with:

- valid runtime owner binding outside canonical bytes;
- operand role and predecessor semantic links;
- zero represented entity/table counts;
- empty canonical ranges;
- geometry-basis disposition compatible with empty semantics;
- verified disposition;
- resource statistics; and
- deterministic owner-free semantic digests.

## 11. Producer structural verification

Before encoding, reconstruct from private records.

Verify entities and cycles:

- dense IDs and complete keys;
- represented vertex and source triangle bijections;
- exact three-halfedge triangle cycles;
- next/previous inverse closure;
- endpoint alignment;
- local role/provenance alignment; and
- orientation/support/geometry-basis lineage.

Verify pairing:

- one edge per halfedge;
- exactly two uses per edge;
- total symmetric non-self pairs;
- exact reversed endpoints;
- role-specific predecessor agreement;
- no source/internal class mixing;
- canonical representative policy; and
- no candidate/source-ownership conflation.

Verify fans:

- regroup outgoing incidences;
- independently rerun transitions;
- require closure, inverse, and complete coverage;
- compare stored sequences; and
- prevent cross-identity linkage.

Verify groups and maps:

- reconstruct facet and shell membership;
- compare boundary, support, orientation, nesting, occupied side, and geometry basis;
- verify every forward and reverse map; and
- prove isolated snapshot vertices are absent.

Verify geometry:

- nominal bits unchanged;
- committed basis unchanged;
- no constructed-realization fallback;
- finite/formula-valid bounds;
- child containment; and
- precision-lineage consistency.

Verify canonical eligibility:

- strictly increasing semantic tables or exact permutations;
- zero reserved fields;
- no raw owner token in semantic fields or encoding inputs;
- correct resource statistics;
- no private handle or active lease; and
- no scheduling-dependent state.

## 12. Canonical ordering, digests, codec, and owner separation

### 12.1 Physical order

Prefer physical order:

- vertices by vertex semantic key;
- triangles by triangle semantic key;
- halfedges by triangle ID and local slot;
- edges by role-specific semantic key;
- fans by vertex key with canonical topological sequence;
- facet and shell groups by predecessor semantic identity;
- maps by source/domain key; and
- attachments/evidence by semantic feature key and kind.

No unordered iteration is serialized.

### 12.2 Digests

Maintain:

1. `source_semantic_digest` — forwarded source vertex/edge/facet/shell semantics, excluding internal triangulation choice;
2. `exact_topology_digest` — exact triangles, rotations, halfedges, pairs, internal diagonals, fans, groups, and maps;
3. `geometry_attachment_digest` — nominal bits, committed geometry basis, bounded points, bounds, precision lineage, formulas, and optional nominal data;
4. `replay_presentation_digest` — stable caller presentation/provenance fields defined by Component 01, excluding runtime owner anchor; and
5. `complete_semantic_artifact_digest` — canonical semantic header and all canonical sections.

Runtime owner values are excluded from all five.

### 12.3 Codec

Encode field-by-field with Component 01 canonical primitives:

- explicit widths and endianness;
- enum tags;
- lengths and ranges;
- option discriminants;
- floating bit patterns;
- stable semantic strong-ID encodings;
- geometry-basis disposition and lineage;
- formula and schema versions; and
- zero reserved fields.

Never encode native structs, pointers, padding, `size_t`, container capacity, hash buckets, runtime owner tokens, or task data.

Decoder:

1. validates counts and lengths before allocation;
2. rejects unknown required tags/versions and nonzero reserved fields;
3. rejects owner-bearing semantic material;
4. reconstructs runtime owner binding from the current verified context rather than serialized bytes;
5. validates predecessor semantic links;
6. runs the independent verifier; and
7. publishes only after success.

Freeze golden V1 bytes for:

- empty;
- tetrahedron;
- triangulated box;
- concave polygon-derived facet;
- disconnected shells;
- cavity/island;
- coordinate-identical distinct identities;
- high-valence;
- isolated snapshot vertex exclusion; and
- constructed coherent realization.

Run the same semantic fixtures under multiple owner anchors and require identical golden bytes.

## 13. Independent verifier

### 13.1 Independence boundary

The verifier receives immutable Component 02/03/04 artifacts and the proposed Component 05 artifact.

It must not call producer helpers for:

- triangle rotation;
- pairing-key grouping;
- edge ID assignment;
- pair closure;
- fan construction;
- group assembly;
- map construction;
- canonical sorting; or
- digest traversal.

It may share:

- closed schema/type definitions;
- strong-ID domain declarations;
- canonical primitive encoding;
- low-level checked arithmetic; and
- Component 03 containment services.

If a low-level exact-index helper is shared after the reuse audit, producer and verifier must still have materially different high-level grouping and traversal control flow, and mutation tests must prove independence.

### 13.2 Workflow

Independently:

1. validate runtime owner binding, operand, versions, reserved fields, predecessor semantic links, counts, and ranges;
2. prove raw owner values are absent from semantic keys and canonical bytes;
3. rebuild the represented source-vertex domain from Component 04 triangles;
4. reject isolated snapshot vertex leakage;
5. recompute orientation-preserving triangle rotations;
6. reconstruct triangle local edges from cycles and tagged predecessor provenance;
7. rebuild source-edge and internal-diagonal groups;
8. require exactly two reversed uses per edge;
9. rebuild edge keys, order, IDs, pairs, and representatives;
10. reconstruct outgoing incidences and fan cycles with independently written traversal;
11. rebuild facet and shell groups;
12. rebuild every forward and reverse map;
13. verify nominal bits and committed geometry-basis references;
14. independently verify bounds and precision/formula lineage;
15. recompute semantic keys, arrays/permutations, digests, and canonical bytes;
16. compare stored data field-by-field;
17. verify resource statistics and absence of private handles; and
18. return one verified disposition or deterministic least-key typed rejection.

### 13.3 Required mutations

Reject at least:

- remove, duplicate, or reorder a represented vertex or triangle;
- add an isolated snapshot vertex to the artifact;
- reverse or incorrectly rotate a triangle;
- change local slot, facet, shell, support, orientation, or geometry basis;
- delete/add a halfedge or break cycle closure;
- change edge class or role payload;
- delete/duplicate a use, add a third use, pair one-way/self/same-direction, mix classes, or pair diagonals across facets;
- alter representative;
- split one fan, join distinct fans, open a fan, duplicate incidence, or change stored order;
- change facet/shell membership, occupied side, or group digest;
- omit/duplicate/change a map;
- merge coordinate-identical distinct vertices;
- replace a constructed realization with nominal geometry;
- mutate, shrink, mislabel, or substitute a bound;
- set an internal diagonal as source/symbolic/barrier/output owner;
- store a Component 06 eligibility bit as semantic truth;
- insert owner token into a key, digest, byte stream, or failure arbitration key;
- permute canonical arrays without corresponding maps;
- alter version, reserved field, count, length, digest, or canonical byte; or
- forge producer summary counts while incidence remains corrupt.

Require zero surviving mandatory mutations.

## 14. Immutable downstream query contract

Provide allocation-free owner-checked reads.

Vertex queries:

- source identity, operand, shell, caller provenance;
- nominal bits;
- committed geometry-basis disposition/reference;
- bounded point and precision;
- canonical outgoing representative;
- fan span; and
- maps.

Triangle queries:

- vertices and halfedges;
- source triangle/facet/shell;
- orientation, support, projection, geometry basis, and bound;
- groups; and
- semantic/exact triangulation lineage.

Halfedge/edge queries:

- origin, destination, next, previous, pair, triangle, edge;
- exact edge class;
- canonical endpoints and representative;
- both incident halfedges/triangles/facets;
- source edge or internal diagonal provenance;
- source-feature, symbolic-owner, barrier, and retained-feature properties;
- whether traversal remains within one source facet; and
- segment bound.

Relationship/group queries:

- shared vertex, edge, facet, or shell;
- edge incident to triangle;
- facet triangles, diagonals, and boundary;
- shell membership and occupied side;
- maps, bounds, and digests.

Component 06 determines candidate visibility from edge class plus its frozen candidate-domain policy.

## 15. Alternative legal triangulation semantics

Use Component 04's test-only alternative legal triangulation builder.

For each legal variant:

- run full producer, codec, and independent verifier;
- allow exact triangles, internal diagonals, halfedges, fans, exact-topology digest, and exact bytes to differ;
- require identical or predecessor-defined-equivalent source semantic digest, source vertices/edges/facets/shells, occupied-side semantics, nominal provenance, and geometry-basis lineage;
- require all internal diagonals remain bookkeeping-only;
- require source boundary-edge records and source-feature queries equivalent;
- feed each variant to Component 06 V1, which may emit different bookkeeping candidates;
- require Component 07 reduce bookkeeping relations to the same source-feature semantics; and
- require final Boolean topology unchanged for the covered fixtures.

Do not require byte identity between genuinely different legal triangulations.

## 16. Metamorphic expectation matrix

Classify every metamorphic test before asserting bytes.

### 16.1 Byte-identical presentation metamorphisms

Require byte identity for semantically identical predecessor artifacts under:

- vertex-array permutation with remapped indices;
- facet permutation;
- ring rotation;
- shell record permutation;
- allocation/layout changes;
- full-key hash collisions;
- owner-anchor changes;
- worker counts;
- task partitions; and
- schedule delays/reversal.

This assumes Components 02-04 already produce identical semantic predecessor bytes as required.

### 16.2 Remapped semantic-equivalence transforms

For axis permutation, exact translation, power-of-two scale, sign flip, and corrected whole-shell orientation transformations, use the predecessor-defined remapping/equivalence contract.

Require byte identity only if predecessor semantic bytes are explicitly identical. Otherwise compare:

- topology;
- exact incidence;
- source-feature ownership;
- remapped geometry basis and bounds;
- shell semantics;
- precision lineage; and
- downstream Boolean semantics.

### 16.3 Alternative triangulations

Use Section 15.

## 17. Transactions, resources, cancellation, and concurrency

### 17.1 Transaction

Use one Component 05 stage transaction with two private operand subtransactions.

Persistent leases transfer only with the final wrapper. If either operand fails:

- join all work;
- select the canonical primary failure;
- discard both candidates;
- release all leases;
- leave predecessors unchanged; and
- publish nothing.

### 17.2 Cancellation points

Poll at stable boundaries:

- operand start;
- predecessor validation chunks;
- proposal generation chunks;
- before/after deterministic sorts;
- edge-run batches;
- pair-resolution boundary;
- incidence and fan batches;
- facet/shell group batches;
- geometry attachment batches;
- producer verification batches;
- codec section boundaries;
- independent-verifier batches; and
- immediately before commit.

Do not poll halfway through local record fill, pair closure, or fan transition. Finish private local work and discard at the next checkpoint.

### 17.3 Component 17 boundary

Retain an executable serial semantic provider.

Parallel execution may:

- generate immutable proposals in private canonical ranges;
- validate private proposals;
- sort private runs and merge by full semantic key;
- compute private geometry attachments;
- verify disjoint canonical partitions; and
- return private failures/evidence.

Parallel tasks may not:

- assign final semantic IDs;
- mutate published/shared records;
- close cross-task pairs before canonical merge;
- choose fan order from task completion;
- leak owner anchors into semantics;
- depend on an unqualified floating environment; or
- expose completion order.

Final global key order, ID assignment, pair closure, fan reconstruction, encoding, failure arbitration, and commit follow serial semantics.

### 17.4 Fault injection

Inject failure at:

- reservation;
- allocation;
- proposal generation;
- sort;
- group validation;
- pair closure;
- fan traversal;
- geometry-basis lookup;
- bound construction;
- codec;
- verifier;
- worker creation/join;
- cancellation; and
- final commit.

Require no leaked resource, active worker, published private ID, escaped handle, predecessor mutation, or nondeterministic failure.

## 18. Diagnostics, replay, and observability

Every failure report includes stable:

- stage/checkpoint;
- operand;
- source and Component 05 semantic feature IDs/keys;
- expected and observed role, endpoints, counts, transition, membership, map, geometry basis, or bound;
- predecessor semantic artifact/digest references;
- formula/precision evidence;
- resource use/limit;
- replay reference; and
- primary arbitration key.

Raw runtime owner tokens may appear only in optional non-canonical debug fields and never affect deterministic summary, arbitration, replay, or bytes.

Expose structural statistics for:

- represented vertices;
- isolated snapshot vertices excluded;
- triangles;
- halfedges;
- edge classes;
- fan valence distribution;
- facet and shell groups;
- sort comparisons;
- fan transitions;
- geometry-basis dispositions;
- bound operations;
- verifier work;
- bytes; and
- resources.

Replay reconstructs exact source bits, options, predecessor semantic artifacts, versions, and geometry-basis records, then reproduces the same Component 05 semantic digest or primary failure under a fresh owner anchor.

## 19. Test and validation plan

### 19.1 Unit and known-answer tests

Cover empty, tetrahedral, box, polygon-derived, high-valence, disconnected, cavity/island, duplicate-coordinate, isolated-snapshot-vertex, and constructed-realization fixtures.

Independently reconstruct cycles, endpoints, represented domain, pairings, fans, groups, maps, and basis linkage.

### 19.2 Pairing and provenance tests

Test ordinary source edges, multiple diagonals in one facet, adjacent coplanar facets, coordinate-identical distinct IDs, repeated coordinates across shells, legal diagonal alternatives, signed zero, subnormals, and capacity-edge IDs.

### 19.3 Geometry-basis tests

Run identical topology with:

- nominal embedded points;
- a valid constructed coherent realization;
- a mutated realization;
- a stale realization reference;
- a nominal fallback mutation;
- incompatible support/projection formula;
- extreme finite exponents; and
- conservative-bound shrink mutations.

Require precise stable failures.

### 19.4 Owner separation tests

Construct equivalent semantic predecessors under distinct owner anchors. Require identical Component 05 semantic keys, IDs, bytes, digests, statistics covered by semantic policy, and failures.

Inject owner-bearing keys and encoding.

### 19.5 Broad-phase adapter tests

Enumerate each edge once. Confirm Component 06 V1 sees internal diagonals as bookkeeping candidates while source ownership remains false. Confirm both incident halfedges remain available.

### 19.6 Canonicalization tests

Use Section 16's expectation matrix. Tests must state whether they assert byte identity, remapped semantic equivalence, or source-semantic equivalence with different exact topology.

### 19.7 Codec tests

Test golden bytes, round-trip, truncation at every byte, overflow, malformed tags/versions/lengths/ranges, duplicate/missing sections, trailing data, nonzero reserved bits, altered scalar bits, digest substitution, geometry-basis substitution, and owner-token leakage.

Decode never publishes before independent verification.

### 19.8 Mutation tests

Implement every Section 13.3 mutation as a targeted mutator. Require zero survivors and stable least-key subcodes.

### 19.9 Exact oracle, properties, fuzzing, and shrinking

Use only the test-only in-tree arbitrary-precision facilities planned by Components 03 and 16.

The oracle computes:

- exact endpoint-identity incidence;
- edge multiplicity and direction;
- triangle cycles;
- vertex links;
- Euler/handshake identities;
- exact represented domain;
- and bound containment for bounded fixture realizations.

Fuzz topology-preserving subdivision, legal triangulation, shell nesting, valence, source presentation, duplicate coordinates, coherent-realization displacement, ULP/exponent ranges, owner anchors, capacity, resources, and structural mutations.

Store every fixed minimized regression permanently.

### 19.10 Resource, cancellation, concurrency, and replay tests

For each resource class test limit-minus-one, exact, and plus-one. Cancel at every checkpoint. Force delayed/reversed tasks and worker counts 1, 2, and configured maximum.

Require identical accepted semantic artifacts and primary failures, no partial publication, no leaked leases, and replay success under a fresh owner anchor.

### 19.11 Structural performance tests

Use counters to prove:

- `O(V + F + H log H)` or better build;
- `O(H)` total fan traversal;
- linear persistent storage;
- bounded verifier duplication;
- no all-vertex, all-triangle, or all-edge pair scan;
- deterministic comparison/work counts; and
- typed work/resource exit for adversarial inputs.

## 20. Integration boundaries

### 20.1 Components 02 and 04

Consume their exact source identities, semantic digests, exact triangulation, shell semantics, and committed coherent geometry basis. Never alter or reinterpret them.

### 20.2 Component 03

Use Component 03 exclusively for required bounds and qualified optional nominal calculations. Component 05 performs no new authoritative relation predicate.

### 20.3 Component 06

Provide:

- every canonical edge exactly once through its representative;
- exact edge class;
- both incident halfedges/triangles/facets;
- source-facet/shell provenance;
- committed-basis-aware conservative bounds; and
- immutable query complexity guarantees.

Component 06 owns candidate-domain and spatial-index policy.

### 20.4 Components 07-10

Preserve enough source lineage that triangle-local and internal-diagonal discoveries reduce to original source features. Internal diagonals cannot own public relations, events, symbolic decisions, or winding barriers.

### 20.5 Components 11-15

Component 05 is source-side topology only. Do not reuse its records as mutable output topology. Component 15 may inspect source lineage but independently reconstructs final public topology.

### 20.6 Legacy APIs

Preserve legacy public behavior. Any extracted helper requires the reuse-audit proof and direct tests. Do not broaden a legacy routine's promise to satisfy Component 05 implicitly.

## 21. Implementation sequence and handoff gates

Implement in this order, leaving the branch buildable after every gate:

1. Record the legacy reuse audit and decisions.
2. Add Component 05 versions, closed enums, strong-ID domains, checkpoints, subcodes, and uniqueness tests.
3. Add runtime-owner versus semantic-identity wrappers and owner-leak tests.
4. Add immutable schemas and checked views.
5. Add represented-domain/count/capacity/resource preflight and isolated-vertex exclusion.
6. Add predecessor capability views and validation.
7. Add vertex proposals with nominal and committed-basis lineage.
8. Add orientation-preserving triangle canonicalization.
9. Add fixed triangle-major halfedge cycles.
10. Add role-specific pairing keys without owner tokens.
11. Add source-edge grouping and validation.
12. Add internal-diagonal grouping and fixed bookkeeping semantics.
13. Add canonical edge order, reciprocal closure, and representatives.
14. Add producer cycle/pair reconstruction and mutations.
15. Add incidence grouping and exact fan transitions.
16. Add canonical fan records and fan mutations.
17. Add facet/shell groups and total maps.
18. Add committed-basis-aware geometry attachments.
19. Add optional nominal attachments with explicit non-authoritative status.
20. Add complete producer verification.
21. Add owner-free canonical ordering, codec, digests, decoder, and golden bytes.
22. Implement the independent verifier with separate reconstruction code.
23. Add complete mutation suite and prove zero survivors.
24. Add exact-oracle/property generators, fuzzing, shrinking, and regressions.
25. Add owner-anchor, isolated-vertex, constructed-realization, duplicate-coordinate, high-valence, shell, and extreme-bound suites.
26. Add Component 04 alternative-triangulation equivalence.
27. Add Component 06 V1 bookkeeping-candidate adapter.
28. Add structural counters and asymptotic gates.
29. Add Component 17 private-task/canonical-merge hooks while retaining serial equivalence.
30. Run Component 01-04 regressions under strict builds.
31. Run debug/release, sanitizers, codec, owner/version, overflow, resource, cancellation, fault injection, replay, worker, schedule, LTO/IPO, and dependency-off matrices.
32. Mark Component 05 complete in `tracker.md` only after every gate in Section 22 is represented and reviewed.

## 22. Definition of done

Component 05 is complete only when all of the following are true:

- only Component 05 scope is implemented;
- the legacy reuse audit is committed and every accepted extraction is narrow, tested, deterministic, identity-only, and source-compatible;
- no legacy Boolean, repair, orientation, hole, remeshing, or generic triangulation provider is called;
- Component 02/04 owner, version, digest, represented-domain, incidence, triangle, facet, shell, orientation, provenance, and geometry-basis contracts are defensively verified;
- `V` is exactly the triangle-referenced represented vertex domain and isolated snapshot vertices are excluded;
- checked identities establish `H=3F`, `B=2E_s`, `D=2E_d`, `H=2E`, and total outgoing incidence `H`;
- every represented source vertex maps exactly once without coordinate welding;
- every source triangle maps exactly once through an orientation-preserving canonical rotation;
- every triangle owns exactly three triangle-major halfedges in a closed oriented cycle;
- every halfedge belongs to exactly one triangle and one role-tagged edge;
- every source edge pairs from exact Component 02/04 identity with two opposite uses;
- every internal diagonal pairs only within one source facet and remains bookkeeping-only;
- no generic eligibility field conflates candidate visibility with source ownership;
- pairing is total, symmetric, non-self, endpoint-reversed, and canonically represented;
- every represented vertex has one independently verified closed fan;
- coordinate-identical distinct vertices never enter each other's maps, edges, or fans;
- facet groups preserve boundary, support, projection, orientation, member, semantic, exact-triangulation, and geometry-basis lineage without merging coplanar facets;
- shell groups preserve exact membership, nesting, orientation, and occupied side without recomputation;
- nominal bits and committed coherent geometry basis are both preserved;
- a constructed coherent realization is never silently replaced by nominal geometry;
- all maps are total, canonical, owner-checked at runtime, and independently reconstructed;
- all bounds are finite, closed, formula-versioned, precision-linked, committed-basis-aware, and independently containment-checked;
- runtime owner tokens are absent from semantic keys, IDs, bytes, digests, golden records, replay, and deterministic failure arbitration;
- semantically identical invocations under different owner anchors produce identical Component 05 semantic artifacts;
- optional nominal geometry is exponent-safe, explicit, non-authoritative, and deterministic;
- source-semantic identity remains invariant under legal retriangulation while exact topology records changed layout;
- immutable queries satisfy Component 06 and later stages without coordinate tests, hidden mutation, or policy conflation;
- producer verification reconstructs cycles, pairs, fans, groups, maps, basis linkage, bounds, and canonical order;
- independent verification repeats every obligation without producer high-level helpers or trust in counts/booleans/digests;
- every required structural, provenance, basis, bound, codec, owner, version, and digest mutation is rejected with stable least-key failure;
- all required unit, known-answer, golden, exact-oracle, owner-separation, isolated-vertex, constructed-realization, duplicate-coordinate, high-valence, shell, alternative-triangulation, broad-phase-adapter, metamorphic, fuzz/shrink, adversarial, resource, cancellation, concurrency, codec, replay, and performance suites pass;
- serial V1 remains executable and optimized/parallel providers are semantically and failure identical;
- counters prove `O(V + F + H log H)` or better construction, `O(H)` fan traversal, linear persistent storage, and no accidental all-pairs scans;
- resource, overflow, host-allocation, cancellation, fault-injection, transaction, and replay tests prove zero partial publication and zero leaked resources;
- Component 01-04 regressions pass under strict targets;
- production and normative tests are standard-library-only portable C++17 with no external dependency; and
- `tracker.md` marks Component 05 complete only after this reviewed plan is committed.
