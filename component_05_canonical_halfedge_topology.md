# Component 05: Canonical Halfedge Topology

## Status and normative language

This document specifies a required component of the dependency-free bounded floating-point surface-mesh Boolean engine described by `broad_plan.md`. Production code and normative tests must be portable C++17 and use no external dependency.

The concrete storage layout, allocation strategy, and canonicalization provider may change. The exact indexed-incidence, provenance, committed-geometry-basis, determinism, verification, transaction, resource, and failure contracts in this document are normative.

## 0. Purpose

Component 05 converts each independently verified `source_triangle_complex<T,I>` from Component 04 into one immutable, canonically ordered, oriented halfedge manifold:

```cpp
canonical_halfedge_operand<T,I>
```

The pair of operand artifacts is published as:

```cpp
canonical_source_manifolds<T,I>
```

This is the first pipeline artifact in which all of the following coexist in one exact indexed representation:

- one oriented three-halfedge cycle for every source triangle;
- one reciprocal pair for every source-boundary edge and every facet-local triangulation diagonal;
- one closed cyclic incident fan for every represented source vertex occurrence;
- exact source-facet and source-shell grouping;
- complete forward and reverse provenance;
- immutable, owner-checked downstream queries;
- conservative geometry attachments tied to the predecessor's committed coherent geometry basis; and
- canonical semantic ordering independent of caller presentation, allocation, hashing, runtime owner anchors, and worker scheduling.

Component 05 makes source-manifold incidence explicit. It does not cut faces, evaluate cross-operand relations, construct intersections, classify winding, select Boolean output, repair source topology, retriangulate source facets, spend cleanup tolerance, or infer adjacency from coordinates.

## 1. Input contract

### 1.1 Required inputs

For both operands, the component must accept:

- the immutable Boolean context and controlled services from Component 01;
- one immutable `validated_operand<T,I>` from Component 02;
- the immutable precision and bounded-geometry services from Component 03;
- one immutable, independently verified `source_triangle_complex<T,I>` from Component 04;
- the frozen Component 05 provider, policy, schema, codec, verifier, and formula versions;
- resource, cancellation, transaction, diagnostic, replay, canonical-encoding, digest, and deterministic-execution capabilities; and
- immutable predecessor lifetime references covering every published view.

The ordinary pipeline must validate both operand inputs and publish the two-operand wrapper atomically. A one-operand builder may exist only as a private or test capability.

No later read may return to caller-owned `fv_surface_mesh<T,I>` storage or to mutable predecessor workspaces.

### 1.2 Required predecessor guarantees

Component 05 may rely on Components 02-04 having committed the following, but it must defensively verify every dependency before publication:

- exact canonical source vertex, directed-edge-use, undirected-edge, facet, ring, shell, and triangle domains;
- every accepted source facet completely covered by definitely oriented source triangles;
- exactly one role record for every directed triangle edge;
- exactly one source-boundary triangle use for every source directed-edge use under the V1 no-source-edge-subdivision policy;
- exactly two opposite same-facet uses for every facet-local internal diagonal;
- complete source-facet triangle and diagonal membership;
- complete source shell membership and occupied-side evidence;
- total source-to-triangle and triangle-to-source provenance;
- one committed coherent geometry-basis disposition and immutable geometry-basis reference for every represented source point, facet support, projection, and triangle;
- separate source-semantic and exact-triangulation digests;
- verified artifact dispositions, versions, owner/domain bindings, and conservative bounds; and
- preservation of coordinate-coincident but identity-distinct source vertices.

A contradiction in a committed predecessor artifact is an `internal_invariant_error` attributed to that predecessor. Component 05 must not repair, reinterpret, or silently replace it.

### 1.3 Represented source-vertex domain

The Component 05 vertex domain is the exact represented topology domain handed off by Components 02 and 04, not the raw Component 01 source-snapshot vertex array.

Under V1:

- every source vertex referenced by an accepted source facet maps to exactly one Component 05 vertex occurrence;
- isolated snapshot vertices that Component 02 declared non-semantic have no Component 04 handoff record and must not acquire a Component 05 vertex, shell membership, bound, map entry, or canonical identity; and
- a future policy that intentionally splits one source identity into several source-side occurrences requires a new explicit occurrence policy and schema version.

