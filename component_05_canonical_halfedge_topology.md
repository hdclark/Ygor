# Component 05: Canonical Halfedge Topology

## Status and normative language

This document specifies a required component of a dependency-free bounded floating-point surface-mesh Boolean engine. Production and normative-test code must be portable C++17 and use no external dependency.

The concrete halfedge storage layout, allocation strategy, and canonicalization algorithm may change. The exact indexed-incidence, provenance, bounded-geometry, determinism, verification, and failure contracts in this document are normative.

## 0. Purpose

This component converts each verified `source_triangle_complex` from Component 04 into an immutable, canonically ordered, oriented halfedge manifold. It is the first pipeline artifact in which triangle cycles, reciprocal edge pairing, vertex incidence, source-facet grouping, and shell membership are represented in one exact indexed topology.

The component has four principal purposes:

- make all source-triangle incidence explicit and independently verifiable;
- distinguish original source features from facet-local triangulation artifacts without consulting coordinates;
- attach conservative geometric bounds and non-authoritative acceleration data to exact topological entities; and
- provide stable feature identities and query tables required by broad-phase enumeration, relation evaluation, event sharing, winding classification, diagnostics, and final provenance.

This component does not cut faces, evaluate cross-operand relations, create intersection vertices, classify winding, select Boolean output, repair input topology, or infer adjacency from geometric proximity. Its topology is a faithful indexed representation of the accepted source operands before Boolean interaction.

The principal output is one immutable `canonical_halfedge_operand` per operand, together forming the `canonical_source_manifolds` artifact.

## 1. Input contract

### 1.1 Required inputs

For each operand, the component must accept:

- one immutable `source_triangle_complex` from Component 04;
- the corresponding immutable `validated_operand` from Component 02;
- the immutable Boolean context and identity services from Component 01;
- the immutable precision context and bounded-geometry services from Component 03;
- canonical source vertex, edge, facet, shell, and triangle identities;
- source-boundary and facet-internal-diagonal provenance for every generated triangle edge;
- resource, cancellation, transaction, diagnostic, replay, and deterministic-order services; and
- the selected canonical-halfedge artifact version.

The component must not retain mutable references to caller-owned meshes or to Component 04 workspaces. Every published reference must target an immutable predecessor artifact whose lifetime covers the halfedge artifact.

### 1.2 Required predecessor guarantees

The component may rely on Component 04 having established all of the following, but it must defensively verify them before publication:

- every accepted source facet is represented by a complete set of oriented triangles;
- every triangle has three valid source vertex identities and accepted bounded orientation evidence;
- every source facet boundary edge is represented exactly once in that facet's triangle group under the v1 no-Steiner policy;
- every facet-internal diagonal has exactly two opposite triangle uses within one source-facet group;
- every triangle edge is labeled as either a source-boundary use or a facet-internal triangulation diagonal;
- every triangle, vertex, and edge label has total source provenance;
- triangle identities and source-feature identities are stable and context-owned; and
- coordinate-coincident but topologically distinct source vertices remain distinct identities.

If these guarantees are contradicted, the failure is an `internal_invariant_error` attributed to the predecessor artifact unless the contradiction can be traced to a typed input or resource failure that was not previously committed.

### 1.3 Accepted operand cases

The component must support:

- an empty operand under the public empty-solid semantics;
- one or more disconnected closed shells;
- cavities, islands, and deeper shell nesting already interpreted by Component 02;
- triangular and polygon-derived source facets;
- concave source facets represented by several triangles;
- high-valence manifold vertices;
- coordinate-coincident source vertices with distinct identities;
- adjacent coplanar source facets that remain separate source features;
- repeated source coordinates across disconnected shells; and
- `float` or `double` geometry with supported internal index capacities.

No empty or non-empty operand may bypass capacity, owner, digest, or artifact-version checks.

### 1.4 Capacity and ownership preconditions

Before allocating topology, the component must validate with overflow-safe arithmetic that the artifact can represent at least:

