# Component 04: Source Polygon Facet Triangulation and Provenance

## Status and normative language

This document specifies a required component of a dependency-free bounded floating-point surface-mesh Boolean engine. Production and normative-test code must be portable C++17 and use no external dependency.

The specific triangulation algorithm may change. The coverage, boundary-preservation, determinism, provenance, bounded-geometry, and failure contracts in this document are normative.

## 0. Purpose

This component converts every validated polygonal source facet into a deterministic triangle complex suitable for halfedge construction, collision enumeration, relation evaluation, and winding computation. The conversion must preserve the source facet's bounded planar region, source-boundary topology, orientation, and provenance.

Internal diagonals introduced by triangulation are bookkeeping edges only. They must never be mistaken for original geometric features, used to change symbolic contact ownership, or allowed to alter the topological result of a Boolean operation.

The principal output is an immutable `source_triangle_complex` for each validated operand.

## 1. Input contract

The component must accept:

- one immutable `validated_operand` from Component 02;
- the immutable Boolean and precision contexts from Components 01 and 03;
- normalized facet rings, source-feature identities, orientation, shell membership, and bounded planar evidence;
- resource, cancellation, transaction, diagnostic, and deterministic-order services; and
- the selected source-triangulation policy version.

Each input facet must already satisfy Component 02's contract:

- at least three distinct topological vertices;
- one simple polygonal ring in v1;
- bounded planarity;
- valid source indices;
- consistent shell orientation; and
- deterministic source identity.

This component must not accept an arbitrary polygon soup or repair malformed source rings.

## 2. Required behavior

### 2.1 Deterministic local frame

For each source facet, the component must construct a deterministic projection frame from bounded geometric evidence. The frame selection must:

- preserve the accepted source orientation;
- avoid an unstable projection direction when another qualified direction is available;
- use stable feature IDs to break exact ties;
- record the projection and its uncertainty;
- be independent of ring rotation and input facet order; and
- fail with a typed geometry error if no projection can support a reliable triangulation within the facet's precision.

Projection is an implementation detail. The output contract is expressed in source topology and three-dimensional bounded geometry, not in projected coordinates.

### 2.2 Ring normalization for triangulation

The component may create an internal triangulation ring that:

- rotates to a canonical start;
- removes vertices proven redundant because consecutive source edges are topologically zero-length by identical indices, which should normally already be removed by Component 02;
- marks geometrically collinear or coordinate-coincident but topologically distinct vertices without deleting them unless the source contract explicitly permits a topology-preserving representation; and
- retains a total mapping back to source ring positions.

Distinct source vertex identities must never be merged because projected coordinates are equal or close.

### 2.3 Triangle generation

The component must partition the accepted facet region into oriented triangles. It may use any in-tree deterministic method whose provider-level implementation is selected later, provided that it:

- preserves every source boundary edge exactly as a boundary chain with the same endpoint identities;
- introduces only internal diagonals between existing source vertex identities in v1, unless a later policy explicitly permits bounded Steiner points;
- creates no positive-area overlaps;
- leaves no positive-area gaps;
- produces triangles with orientation consistent with the source facet;
- terminates with either a complete triangulation or a typed failure;
- handles concave polygons, long thin features, repeated-coordinate identities, and boundedly collinear sequences according to policy; and
- records sufficient evidence for independent coverage verification.

A triangle that is numerically degenerate under the source precision must not silently receive an arbitrary orientation. The component must either produce a contract-approved degenerate bookkeeping record for later handling or fail. The default source-triangle complex supplied to Component 05 should contain only triangles whose orientation is accepted by the bounded predicate contract.

### 2.4 Boundary-edge preservation

Every source directed edge use must map to exactly one directed triangle-boundary edge unless the triangulation policy explicitly subdivides source edges. Under the initial no-Steiner policy, subdivision is prohibited.

The mapping must preserve:

- source edge identity;
- source directed-use identity;
- endpoint source vertex identities;
- direction within the source facet ring;
- paired source use on the adjacent source facet; and
- shell provenance.