The component must verify exact equality between the represented vertex domain and the union of triangle vertex identities.

### 1.4 Runtime ownership versus semantic identity

Runtime owner tokens are lifetime- and misuse-detection metadata. They are not semantic mesh data.

Every handle and published artifact must be bound to the current invocation owner and must reject stale, wrong-context, wrong-operand, and wrong-domain access. However, the raw owner token, pointer, allocation anchor, or process-specific owner value must not enter:

- canonical entity keys;
- semantic ordering;
- dense canonical ID assignment;
- canonical artifact bytes;
- semantic, topology, geometry, replay, or complete digests;
- golden test vectors;
- deterministic primary-error arbitration keys; or
- public output.

Canonical encodings of strong IDs contain stable domain, operand role where applicable, schema version, and canonical ordinal or complete semantic key, but not the runtime owner token.

Two semantically identical invocations with different valid owner anchors must produce identical Component 05 semantic bytes, canonical IDs, digests, and deterministic semantic diagnostics. Owner-misuse diagnostics may report stable facts such as `wrong_owner`, expected domain, operand, stage, and offending semantic key; any raw runtime token is optional non-canonical debug data and must not affect failure selection or replay.

### 1.5 Committed coherent geometry basis

Component 05 must preserve the exact geometry basis committed by Components 02 and 04:

- `nominal_embedded`, using the committed nominal source point with inherited enclosure; or
- `constructed_coherent_realization`, using the committed shared realized point and its proof of containment in the source precision envelope.

For every represented vertex, the artifact must retain both:

- exact nominal source coordinate bits and source precision lineage for provenance and replay; and
- the committed geometry-basis point/reference used by Components 02 and 04 for geometric validity and source triangulation.

Component 05 must not silently substitute nominal points for a constructed coherent realization, choose a new realization, recompute a support plane or projection through a different expression, or collapse the distinction between nominal provenance and committed geometry basis.

Conservative edge, triangle, facet, and shell bounds supplied to Component 06 must enclose every realization admitted by the committed basis and inherited uncertainty. Optional nominal acceleration data must be explicitly non-authoritative.

### 1.6 Accepted cases

The component must support:

- either or both operands empty;
- one or more disconnected closed shells;
- cavities, islands, and deeper validated nesting;
- triangular and polygon-derived source facets;
- concave facets represented by several triangles;
- high-valence manifold vertices;
- coordinate-coincident but identity-distinct source vertices;
- adjacent coplanar source facets that remain distinct source features;
- repeated coordinate patterns across shells;
- signed zero, subnormal values, extreme finite exponents, and large translations;
- `float` or `double` with `std::uint32_t` or `std::uint64_t`; and
- legal alternative source-facet triangulations permitted by Component 04.

Empty and non-empty operands remain subject to owner, version, digest, capacity, codec, resource, and verifier checks.

### 1.7 Capacity and lifetime preconditions

Before allocation, the component must prove with Component 01 checked arithmetic that it can represent and account for:

- every represented vertex;
- every source triangle;
- exactly three halfedges per triangle;
- every source undirected edge and every facet-local internal diagonal;
- every vertex incidence and fan element;
- every facet and shell group;
- every forward and reverse map;
- every required geometry attachment and verifier-evidence record;
- every canonical byte, sort key, temporary proposal, and verifier workspace; and
- all abstract work under the caller's configured limits.

No unchecked `size_t` product, prefix sum, or index conversion is permitted. Published storage must outlive every Component 06-10 consumer and may refer only to immutable predecessor artifacts or stage-owned immutable storage.

## 2. Required behavior

### 2.1 Exact entity model

The artifact must contain or support constant-time or bounded deterministic access to:

- internal source-side vertex occurrences;
- oriented source triangles;
- directed halfedges;
- role-tagged undirected edges;
- reciprocal halfedge pairs;
- source-facet groups;
- source-shell groups;
- vertex-incidence and cyclic-fan records;
- complete provenance and canonical-order maps;
- committed-geometry-basis and conservative-bound references; and
- independently reconstructible verification evidence.