- one internal vertex record for every accepted source vertex occurrence;
- one internal triangle record for every source triangle;
- exactly three halfedge records per source triangle;
- one undirected-edge record per source undirected edge and per facet-local internal diagonal;
- all vertex-incidence, facet-group, shell, canonical-order, provenance, bound, and verifier-evidence tables; and
- any temporary sorting, pairing, or validation storage.

Every input ID and handle must belong to the current Boolean context, the expected operand, and the expected identity domain. Wrong-owner, stale, cross-operand, and cross-domain references must be rejected deterministically.

## 2. Required behavior

### 2.1 Exact internal entity model

The component must construct an exact indexed topology containing, conceptually, at least:

- internal vertex occurrences;
- oriented source triangles;
- directed halfedges;
- undirected edge records;
- reciprocal halfedge pairs;
- source-facet triangle groups;
- source shell groups;
- vertex incidence and cyclic-link records;
- source-to-internal and internal-to-source provenance maps; and
- canonical feature tables for later geometric stages.

The representation may use array indices, strongly typed IDs, compact offsets, or another portable C++17 layout. Regardless of layout, it must support constant-time or bounded deterministic lookup of a triangle's three directed edges, a halfedge's reverse pair, an edge's endpoints and incident triangles, a vertex's incident fan, and a triangle's source facet and shell.

No pointer address, container iteration order, allocation order, or worker schedule may be part of an entity's semantic identity.

### 2.2 Internal vertex construction

Each accepted source vertex occurrence must map to exactly one internal vertex occurrence within its operand unless a predecessor artifact explicitly represents multiple topological occurrences. Component 02's default manifold input contract normally yields one occurrence per canonical source vertex.

An internal vertex record must preserve:

- canonical source vertex identity;
- operand and shell membership;
- nominal coordinate bit pattern;
- bounded point or precision-ledger reference from Component 03;
- caller-index and source-provenance mappings needed for diagnostics;
- a canonical incident-halfedge representative or an empty marker; and
- a stable canonical key and artifact-local ID.

Distinct source vertex identities must remain distinct even when their nominal coordinates and uncertainty envelopes are bit-identical. The component must not deduplicate, weld, bucket-merge, or alias vertices by coordinate value, distance, Morton code, hash, or tolerance.

### 2.3 Triangle and halfedge cycle construction

For every source triangle, the component must create exactly one oriented triangle record and exactly three directed halfedges. Each triangle cycle must:

- preserve the source triangle orientation;
- reference the same three internal vertex identities in the same cyclic order;
- have total `next` and `previous` relations, whether stored or derivable;
- close after exactly three steps;
- contain no repeated halfedge identity;
- identify its source facet, source shell, operand, and source triangle;
- preserve each edge's source-boundary or internal-diagonal classification; and
- carry bounded orientation and planar-support references without recomputing them through a different authoritative expression.

A triangle's canonical local halfedge order must be derived from its oriented source vertex cycle and stable IDs. Canonical rotation may change which corner is local slot zero, but it must not reverse orientation.

The destination of a halfedge may be stored explicitly or derived as the origin of `next`. In either case, endpoint agreement must be independently checkable.

### 2.4 Undirected edge identity and reciprocal pairing

Every halfedge must belong to exactly one undirected edge record and have exactly one reciprocal pair.

Pairing must use authoritative topological provenance:

- a source-boundary halfedge is keyed by its canonical source undirected-edge identity;
- a facet-internal halfedge is keyed by its canonical source-facet identity and facet-local diagonal identity; and
- no edge is paired solely from coordinate equality or unordered endpoint coordinates.

For each source undirected edge, the component must verify and publish exactly two halfedge uses that:

- come from the two validated source directed-edge uses;
- belong to the correct adjacent source facets;
- have reversed source endpoint identities;
- have reversed internal endpoint identities;
- refer to the same source undirected-edge identity; and
- belong to the same operand and shell.

For each facet-local internal diagonal, the component must verify and publish exactly two halfedge uses that:

- belong to two triangles in the same source-facet group;
- have reversed endpoint identities;
- carry the same facet-local diagonal identity;
- are marked non-source and non-geometric for source-feature ownership; and
- do not cross a source facet boundary.