The two incident source facets may triangulate independently, but their shared source edge must retain identical endpoint identities and opposite directed uses. No coordinate snapping is permitted.

### 2.5 Internal diagonal semantics

Each introduced diagonal must be marked as:

- internal to one source facet;
- not an original source edge;
- not eligible for source-feature symbolic ownership;
- not a barrier to connectivity or winding propagation when the original facet is uncut;
- paired within the same source facet triangle group after Component 05 builds halfedges; and
- removable or replaceable by an alternative legal triangulation without changing source-facet semantics.

Downstream components must be able to distinguish source edges from internal diagonals without consulting coordinates.

### 2.6 Provenance graph

The component must produce a complete provenance graph mapping:

- source facets to generated triangles;
- generated triangles to source facets and shells;
- triangle vertices to source vertex identities;
- triangle boundary edges to source directed-edge uses;
- triangle internal edges to source-facet-local diagonal identities;
- triangle orientation to source orientation; and
- any accepted degeneracy marker to its source ring evidence.

Provenance must be stable under source-array permutations and legal alternative internal work ordering.

### 2.7 Canonical triangle identities and order

Generated triangles require stable identities. Canonicalization must use source-facet identity plus a deterministic triangle key derived from source vertex identities and orientation. It must not use allocation order or task scheduling.

The component must publish triangles in canonical order or publish a canonical-order mapping used by Component 05. Symmetric triangulations must be resolved by the frozen triangulation tie rules.

### 2.8 Bounded coverage evidence

For each facet, the component must record evidence sufficient to verify:

- the union of triangle boundaries contains exactly the source ring boundary plus paired internal diagonals;
- each internal diagonal has two opposite uses;
- triangle orientation is accepted;
- triangle interiors do not overlap with positive area;
- total accepted projected area agrees with the source polygon within conservative bounds; and
- every source-ring segment is represented.

Coverage verification must not depend solely on summing floating areas, because cancellation and overlapping triangles can preserve a sum. Combinatorial boundary cancellation and independent point/segment checks are required.

### 2.9 Alternative-triangulation invariance support

The component must make source-facet identity authoritative so downstream topology is insensitive to legal source triangulation. In particular:

- candidate/event ownership must trace through the source facet, source edge, and source vertex rather than rely only on a generated triangle number;
- intersections that cross internal diagonals must be recognized as belonging to one source facet relation lineage;
- connectivity across uncut internal diagonals must be preserved; and
- tests must be able to substitute alternative legal triangulations of the same source facet.

### 2.10 Transactionality and limits

Triangulation of a facet may use private mutable work, but publication is transactional. Before publication the component must verify local coverage and provenance.

Resource accounting must cover:

- projected ring storage;
- work queues or search structures;
- generated triangles;
- internal diagonals;
- provenance records;
- verification evidence; and
- abstract triangulation work.

A pathological polygon must terminate through resource or work limits rather than loop indefinitely.

## 3. Output contract

For each operand, the component must produce one immutable `source_triangle_complex` containing:

- canonical source vertex references and nominal coordinates;
- all generated oriented source triangles;
- stable triangle IDs;
- source-facet triangle ranges or membership maps;
- source boundary-edge labels on triangle halfedges;
- source-facet-internal diagonal labels;
- shell and operand provenance;
- deterministic projection records;
- per-triangle bounded orientation and plane evidence;
- facet-level coverage evidence;
- canonical ordering and digest; and
- reverse mappings to caller facet/ring positions.

The output must guarantee:

- every accepted source facet is completely triangulated;
- source boundary topology is unchanged;
- every internal diagonal has exactly two opposite uses within one source-facet group;
- every generated triangle has accepted orientation or an explicitly permitted marker that downstream components understand;
- downstream code can distinguish source features from triangulation artifacts using identities alone; and
- changing to another legal triangulation cannot change the semantic identity of the source facet.

On failure, no partial triangle complex is published. The typed error must identify the source facet, relevant ring positions, bounded predicate evidence, and replay payload.

## 4. Required invariants and prohibited behavior

Required invariants:

- source-ring boundary is preserved exactly in index topology;
- internal diagonals cancel in opposite pairs;
- triangle orientation agrees with the source facet;
- triangle provenance is total and immutable;
- coordinate equality never merges source vertices;
- triangulation ordering is deterministic;
- legal re-triangulation does not alter source-feature semantics; and
- only verified complete facet results enter the published artifact.

Prohibited behavior:

- tolerance-based welding of source vertices;
- dropping a narrow source feature merely to make triangulation easier;
- introducing cracks along shared source edges;
- labeling an internal diagonal as a source edge;
- resolving an uncertain orientation with arbitrary ear selection;
- publishing a partial triangulation;
- using a library triangulator or any external dependency; or
- relying only on area equality as proof of coverage.

## 5. Test and validation specification

### 5.1 Known-answer polygon corpus

Include hand-auditable facets covering:

- triangles and convex polygons;
- concave polygons with multiple reflex vertices;
- long thin polygons;
- repeated-coordinate but distinct-index vertices;
- boundedly collinear chains;
- nearly collinear ears;
- large vertex counts;
- projection-axis ties;
- signed-zero and subnormal coordinates; and
- large translations with small local features.

For each, store expected source boundary, triangle count range where not uniquely prescribed, orientation, provenance, and canonical digest under the frozen policy.

### 5.2 Coverage verification tests

Independently reconstruct:

- directed boundary cancellation;
- internal-diagonal pairing;
- source-boundary equality;
- triangle adjacency;
- projected segment intersections;
- representative interior coverage; and
- conservative area agreement.

Do not call the producer's ear-selection or internal-diagonal helpers from the verifier.

### 5.3 Determinism and metamorphic tests

Apply:

- ring rotation;
- facet order permutation;
- vertex-array permutation with remapping;
- exactly representable translation;
- power-of-two scale;
- axis permutation;
- global orientation reversal with corrected shell policy; and
- forced task scheduling permutations.

Canonical source-triangle output must remain byte-identical when deterministic policy requires a unique triangulation. If the policy permits alternative internal diagonals, the semantic provenance and coverage digest must remain equivalent and later Boolean results must be invariant.

### 5.4 Alternative-triangulation tests

For facets with several legal triangulations:

- feed independently enumerated legal triangulations into test adapters for Component 05 and later components;
- verify source-facet relations and final Boolean topology are unchanged;
- test intersections passing through different internal diagonals; and
- confirm internal diagonals do not become classification barriers.

### 5.5 Boundary-sharing tests

Construct adjacent source facets that share edges under:

- ordinary coordinates;
- reversed array order;
- duplicate coordinate values elsewhere;
- very long edges;
- subnormal offsets; and
- different local projection axes.

Verify identical endpoint identities and opposite source-edge directions survive triangulation.

### 5.6 Mutation tests

Corrupt valid output by:

- deleting one triangle;
- duplicating a triangle;
- flipping one triangle orientation;
- introducing overlapping triangles;
- omitting one internal-diagonal partner;
- labeling a diagonal as a source edge;
- changing one source boundary endpoint;
- assigning a triangle to the wrong source facet; and
- shrinking an orientation enclosure.

Independent verification must reject each mutation.

### 5.7 Fuzzing and shrinking

Generate valid simple polygons from exact rational templates, convert to `T`, and apply controlled ULP perturbations. Fuzz ring size, reflex patterns, collinear runs, and scale. Every failure must serialize exact source bits and shrink while preserving the failure category.

### 5.8 Performance and resource tests

Use structural counters to test:

- expected near-linear behavior for ordinary convex and mildly concave rings;
- bounded work for adversarial ear-blocking patterns;
- deterministic resource-limit failure; and
- no accidental quadratic memory growth beyond documented policy.

### 5.9 Definition of done

Component 04 is complete only when:

- all accepted facet classes triangulate or fail with typed deterministic errors;
- boundary and coverage are independently verified;
- source versus internal-edge provenance is unambiguous;
- legal source re-triangulation cannot change later Boolean semantics;
- canonical output is stable under input permutations;
- pathological inputs terminate within resource limits; and
- production and normative tests are strict portable C++17 with no external dependencies.