Pointer addresses, container iteration order, allocation order, source presentation order, hash order, worker number, task completion order, and runtime owner anchors are never semantic identities.

### 2.2 Vertex construction

Each represented source vertex identity maps to exactly one V1 manifold vertex occurrence.

A vertex record must preserve:

- canonical Component 05 vertex ID and complete owner-free semantic key;
- runtime owner binding in non-semantic validation metadata;
- operand role and source vertex identity;
- V1 occurrence discriminator zero;
- source shell and shell-group identity;
- exact nominal coordinate bits;
- committed geometry-basis point/reference and disposition;
- Component 03 precision and bounded-point lineage;
- caller/source-position provenance;
- canonical outgoing halfedge representative;
- fan range or reconstructible exact adjacency; and
- total source-to-internal and internal-to-source mappings.

Distinct source identities remain distinct even when all nominal coordinates, realized coordinates, and enclosures are bit-identical. No coordinate, distance, Morton, spatial-cell, hash, or tolerance operation may merge them.

### 2.3 Triangle and halfedge cycles

For every Component 04 source triangle, create exactly one oriented triangle record and exactly three directed halfedges.

Each triangle cycle must:

- preserve the predecessor orientation;
- use an orientation-preserving canonical rotation only;
- reference exactly three distinct represented vertex identities;
- provide total `next` and `previous` relations;
- close after exactly three steps;
- contain three distinct halfedges;
- preserve exact local edge-role and provenance alignment;
- retain source facet, shell, operand, source triangle, support, orientation, and geometry-basis references; and
- never reverse, omit, replace, or retriangulate the predecessor triangle.

Destination may be stored or derived as `origin(next(h))`, but endpoint agreement must be independently checkable.

### 2.4 Role-tagged edge identity and reciprocal pairing

Every halfedge belongs to exactly one role-tagged undirected edge and has exactly one reciprocal pair.

The closed V1 edge classes are:

```cpp
enum class canonical_edge_class : std::uint8_t {
    source_edge = 1,
    facet_internal_diagonal = 2
};
```

Pairing keys use authoritative provenance only:

- source-boundary uses pair by exact canonical source undirected-edge identity;
- internal-diagonal uses pair by exact source-facet identity and exact facet-local diagonal identity.

Coordinates, coordinate bits, unordered endpoint coordinates, bounds, distance, tolerance, spatial keys, and hashes alone are prohibited pairing evidence.

For every source edge, require exactly the two opposite Component 02 directed uses, correct facets, reversed source and Component 05 endpoints, common operand and shell, and exact source-edge provenance.

For every internal diagonal, require exactly two opposite uses in distinct triangles of the same source facet, reversed endpoints, exact diagonal identity, common operand/shell/facet group, and no source-edge identity.

Reciprocity is total and symmetric:

```text
pair(h) = r  =>  pair(r) = h
```

One-way pairing, self-pairing, more or fewer than two uses, same-direction uses, endpoint disagreement, cross-facet internal pairing, or cross-operand pairing prevents publication.

### 2.5 Internal-diagonal semantic separation

A facet-local internal diagonal is exact source-triangulation topology, but it is not an original source feature.

The artifact must expose separate, non-conflated properties:

- `edge_class == facet_internal_diagonal`;
- `source_feature_owner == false`;
- `symbolic_contact_owner == false`;
- `classification_barrier_inside_source_facet == false`;
- `retained_surface_feature == false`; and
- `candidate_visibility` is not a stored semantic truth, but is decided by Component 06's versioned candidate-domain policy.

Component 05 must make every internal diagonal queryable. Component 06 V1 may include it as a bookkeeping edge to guarantee triangle-local relation coverage. If included, Components 06 and 07 must retain `bookkeeping_only` status and reduce any discovered interaction to original source-feature semantics or a documented no-public-relation disposition.

No generic `eligible` flag may conflate source ownership, broad-phase inclusion, symbolic ownership, winding barriers, or output retention.

### 2.6 Source-facet groups

All triangles from one source facet must form one exact group preserving:

- the canonical source ring and directed boundary uses;
- canonical member triangles and internal diagonals;
- source facet, ring, shell, operand, and orientation;
- committed support plane, projection frame, geometry-basis disposition, and bounded evidence;
- source semantic and exact triangulation digest references;
- caller provenance;
- conservative group bound; and
- stable owner-free group key and digest.

Distinct source facets remain distinct even when their nominal or realized planes coincide. Component 05 must not merge coplanar facets.

A downstream traversal must be able to determine by identity that crossing an uncut internal diagonal remains within one source-facet semantic region.

### 2.7 Source-shell groups

For every Component 02 shell, publish exact membership sufficient to reconstruct its represented vertices, source edges, facets, triangles, and Component 05 edges.

Preserve, do not recompute:

- shell orientation;
- nesting and parity;
- occupied-side semantics;
- source-shell provenance; and
- coherent geometry-basis and conservative shell-bound lineage.

Every represented feature belongs to exactly one shell. Empty shell groups are invalid unless a future predecessor policy explicitly permits them.

### 2.8 Vertex incidence and cyclic links

For every represented vertex, build or derive its outgoing halfedge incidence and verify one closed manifold fan.

A conforming exact transition for an outgoing halfedge `h` is:

```text
fan_next(h)     = pair(previous(h))
fan_previous(h) = next(pair(h))
```

The component must verify:

- all transitions exist and remain outgoing from the same vertex identity;
- `fan_next` and `fan_previous` are mutual inverses;
- traversal returns to the canonical start;
- no incidence repeats before closure;
- closure occurs after exactly the grouped incidence count;
- every outgoing incidence is visited exactly once;
- no second disjoint cycle shares the vertex identity;
- internal diagonals refine but do not split the source link; and
- a coordinate-coincident distinct vertex never enters the fan.

Publish the least complete halfedge key as canonical representative and either the canonical fan sequence or sufficient immutable exact adjacency to reconstruct it deterministically.

### 2.9 Conservative geometry attachments

Required attachments are:

- one inherited bounded source point and committed geometry-basis reference per represented vertex;
- one closed conservative segment bound per undirected edge;
- one closed conservative triangle bound per triangle;
- one conservative source-facet group bound; and
- one conservative shell bound.

All bounds must be produced or validated through frozen Component 03 formulas, include inherited precision and construction rounding, be finite and representable, and contain every realization admitted by the committed coherent geometry basis.

Optional non-authoritative data may include scaled area vectors, centroids, dominant axes, spatial sort hints, and local scales. It must be exponent-safe, versioned, explicitly available or absent, and prohibited from deciding topology, pairing, semantic ownership, or Component 07 relations.

Component 05 may publish canonical feature traversal order and bounds. Component 06 owns the versioned spatial-index and spatial-ordering policy.

### 2.10 Canonical identity and ordering

Every semantic key must be complete, versioned, owner-free, collision-safe through full-key comparison, and derived from predecessor semantic identity.

Representative forms include:

```text
vertex:
(operand_role, source_vertex_id, occurrence_discriminator, vertex_schema_version)

triangle:
(operand_role, source_triangle_id, orientation_preserving_rotation_key,
 triangle_schema_version)

source edge:
(operand_role, source_edge_tag, source_undirected_edge_id,
 edge_pair_policy_version)

internal diagonal:
(operand_role, internal_diagonal_tag, source_facet_id,
 facet_local_diagonal_id, edge_pair_policy_version)

halfedge:
(edge_semantic_key, incident_triangle_key, directed_endpoint_keys,
 local_slot, predecessor_edge_use_key, halfedge_schema_version)
```

The runtime owner token is validated separately and excluded from these keys.

Published arrays must be in canonical order or accompanied by immutable canonical-order permutations. Temporary hash tables are permitted only with complete-key equality and deterministic sorting before any observable result.

Every undirected edge must expose one canonical directed representative chosen by stable endpoint and provenance keys, never coordinates or memory order. Both face-oriented uses remain available.

### 2.11 Immutable query contract

Downstream code must be able to determine by exact identity alone:

- operand, shell, source facet, source triangle, source vertex, and caller provenance;
- triangle vertices, halfedges, orientation, support, geometry-basis, and bounds;
- halfedge origin, destination, next, previous, pair, triangle, and edge;
- edge class, canonical endpoints, canonical representative, incident halfedges/triangles/facets, and bounds;
- whether traversal across an edge remains in one source facet;
- whether two triangles share a vertex, edge, source facet, or shell;
- all triangles and diagonals of a source facet;
- all represented features of a shell;
- exact forward/reverse provenance maps;
- source-semantic and exact-topology digest lineage; and
- internal-diagonal source-ownership properties without deciding Component 06 candidate policy.

Queries must be immutable, allocation-free for ordinary reads, owner/domain/range checked, deterministic under concurrent reads, and documented as `O(1)`, `O(log n)`, or bounded span traversal. No scheduling-dependent lazy cache is permitted.

### 2.12 Independent verification evidence

The artifact must expose enough primary records for an independent verifier to reconstruct, rather than trust:

- represented vertex and triangle domains;
- orientation-preserving triangle rotation;
- triangle cycles and endpoints;
- edge-role classification;
- exact two-use grouping and reciprocal pairing;
- canonical representatives;
- vertex-fan transitions, closure, and coverage;
- facet and shell membership;
- all forward and reverse maps;
- committed geometry-basis linkage;
- bound containment;
- canonical order;
- semantic/geometry/topology digest inputs; and
- canonical bytes.

Producer booleans, counts, checksums, and cached grouping results are not sufficient.

### 2.13 Transactions, cancellation, resources, and concurrency

Construction occurs in private stage storage. Both operand candidates must pass producer verification, canonical encoding, independent verification, resource reconciliation, and a final cancellation check before one wrapper is committed.

Resource accounting must cover persistent and temporary:

- entity records;
- pairing and sort proposals;
- fan incidence and traversal marks;
- groups and maps;
- geometry attachments;
- canonical bytes and digests;
- diagnostics and replay;
- verifier workspaces; and
- abstract work units.

All counts and bytes use checked arithmetic. Resource exhaustion, host allocation failure, index exhaustion, cancellation, or verifier rejection rolls back the entire two-operand stage and publishes nothing.

Parallel work may create task-private proposals and evidence. Final semantic IDs, edge groups, pair closure, fan reconstruction, canonical order, canonical bytes, and primary failure are determined by the frozen serial semantics. All workers are joined before rollback or commit.

## 3. Output contract

On success, publish one immutable `canonical_source_manifolds<T,I>` containing two verified `canonical_halfedge_operand<T,I>` artifacts.

Each operand artifact contains at least:

- stable provider/policy/schema/codec/verifier/formula versions;
- runtime owner binding held outside canonical semantics;
- operand role and predecessor artifact/digest links;
- canonical represented vertices;
- canonical oriented triangles;
- exactly three halfedges per triangle;
- role-tagged undirected edges and reciprocal pairs;
- vertex incidence and fan records;
- source-facet and source-shell groups;
- total source/internal and caller provenance maps;
- exact nominal bits, committed geometry-basis references, and precision lineage;
- conservative vertex/edge/triangle/facet/shell bounds;
- immutable query tables;
- canonical order maps;
- independently reconstructible evidence;
- resource and structural statistics;
- separate source-semantic, exact-topology, geometry-attachment, replay/presentation, and complete semantic digests; and
- canonical bytes excluding runtime owner values.

The wrapper guarantees:

- every represented source triangle appears exactly once;
- every triangle has one closed oriented three-halfedge cycle;
- every halfedge has one symmetric reverse pair;
- every edge has exactly two opposite uses;
- source edges agree with Components 02 and 04;
- internal diagonals pair only within one source facet and remain bookkeeping-only;
- every represented vertex has one closed fan;
- source facets, shells, nominal provenance, committed geometry basis, and caller provenance remain recoverable;
- coordinate-coincident distinct topology remains distinct;
- all required bounds are finite and conservative;
- canonical semantics are independent of presentation, owner anchor, allocation, hashing, workers, and schedule; and
- the artifact is sufficient for deterministic Component 06 candidate enumeration.