Reciprocity is mandatory: if `pair(h) == r`, then `pair(r) == h`. One-way pairing, more than two uses, endpoint disagreement, wrong-facet pairing, and cross-operand pairing are invariant violations and must prevent publication.

### 2.5 Authoritative versus bookkeeping edge semantics

Every undirected edge and halfedge must expose an identity-only semantic classification sufficient for downstream components to distinguish:

- an original source edge;
- a directed use of an original source edge by one source facet;
- a facet-internal triangulation diagonal; and
- any future explicitly versioned edge class.

For a source edge, the artifact must retain both oriented source-facet uses even if later stages use one canonical geometric segment representative. For an internal diagonal, the artifact must retain the two triangle uses but must make clear that the diagonal:

- is not an original geometric feature;
- cannot own a symbolic source contact;
- cannot by itself split an uncut source-facet classification region;
- may be crossed or replaced by legal re-triangulation without changing source-facet semantics; and
- must be traced through its source-facet provenance by Components 06-09.

No downstream component may need a coordinate test to determine whether an edge is original or bookkeeping.

### 2.6 Source-facet groups and planar provenance

All triangles produced from one source facet must be grouped by the authoritative source-facet identity. A source-facet group must preserve:

- the canonical boundary ring and directed source-edge uses;
- member triangle identities;
- internal-diagonal identities and pairings;
- shell and operand membership;
- source orientation;
- deterministic projection-frame and bounded planar-support references;
- caller-facet provenance; and
- a stable group digest.

The group must allow downstream code to move across an uncut internal diagonal while remaining in the same source-facet semantic region.

"Coplanar provenance" in this component means that every member triangle carries the common accepted planar support and source-facet lineage from Components 02 and 04. This component must not merge separate source facets into a larger coplanar patch merely because nominal normals or plane coefficients are close. Any cross-facet or cross-operand coplanarity decision belongs to bounded relation evaluation and symbolic policy in Component 07.

### 2.7 Vertex incidence and cyclic-link reconstruction

The component must build or derive the incident halfedge fan for every internal vertex occurrence. Before publication, it must verify that the triangulated topology preserves one closed manifold link per source vertex occurrence.

The verifier must establish at least:

- every incident halfedge starts or ends at the expected vertex;
- traversal through triangle adjacency and reciprocal pairs returns to the starting incidence;
- the fan has no open end;
- every incidence is visited exactly once;
- no second disjoint fan shares the same internal vertex identity;
- internal diagonals refine, but do not split, the source vertex link; and
- coordinate-coincident vertices do not enter each other's fan.

The artifact must publish a canonical incident representative and either an ordered cyclic fan or sufficient exact adjacency to reconstruct it deterministically.

### 2.8 Bounded geometric attachments

The component must attach conservative geometry metadata without making floating geometry authoritative for topology. At minimum it must provide or reference:

- bounded source point data for every internal vertex;
- a closed conservative bound for every undirected edge segment;
- a closed conservative bound for every triangle;
- bounded planar support and oriented-area evidence for every triangle;
- conservative source-facet and shell bounds; and
- finite nominal geometry needed for acceleration heuristics.

Feature bounds must be produced using Component 03 and must contain every geometric realization permitted by the inherited precision envelopes. Bounds must not be shrunk to nominal coordinates, and non-finite or unrepresentable conservative bounds must cause a typed failure rather than an unsafe artifact.

The component may compute non-authoritative data such as:

- nominal area vectors;
- centroids;
- dominant axes;
- unnormalized normals;
- spatial sort keys; and
- local scale estimates.

Such data may guide acceleration or deterministic ordering only. A later topology-affecting decision must use bounded predicates or Component 07 relation results, not an unchecked cached normal, normalized direction, centroid, or scalar area.

### 2.9 Canonical identities and ordering

All published internal entities require stable IDs and total orderings. Canonicalization must be derived from predecessor canonical identities and frozen tie rules, for example:

- internal vertex key from operand role and canonical source vertex identity;
- triangle key from operand role and canonical source triangle identity;
- source-edge key from operand role and canonical source undirected-edge identity;
- internal-diagonal key from operand role, canonical source facet, and facet-local diagonal identity;
- halfedge key from its edge key, incident triangle key, directed endpoint identities, and source-use role; and
- group keys from canonical source facet or shell identity.