An empty operand publishes a valid canonical empty artifact with zero represented entities, valid predecessor links, versions, owner binding, digests, and verified disposition.

On failure, no operand artifact is published. The typed error identifies the stage/checkpoint, operand, least canonical offending semantic feature, expected and observed contract, relevant versions/digests, bounded evidence, resource state, and replay payload.

## 4. Required invariants and prohibited behavior

Required invariants:

- exact IDs and incidence are authoritative topology;
- runtime owner validation is separate from owner-free canonical semantics;
- the represented vertex domain equals the triangle-referenced predecessor domain;
- every triangle owns exactly three halfedges in one oriented cycle;
- every halfedge belongs to one triangle and one role-tagged undirected edge;
- reciprocal pairing is total and symmetric;
- paired endpoints reverse exactly by identity;
- every edge has exactly two uses;
- every represented vertex has exactly one closed fan;
- source edges and internal diagonals remain semantically distinct;
- internal diagonals remain available as bookkeeping topology without acquiring source ownership;
- facet and shell provenance is total and immutable;
- nominal source provenance and committed coherent geometry basis are both preserved;
- all required bounds conservatively contain the committed basis;
- canonical keys, IDs, bytes, digests, and deterministic failures exclude runtime owner values;
- all published data is immutable and independently verified; and
- legal source-facet retriangulation cannot change source-feature semantics.

Prohibited behavior:

- pairing or merging by coordinate value, distance, tolerance, overlapping bounds, spatial key, or hash;
- importing isolated non-semantic snapshot vertices into Component 05;
- substituting nominal geometry for a committed constructed coherent realization;
- selecting a new realization, plane, projection, or authoritative orientation;
- repairing missing, extra, or mismatched edge uses;
- dropping, reversing, replacing, or retriangulating a Component 04 triangle;
- merging adjacent coplanar source facets;
- making internal diagonals source owners, symbolic owners, winding barriers, or retained output features;
- hiding Component 06 candidate-policy decisions behind a generic edge `eligible` bit;
- using optional nominal normals, centroids, or spatial keys as topology predicates;
- assigning semantic IDs from owner tokens, pointers, source positions, allocation, traversal, tasks, workers, or unordered iteration;
- serializing runtime owner tokens, pointers, native structs, padding, container capacity, or hash layout;
- publishing incomplete pairs, maps, bounds, or mutable caches;
- continuing after capacity, resource, conservative-bound, codec, or verifier failure;
- using a legacy Boolean or mutable repair implementation as the provider; or
- calling an external topology, mesh, geometry, acceleration, serialization, hashing, or test library.

## 5. Required failure distinctions

At minimum, stable Component 05 subcodes must distinguish:

- unsupported provider/policy/schema/formula/codec/verifier version;
- wrong, stale, cross-context, cross-operand, or cross-domain handle;
- predecessor artifact, digest, disposition, or geometry-basis mismatch;
- represented-vertex-domain mismatch or isolated-source-vertex leakage;
- count, byte, offset, work, storage-index, or public-index overflow;
- temporary or persistent resource exhaustion;
- malformed vertex, triangle, local edge-use, facet, shell, or provenance reference;
- repeated triangle vertex or invalid orientation-preserving rotation;
- missing, repeated, non-closing, or wrong-length triangle cycle;
- unknown or contradictory edge role;
- source-edge missing, duplicate, same-direction, wrong-facet, wrong-shell, or wrong reciprocal use;
- internal-diagonal missing, duplicate, same-direction, cross-facet, or source-owner contamination;
- edge run cardinality other than two;
- one-way, self, endpoint-mismatched, or cross-edge pairing;
- canonical representative mismatch;
- missing, duplicate, wrong-origin, open, repeated, disconnected, or cross-identity vertex fan;
- facet/shell membership or occupied-side lineage mismatch;
- non-total or non-bijective map where bijection is required;
- missing, non-finite, stale, wrong-basis, wrong-formula, or non-conservative geometry attachment;
- runtime owner leaked into semantic key, bytes, digest, or deterministic error key;
- duplicate semantic key, canonical ordering mismatch, or full-key collision mishandling;
- malformed codec count, tag, length, reserved field, or trailing data;
- independent-verifier rejection;
- resource reconciliation or abstract-work failure;
- cancellation; and
- unexpected internal construction contradiction.