The exact key encoding may differ, but it must be total, versioned, collision-safe through full-key comparison, and independent of source array order after canonicalization.

The component must publish arrays in canonical order or publish immutable canonical-order permutations. Hash tables may be used for temporary lookup, but hash iteration order must not affect IDs, serialization, diagnostics, or downstream traversal.

A canonical directed representative for every undirected edge must be available to Component 06 and Component 07. Its direction must be selected from stable endpoint and provenance keys, not from memory order or floating coordinates. Both face-oriented halfedge uses remain available separately.

### 2.10 Downstream topology and feature-query contract

The artifact must provide narrow immutable query services or tables for later components. At minimum, downstream code must be able to determine by identity alone:

- whether a feature belongs to operand A or B;
- whether an edge is a source edge or internal diagonal;
- an edge's canonical endpoints and directed representative;
- both incident halfedges and triangles of an undirected edge;
- a triangle's vertices, halfedges, source facet, shell, and orientation;
- whether two triangles share an edge, vertex, source facet, or shell;
- whether an edge is incident to a triangle;
- whether traversal across an edge stays within one source facet;
- all triangles belonging to a source facet;
- all features belonging to a shell; and
- the conservative bounds and precision references for vertices, edges, triangles, facets, and shells.

These queries must not mutate caches with scheduling-dependent behavior. Any lazy cache must have deterministic content and thread-safe publication, or be replaced by eager immutable tables.

### 2.11 Independent verification evidence

The component must publish enough evidence for an independent verifier to reconstruct rather than trust:

- triangle cycles;
- endpoint agreement;
- undirected edge-use multiplicity;
- reciprocal pairing;
- source-edge and internal-diagonal provenance;
- source-facet triangle membership;
- vertex-link cycles;
- shell membership;
- canonical ordering;
- feature-bound containment; and
- artifact digest inputs.

Producer-owned summary booleans such as `all_edges_paired=true` or counts alone are not sufficient.

The verifier must be able to start from Component 04 triangles and provenance and independently rebuild the expected incidence relations without calling the producer's pairing or canonicalization helpers.

### 2.12 Transactionality, cancellation, and resource limits

Construction must occur in a private stage workspace. Before committing, the component must verify the complete operand artifact and reserve all persistent storage.

Resource accounting must cover at least:

- vertex, triangle, halfedge, and undirected-edge records;
- incidence and vertex-fan storage;
- source-facet and shell-group tables;
- provenance and reverse maps;
- bounded geometry and spatial-key records;
- canonical sorting and pairing workspaces;
- verifier evidence and diagnostics; and
- abstract work units for sorting, grouping, and fan traversal.

All counts and byte calculations must be overflow-safe. Limit exhaustion, index-capacity exhaustion, or cancellation must roll back the entire stage and publish no partial topology.

Cancellation must be checked at deterministic safe points during triangle expansion, edge grouping, vertex-fan construction, canonical sorting, and verification. All worker activity must be joined before rollback.

Parallel construction may use task-local records, but canonical IDs and final pairing must be published through deterministic merge order. No task may mutate a published shared halfedge record.

## 3. Output contract

For each operand, the component must produce one immutable `canonical_halfedge_operand<T>` containing at least:

- operand identity and artifact version;
- canonical internal vertex records;
- canonical oriented triangle records;
- exactly three halfedges per triangle;
- reciprocal halfedge-pair relations;
- canonical undirected source-edge records;
- canonical facet-local internal-diagonal records;
- vertex-incidence and cyclic-link data;
- source-facet triangle groups;
- shell groups and occupied-side provenance inherited from Component 02;
- total source-to-internal and internal-to-source provenance maps;
- conservative vertex, edge, triangle, facet, and shell bounds;
- bounded orientation and planar-support references;
- canonical directed edge representatives;
- immutable topology and feature-query tables;
- canonical ordering maps;
- resource and structural statistics;
- independent-verification evidence; and
- a deterministic canonical digest.