Expected contract, geometry-basis, capacity, resource, host-allocation, and cancellation failures remain typed and must not be collapsed into `internal_invariant_error`.

## 6. Test and validation specification

### 6.1 Entity, cycle, and domain tests

Cover:

- empty operands;
- tetrahedra and triangulated boxes;
- polygon-derived facets;
- high-valence vertices;
- disconnected shells, cavities, and islands;
- exact orientation-preserving local rotations;
- all halfedge accessors;
- all four V1 type combinations;
- capacity boundaries; and
- source snapshots containing isolated finite vertices that must remain absent from Component 05.

Every test independently reconstructs the represented vertex domain and triangle cycles.

### 6.2 Pairing and provenance tests

Include:

- ordinary source edges between distinct facets;
- several internal diagonals in a concave source facet;
- adjacent coplanar facets that remain separate;
- identical nominal and realized coordinates with distinct source IDs;
- duplicate coordinate patterns across shells;
- alternative legal diagonal layouts;
- signed-zero endpoints;
- IDs near capacity; and
- injected full-key hash collisions.

Verify exact two-use opposite pairing, role discovery without geometry, and source/internal provenance.

### 6.3 Vertex-link tests

Independently reconstruct links for valence three, four, and high valence; multiple source facets around one vertex; internal diagonals incident to a source vertex; coordinate-coincident vertices in separate components; cavity/island shells; and presentation permutations.

Inject open fans, duplicate incidences, bow ties, disconnected cycles, wrong pair transitions, and cross-identity links. Every mutation must be rejected deterministically.

### 6.4 Geometry-basis and bound tests

For both `nominal_embedded` and `constructed_coherent_realization` predecessors, verify:

- exact nominal bits remain unchanged;
- the committed shared realization reference is preserved;
- a constructed realization is never replaced by nominal geometry;
- edge, triangle, facet, and shell bounds enclose the committed basis and inherited uncertainty;
- support/projection/orientation lineage agrees with Components 02 and 04;
- signed zero, subnormals, extreme finite exponents, large translations, long thin triangles, and zero-width bound axes remain conservative; and
- mutated, stale, cross-owner, wrong-formula, or shrunk bounds are rejected.

Use the in-tree test-only exact or higher-precision oracle from Components 03 and 16 for bounded fixtures.

### 6.5 Owner-separation and canonical-byte tests

Run semantically identical predecessor artifacts under different valid runtime owner anchors, allocation layouts, task partitions, and worker schedules.

Require identical:

- semantic keys and dense IDs;
- canonical arrays and fan sequences;
- canonical bytes;
- source-semantic, exact-topology, geometry, replay/presentation, and complete semantic digests; and
- deterministic semantic diagnostics.

Inject raw owner tokens into keys, bytes, digests, golden records, and primary-error keys; the independent verifier must reject each mutation.

Wrong-owner handle use must still fail before data access.

### 6.6 Internal-diagonal downstream integration tests

Provide a Component 06 adapter test that:

- enumerates every Component 05 edge once through its canonical representative;
- includes internal diagonals under Component 06 V1;
- preserves `bookkeeping_only` status;
- never assigns source-feature or symbolic ownership to an internal diagonal;
- exposes both incident face uses;
- excludes internal diagonals from source-semantic digests; and
- proves alternative legal triangulations preserve source-feature relations and final Boolean semantics even when bookkeeping candidates differ.

### 6.7 Canonicalization and metamorphic tests

Partition expectations explicitly:

1. **Presentation-only changes** — vertex-array permutation with remapped indices, facet permutation, ring rotation, shell record permutation, allocation changes, hash collisions, worker counts, and schedules must produce byte-identical Component 05 semantic artifacts.
2. **Exactly semantics-preserving geometric transforms** — axis permutation, exactly representable translation, power-of-two scale, and corrected whole-shell orientation transforms must satisfy the predecessor-defined remapping/equivalence contract; byte identity is required only when Components 02-04 explicitly canonicalize the transform to identical predecessor semantic bytes.
3. **Alternative legal source triangulations** — source-semantic identities, groups, boundary-edge records, shell semantics, and downstream Boolean results must be equivalent; exact triangle, internal-diagonal, halfedge, fan ordering, and exact-topology bytes may differ.

Tests must not demand byte identity where a frozen predecessor contract permits different exact topology.

### 6.8 Codec, independent-verifier, and mutation tests

Golden encode/decode fixtures cover empty, tetrahedral, box, concave polygon-derived, disconnected, cavity/island, duplicate-coordinate, high-valence, and constructed-realization cases.

Test truncation at every byte, count/length overflow, unknown tags and versions, nonzero reserved fields, duplicate/missing sections, trailing required bytes, malformed optional discriminants, altered floating bits, digest substitution, and owner-token leakage.

Required structural mutations include deleting or duplicating entities, breaking triangle closure, changing edge role/provenance, one-way or same-direction pairing, adding a third edge use, splitting or joining fans, changing groups/maps, merging coordinate-coincident identities, substituting geometry basis, shrinking bounds, and permuting canonical arrays without matching maps.

The verifier must reject from independent reconstruction, not merely counts or digests.

### 6.9 Properties, fuzzing, shrinking, and regressions

Generate valid closed manifolds and polygon-derived source complexes from exact templates. Fuzz:

- topology-preserving subdivision;
- legal source triangulation;
- shell count and nesting;
- vertex valence;
- source presentation;
- duplicate coordinates without identity merge;
- coherent-realization displacement;
- ULP and exponent ranges;
- owner anchors;
- capacity and resource boundaries; and
- structural artifact mutations.

Every failure records exact source bits, predecessor artifacts, policies, versions, limits, and least failure key. Shrinking preserves category and violated invariant. Every fixed failure becomes a permanent in-tree regression.

### 6.10 Resource, cancellation, concurrency, and performance tests

For every resource and work class, test limit-minus-one, exact limit, and limit-plus-one. Cancel at stable boundaries in proposal generation, grouping, pair closure, fan traversal, group construction, bounds, encoding, and verification.

Run serial and deterministic parallel paths with delayed and reversed partitions. Accepted artifacts and selected failures must be identical under the applicable canonical contract. No run may expose a partial artifact, leaked reservation, active worker, or mutable predecessor.

Structural counters must establish:

- linear entity expansion;
- `O(H log H)` or better deterministic grouping for `H = 3F`;
- `O(H)` total fan traversal;
- linear persistent storage;
- bounded verifier duplication;
- no all-vertex, all-triangle, or all-edge pair scan; and
- deterministic resource/work accounting.

### 6.11 Definition of done

Component 05 is complete only when:

- both operand triangle complexes convert transactionally into one immutable verified wrapper;
- runtime owner validation is effective while owner tokens are absent from canonical semantics;
- the represented vertex domain exactly excludes non-semantic isolated snapshot vertices;
- nominal provenance and the committed coherent geometry basis are both preserved;
- every triangle has one exact oriented three-halfedge cycle;
- every edge has exactly two opposite reciprocal uses;
- source edges and internal diagonals pair from exact predecessor identity;
- internal diagonals remain queryable bookkeeping topology without source, symbolic, winding, or output ownership;
- every represented vertex has one independently verified closed fan;
- facet and shell groups preserve exact predecessor semantics;
- all maps are total, canonical, owner-checked, and independently reconstructed;
- all required bounds are finite, formula-versioned, and conservative for the committed basis;
- immutable queries satisfy Component 06 and later contracts without hidden policy or coordinate tests;
- presentation, owner anchor, allocation, hashing, workers, and schedules cannot change canonical semantic bytes or failures;
- alternative legal triangulations preserve source-feature and downstream Boolean semantics;
- all required unit, golden, exact-oracle, mutation, property, fuzz/shrink, resource, cancellation, concurrency, codec, replay, and structural-performance gates pass;
- production and normative tests remain strict standard-library-only C++17; and
- `tracker.md` marks Component 05 complete only after this reviewed specification and its implementation plan are committed.