The pair of operand artifacts forms `canonical_source_manifolds` and must guarantee:

- every source triangle is represented exactly once;
- every triangle has one closed oriented three-halfedge cycle;
- every halfedge has exactly one reciprocal pair;
- paired halfedges have reversed endpoint identities;
- every undirected edge has exactly two directed uses;
- source-edge pairs agree with Component 02 source incidence;
- internal diagonals pair only within one source-facet group;
- every internal vertex occurrence has one closed cyclic fan;
- source facets, shells, and caller provenance remain recoverable;
- source features and triangulation artifacts are distinguishable by identity;
- coordinate-coincident distinct vertices remain topologically separate;
- all geometric bounds are finite and conservative;
- canonical bytes are independent of permitted input ordering and worker schedule; and
- the artifact is suitable for deterministic Component 06 feature enumeration.

An empty accepted operand must produce a valid empty artifact with a canonical digest and zero entity counts.

On failure, no halfedge operand is published. The typed error must identify the operand, canonical source or triangle features involved, violated incidence or bound contract, relevant counts and limits, and deterministic replay payload.

## 4. Required invariants and prohibited behavior

Required invariants:

- topology is represented only by exact IDs and incidence;
- every triangle owns exactly three halfedges in one oriented cycle;
- every halfedge belongs to exactly one triangle and one undirected edge;
- reciprocal pairing is total and symmetric;
- paired endpoints reverse exactly by identity;
- each undirected edge has exactly two uses;
- every vertex occurrence has exactly one closed incident fan;
- source edges and internal diagonals remain semantically distinct;
- source-facet and shell provenance is total and immutable;
- feature bounds conservatively contain inherited geometry;
- all arrays, maps, and IDs have canonical deterministic order;
- the published artifact is immutable and context-owned; and
- legal source-facet re-triangulation cannot convert an internal diagonal into a source feature or alter source-facet semantics.

Prohibited behavior:

- pairing edges from coordinate values or tolerance;
- merging vertices because coordinates or bounds overlap;
- repairing missing, extra, or mismatched edge uses;
- silently dropping a degenerate or inconvenient triangle;
- merging adjacent coplanar source facets by approximate normal or plane comparison;
- using cached nominal normals as authoritative topology predicates;
- assigning IDs from pointer addresses, allocation order, thread timing, or unordered-container iteration;
- publishing one-way or incomplete pairing;
- allowing a facet-internal diagonal to acquire source-edge ownership;
- retaining mutable references to a stage workspace;
- continuing after a capacity or conservative-bound failure; or
- calling an external topology, mesh, acceleration, or geometry library.

## 5. Test and validation specification

### 5.1 Entity and cycle unit tests

Unit tests must cover:

- empty artifacts;
- one tetrahedral shell;
- triangulated boxes;
- polygon-derived facets with several triangles;
- high-valence manifold vertices;
- multiple disconnected shells;
- cavities and nested shell groups;
- canonical local triangle rotation;
- all halfedge accessors and endpoint derivations; and
- capacity boundaries for every internal index type.

Every test must reconstruct triangle cycles independently and verify exact endpoint and provenance agreement.

### 5.2 Edge-pairing and source-provenance tests

Include fixtures containing:

- ordinary source edges shared by two different source facets;
- several internal diagonals in one concave source facet;
- adjacent coplanar source facets that must not be merged;
- identical endpoint coordinates with different source vertex IDs;
- duplicate coordinate patterns across different shells;
- facets whose local triangulations use different diagonal orientations; and
- large source edge and facet IDs near capacity limits.

Verify source edges pair across validated source directed uses, internal diagonals pair only within their source facet, and every edge kind remains discoverable without geometry.

### 5.3 Vertex-link and manifold tests

Independently reconstruct the link of every vertex in fixtures with:

- valence three, four, and high valence;
- triangles from several source facets around one vertex;
- internal diagonals incident to a source vertex;
- coordinate-coincident vertices in separate components;
- a cavity shell and an island shell; and
- source array and triangle-order permutations.

Inject open fans, duplicated incidences, disconnected bow-tie fans, and wrong twin transitions and require deterministic rejection.

### 5.4 Bounded geometry tests

For vertex, edge, triangle, facet, and shell bounds, test:

- exact ordinary coordinates;
- one-ULP uncertainty;
- signed zero;
- subnormal coordinates;
- very large translations with small local features;
- extreme finite exponents;
- long thin triangles; and
- coordinate-coincident but distinct features.

An in-tree exact or higher-precision test oracle must verify that all permitted source realizations used by the fixtures are contained. Mutations that shrink one bound below a source endpoint or triangle realization must be rejected.

### 5.5 Canonicalization and metamorphic tests

Apply:

- vertex-array permutation with remapped indices;
- facet permutation;
- ring rotation;
- disconnected shell permutation;
- legal alternative source-facet triangulations;
- exact power-of-two scale;
- exactly representable translation;
- axis permutation;
- global orientation reversal with corrected solid policy;
- thread counts 1, 2, and maximum; and
- forced task partition and scheduling permutations.

Where the frozen policy defines unique canonical source triangles, canonical artifact bytes must be identical. Where legal triangulations differ, source-facet semantic tables, source-feature identities, shell provenance, and downstream Boolean results must remain equivalent even if internal-diagonal records differ.

### 5.6 Independent verifier and mutation tests

Corrupt valid artifacts by:

- deleting a halfedge;
- assigning four halfedges to one triangle;
- breaking `next` or `previous` closure;
- making pairing one-way;
- pairing halfedges with equal rather than reversed endpoints;
- assigning three uses to an undirected edge;
- pairing a source edge to an internal diagonal;
- pairing internal diagonals across source facets;
- changing a triangle's source facet or shell;
- merging two coordinate-coincident internal vertices;
- splitting one vertex fan into two cycles;
- changing a canonical directed representative;
- omitting a source-to-internal provenance entry;
- permuting canonical arrays without updating canonical maps;
- shrinking a feature bound; and
- forging counts or digests while leaving corrupt incidence.

The independent verifier must reject every mutation from reconstructed incidence, not solely from producer counts or checksums.

### 5.7 Fuzzing and shrinking

Generate valid closed triangle manifolds and polygon-derived source triangle complexes from exact templates. Fuzz:

- topology-preserving subdivisions;
- facet triangulations;
- shell count and nesting;
- vertex valence;
- source feature permutations;
- coordinate duplication without identity merging;
- ULP perturbations; and
- internal index capacities.

Also mutate valid artifacts structurally. Every failure must serialize exact source bits and identities and shrink while preserving the failure category and violated invariant.

### 5.8 Resource, cancellation, and concurrency tests

For each resource class, test limit-minus-one, limit, and limit-plus-one. Cancel during triangle expansion, edge grouping, fan construction, canonical sorting, and verification.

Run serial and deterministic parallel builds with delayed tasks and reversed partition order. The same accepted input must produce byte-identical artifacts and the same primary typed failure regardless of thread count or schedule.

No cancelled or resource-limited run may expose a partial artifact or leaked reservation.

### 5.9 Structural performance tests

Use counters rather than wall-clock time alone to establish:

- linear entity expansion from triangle count;
- `O(E log E)` or better deterministic edge grouping under the selected provider;
- bounded vertex-fan traversal proportional to incidence;
- no accidental all-vertex or all-triangle pair scans;
- no quadratic persistent memory growth; and
- deterministic accounting across thread counts.

A provider may choose different asymptotics only if the implementation plan documents them and the qualification gates prevent pathological regressions.

### 5.10 Definition of done

Component 05 is complete only when:

- both operand triangle complexes convert transactionally into immutable halfedge manifolds;
- all triangle cycles, reciprocal edge pairs, and vertex fans are independently verified;
- source versus internal-diagonal semantics are unambiguous by identity;
- source-facet, shell, and caller provenance is total;
- conservative feature bounds are available for Component 06;
- coordinate-coincident distinct topology is preserved;
- canonical artifacts and failures are schedule-independent;
- mutation, fuzz, resource, cancellation, and performance gates pass;
- legal source re-triangulation cannot change source-feature semantics; and
- production and normative tests are strict portable C++17 with no external dependencies.
