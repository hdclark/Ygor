# Component 14: Output Assembly and Canonical Serialization

## Status and normative language

This document specifies a required component of a dependency-free bounded floating-point surface-mesh Boolean engine. Production and normative-test code must be portable C++17 and use no external dependency.

The concrete public-mesh builder, canonical graph-labeling provider, component signature, temporary index-remap storage, provenance compression, logical byte encoding, and digest implementation may change. Lossless conversion from the cleaned internal manifold, duplicate-coordinate preservation, checked index conversion, canonical ordering independent of transient identities and schedules, monotonic precision/report assembly, deterministic logical serialization, round-trip structural equivalence, Component 15 verification handoff, and failure contracts in this document are normative.

## 0. Purpose

This component converts the verified-clean internal triangle manifold from Component 13 into the exact public `fv_surface_mesh<T, I>` representation and assembles the complete deterministic result candidate for Component 15.

Its purposes are to:

- map every cleaned internal topological vertex occurrence to one public vertex index without coordinate-based deduplication;
- copy every authoritative nominal coordinate to the public scalar representation under the frozen rounding and signed-zero policy;
- map every cleaned oriented triangle to one public triangular facet while preserving outward orientation;
- preserve separate public indices for topology-distinct vertices with bit-identical coordinates;
- check every count, offset, allocation, and conversion against the capacity of `I` and configured resources before publication;
- assign canonical component, vertex, and face order independent of source indexing, internal allocation order, cleanup mutation history, hash iteration, and worker schedule;
- assemble output precision, tolerance usage, topology, geometry, cleanup, provenance, deterministic, and replay reports without understating any contributor;
- serialize a canonical logical byte representation and deterministic digest using versioned platform-independent encoding rules;
- reconstruct and compare the public mesh's indexed topology against the cleaned internal manifold; and
- provide Component 15 with an immutable `assembled_output_candidate<T, I>` that requires verification but no further topology repair, cleanup, reorientation, or canonical reordering.

This component is an assembly and serialization boundary. It does not perform Boolean selection, construct intersections, pair missing edges, triangulate polygons, move vertices, collapse or weld features, remove components, merge coplanar triangles into polygons, or declare ordinary public success before Component 15 accepts the candidate.

The principal output is an immutable `assembled_output_candidate<T, I>` containing:

- a fully populated canonical `fv_surface_mesh<T, I>`;
- canonical internal-to-public and public-to-internal maps;
- candidate output precision and tolerance-use summaries;
- topology, geometry-pending-verification, cleanup, topology-change, provenance, resource, and determinism reports;
- canonical logical serialization and digests;
- structural round-trip evidence; and
- complete references needed by Component 15 for independent final verification and replay.

## 1. Input contract

### 1.1 Required inputs

The component must accept:

- the immutable `cleaned_triangle_manifold<T>` from Component 13;
- the immutable Component 13 cleanup action, displacement, feature-removal, topology-change, and precision reports;
- immutable provenance references to Components 02, 04, 05, 08, 10, 11, and 12 required to assemble the public provenance report and independent-verification map;
- the immutable `precision_context<T>` and final precision-ledger aggregation services from Component 03;
- the immutable Boolean context, output policy, public scalar/index descriptors, identity domains, deterministic comparators, resources, cancellation, diagnostics, replay, and transaction services from Component 01;
- a supported in-tree adapter or builder for `fv_surface_mesh<T, I>`;
- the selected canonical-labeling, public-mesh, report, logical-serialization, digest, artifact, and replay versions; and
- verification settings controlling assembly checks and Component 15 handoff detail.

The component must not read mutable caller meshes, reinterpret source coordinates, change cleaned topology, infer vertex sharing from coordinates, invoke external serialization or graph-canonicalization libraries, or expose mutable public-mesh storage before commit.

### 1.2 Required predecessor guarantees

The component may rely on Component 13 having established:

- finite bounded coordinates for every cleaned vertex occurrence;
- one closed incident triangle fan per topological vertex occurrence;
- reciprocal paired halfedges with exact reversed endpoints;
- exactly two triangle uses per undirected edge;
- oriented triangles with three distinct vertex occurrence IDs and definite accepted nonzero area;
- no unresolved cleanup obligations;
- complete old-to-new provenance and cleanup action certificates;
- cumulative displacement and feature-removal costs within policy and caller tolerance;
- output precision candidates dominating inherited, construction, and cleanup contributions;
- explicit component/genus/cavity topology-change reports;
- no known forbidden cleanup-introduced intersection; and
- deterministic immutable topology and digests.

The component must defensively verify owner tokens, versions, entity ranges, coordinate validity, pair reciprocity, triangle cycles, vertex links, report references, precision-ledger completeness, cleanup-action coverage, topology-change consistency, and predecessor digests.

A contradiction in a committed predecessor artifact is an `internal_invariant_error`. Assembly must not skip an invalid triangle, duplicate a missing reverse face, normalize topology by welding, or lower reported precision so the candidate appears valid.

### 1.3 Public mesh contract

The v1 public output policy must produce a facet-vertex surface mesh with triangular facets.

The public mesh must support, directly or through the Ygor in-tree adapter:

- a contiguous vertex-coordinate sequence of `T` triples;
- a contiguous facet sequence;
- exactly three `I` indices per facet under the v1 policy;
- distinct indices with equal coordinate bit patterns;
- empty meshes;
- deterministic construction without dependence on container address; and
- readback sufficient to reconstruct vertex and facet sequences for verification.

The public mesh representation need not store halfedge pairing, occurrence identities, or provenance internally. Those facts remain in the success/candidate reports and must be reconstructible from the facet index data.

If the existing `fv_surface_mesh<T, I>` API has weaker exception, allocation, or validation semantics, the adapter must provide the required transactional wrapper in-tree. The component must not change correctness requirements to match a convenient container API.

### 1.4 Output scalar contract

The public coordinate scalar type is the same supported `T` used by the Boolean invocation unless a future output policy explicitly defines a checked conversion.

For v1:

- each public coordinate must be the authoritative Component 13 nominal `T` value;
- no decimal or text round-trip may occur;
- no averaging, re-quantization, snapping, normalization, or coordinate deduplication is permitted;
- signed zero must be preserved or canonicalized exactly according to the frozen Component 03/output policy;
- non-finite output is prohibited; and
- copying into the public mesh must not reduce the published precision below the coordinate's ledger entry.

### 1.5 Output index contract

The public index type `I` must be an accepted unsigned index type.

Before allocating the public mesh, the component must prove with overflow-safe arithmetic that:

- the number of public vertices is representable in `I` under the mesh's indexing convention;
- every final vertex index is representable;
- the number of facets and facet-index entries is representable in all internal and public count types;
- all prefix sums, component offsets, and `3 * face_count` calculations are checked;
- public container sizes are representable by `std::size_t` and configured resource counters;
- internal-to-public and public-to-internal maps are representable; and
- no sentinel value reserved by the public mesh collides with a valid index.

Capacity failure returns `index_overflow` or `resource_limit` before partial public construction is committed.

### 1.6 Canonicalization input requirements

Every cleaned component, vertex occurrence, paired edge, and triangle must provide a stable structural key or enough immutable content to derive one without transient IDs.

Canonicalization inputs may include:

- coordinate bit patterns under the frozen signed-zero rule;
- exact adjacency and oriented incidence;
- component and vertex-link topology;
- triangle orientation;
- normalized source/event/retained-use/cleanup provenance keys;
- topology-separation and multiplicity classes;
- cleanup action lineage; and
- predecessor canonical digests.

Transient allocation IDs, pointer values, mutable slot numbers, candidate discovery order, and worker timing are not canonicalization inputs.

Provenance used as a tie-break must itself be canonical under the input permutation and operand-remapping contracts. The public mesh byte sequence must not depend on caller facet order merely because diagnostics preserve that original order separately.

### 1.7 Capacity and lifetime preconditions

Before assembly begins, the component must validate that it can account for:

- connected-component discovery and signatures;
- canonical-label refinement and tie-resolution work;
- canonical vertex and triangle permutations;
- coordinate and facet-index arrays;
- internal/public reverse maps;
- public mesh builder storage;
- topology, geometry, cleanup, provenance, and precision reports;
- canonical logical serialization bytes or streaming state;
- digest state;
- structural round-trip reconstruction;
- diagnostics and replay data; and
- worst-case work up to configured component, vertex, face, refinement, serialization, and byte limits.

The assembled candidate and all report references must remain immutable and valid through Component 15 and ordinary success publication.

## 2. Required behavior

### 2.1 Assembly-stage transaction

The component must execute in a private stage transaction.

The transaction must contain:

- canonical labeling workspace;
- checked index maps;
- private public-mesh builder storage;
- report builders;
- logical serialization and digest state;
- structural readback/round-trip workspace; and
- proposed immutable candidate storage.

No caller-visible mesh, partial vertex array, partial facet list, report, or digest may be published before all assembly checks pass. Failure or cancellation rolls back the complete candidate.

### 2.2 Cleaned-manifold structural intake audit

Before canonicalization, the component must independently audit the cleaned internal manifold.

At minimum, it must verify:

- all referenced vertex, halfedge, edge, and triangle IDs are in range;
- all coordinates are finite;
- every triangle has three distinct vertex occurrences;
- every triangle halfedge cycle closes;
- every pair reference is reciprocal with reversed endpoints;
- every undirected edge has exactly two triangle uses;
- every vertex link is one closed cycle;
- triangle orientation metadata is accepted and consistent;
- no cleanup obligation remains;
- cleanup and topology-change reports cover every changed entity; and
- precision-ledger aggregation includes every output coordinate lineage.

This audit is not a substitute for Component 15, but it prevents serialization of an internally contradictory artifact.

### 2.3 Connected-component reconstruction

The component must reconstruct connected components from exact triangle/edge topology rather than trust only predecessor component labels.

Component reconstruction must:

- traverse adjacency through reciprocal paired edges;
- include every triangle exactly once;
- identify every vertex and edge belonging to each component;
- preserve topologically separate components whose coordinates touch or coincide;
- produce a canonical member-set digest; and
- compare reconstructed components with Component 13 topology-change reports.

An empty manifold has zero components. A component with no triangle is invalid and must not be serialized as isolated vertices or edges.

### 2.4 Canonical content model

Canonical ordering must be based on a logical content model of the cleaned oriented triangle manifold.

The content model must distinguish:

- topological vertex occurrences even when coordinates match;
- oriented triangle incidence;
- connected components;
- component-local graph structure;
- duplicate geometric components that are topologically separate;
- cleanup-generated vertex splits or merges where they affect topology; and
- policy-versioned coordinate bit patterns.

It must ignore:

- transient internal IDs;
- container insertion order;
- free-list history;
- cleanup candidate discovery order;
- parallel task order;
- hash-table bucket order; and
- pointer addresses.

The canonical content model may exclude detailed provenance from the public mesh content digest if the digest contract defines a separate artifact/provenance digest. The choice must be versioned and explicit.

### 2.5 Canonical labeling requirements

The component must assign a canonical public label to every vertex occurrence and triangle.

A provider may use deterministic partition refinement, canonical traversal with complete structural tie resolution, lexicographic minimization, in-tree graph canonicalization, or another algorithm. Regardless of provider:

- the result must be a total labeling of every cleaned component;
- isomorphic presentations with the same oriented coordinates and topology must produce the same logical mesh bytes under the frozen policy;
- exact automorphisms must be resolved without transient IDs;
- coordinate-equal but topology-distinct vertices must receive separate labels;
- disconnected duplicate components must both be retained;
- ties unresolved by local signatures must use deeper structural refinement or a deterministic canonical-minimum encoding;
- resource/work limits must be enforced; and
- failure to finish canonical labeling must return `resource_limit`, not fall back to input order.

The specification does not require a particular asymptotic graph-canonicalization algorithm. It requires a deterministic correct result for the supported resource domain.

### 2.6 Initial vertex and triangle descriptors

Canonical refinement should begin from immutable descriptors that may include:

For vertices:

- coordinate bit patterns;
- bounded precision category or ledger digest where included by policy;
- valence;
- cyclic sequence of incident oriented edge/triangle role descriptors;
- component-local topology signatures;
- occurrence-separation/multiplicity class; and
- normalized provenance signature when enabled.

For triangles:

- ordered or cyclic coordinate/vertex descriptors preserving orientation;
- adjacent triangle/edge role descriptors;
- source/carrier/internal-diagonal ancestry summary;
- cleanup ancestry summary; and
- component signature.

Descriptors are refinement inputs, not proof that local equality implies entity identity. Equal descriptors may represent several distinct vertices or faces and must remain separate until canonical labeling assigns positions.

### 2.7 Exact automorphism handling

A geometrically and topologically symmetric component may retain nontrivial automorphisms after ordinary refinement.

The component must handle this explicitly. Permitted strategies include:

- deterministic branching over unresolved cells with lexicographic pruning;
- canonical cycle/traversal enumeration from every tied seed within configured work limits;
- component-specific canonical encodings proven complete; or
- another in-tree canonical-minimum provider.

Choosing the smallest transient ID in an unresolved cell is prohibited.

If several disconnected component blocks have identical canonical logical bytes, their relative order does not affect the concatenated public mesh content provided vertex offsets and facet blocks are assigned consistently. The implementation must still produce deterministic internal/public reverse maps using canonical occurrence keys and occurrence multiplicity ranks that do not depend on discovery order.

### 2.8 Canonical component ordering

Each connected component must receive a canonical component encoding and signature.

Components must be ordered by a total key that may include:

- canonical logical component bytes or digest plus collision-resolving bytes;
- canonical coordinate bounds encoded by exact `T` bits;
- vertex, edge, and triangle counts;
- oriented topology signature;
- cleanup/topology-change class where included by output policy; and
- normalized provenance signature for diagnostic ordering.

Digest equality alone is insufficient to resolve order unless full canonical bytes are compared on collision.

Coordinate proximity, bounding-box center order using nondefinite floating comparisons, and source operand priority are prohibited component-order rules.

### 2.9 Canonical vertex ordering

Within the globally ordered component sequence, public vertex indices must follow the component's canonical labeling.

The mapping must:

- assign one public index to every cleaned vertex occurrence;
- preserve separate indices for all topology-distinct occurrences;
- assign contiguous index ranges according to the selected output policy;
- use checked conversion to `I`;
- publish internal-to-public and public-to-internal maps;
- preserve the authoritative coordinate bit pattern; and
- be reproducible from the canonical content model.

The component must not sort vertices by coordinates alone. Coordinate-only sorting is ambiguous for duplicates and can make topology depend on unstable tie handling.

### 2.10 Canonical triangle orientation and rotation

Every public facet must preserve the cleaned triangle's outward orientation.

After public vertex labels are known, the component may rotate the three indices cyclically to choose a canonical starting corner. It must not reverse their order.

For an oriented index triple `(i0, i1, i2)`, the canonical rotation is the smallest permitted cyclic rotation under the versioned total key:

```text
(i0, i1, i2)
(i1, i2, i0)
(i2, i0, i1)
```

The reversed triples are not candidates.

Triangle descriptors must retain the original cleaned triangle ID and complete provenance even after canonical rotation.

### 2.11 Canonical triangle ordering

Public facets must be ordered by a total canonical key after vertex labels and oriented cyclic rotations are fixed.

The key must include at least:

- canonical component position;
- rotated public index triple;
- any additional structural tie key required to distinguish permitted duplicate geometric triangles in separate occurrence sheets; and
- a collision-resolving canonical provenance or occurrence rank when two topologically distinct face records otherwise have equal public triples in a versioned representation that permits them.

In an ordinary valid indexed two-manifold, two distinct triangles in one component should not have the same oriented public index triple. If they do, the component must verify whether this is a permitted multiplicity representation or an invariant violation.

Facet order must not depend on source facet order, cleanup action order, or internal triangle ID.

### 2.12 Coordinate copying and signed-zero policy

Coordinates must be copied from Component 13 bounded points using the frozen output scalar policy.

For each public vertex:

- copy exactly three nominal `T` values;
- defensively verify finiteness;
- apply the frozen signed-zero preservation/canonicalization rule once;
- record whether any bit pattern changed due solely to an authorized signed-zero rule;
- retain the original bounded precision and cleanup lineage in reports; and
- update the serialization ledger for exact output bits.

No coordinate normalization, deduplication, snapping, decimal formatting, or unit conversion is permitted.

If applying the signed-zero rule would invalidate a bit-pattern determinism or lineage contract, the stage must fail rather than use inconsistent per-coordinate behavior.

### 2.13 Public facet construction

For every canonically ordered cleaned triangle, the component must append one triangular public facet containing the three checked public vertex indices.

The builder must ensure:

- exactly three indices are written;
- all indices are less than public vertex count;
- the three indices are distinct;
- facet orientation matches the cleaned triangle;
- no facet is omitted or duplicated;
- empty output uses the public mesh's valid empty representation; and
- any public-container allocation or insertion failure rolls back the candidate.

The v1 output policy must not merge coplanar adjacent triangles into polygons. Such merging could alter source/carrier feature representation and requires a later proof-preserving component with its own specification.

### 2.14 Public-mesh reverse topology reconstruction

After the public mesh is built, the component must reconstruct its indexed topology independently from the public coordinate and facet arrays.

The reconstruction must verify:

- every facet index is valid;
- every facet has three distinct indices;
- every directed edge has exactly one reverse directed edge;
- every undirected edge has exactly two uses;
- reciprocal endpoint pairs match the cleaned internal edge map;
- every public vertex link forms one closed cycle;
- connected-component membership matches the cleaned manifold;
- topologically separate coordinate-equal vertices remain separate indices; and
- the public facet set is in bijection with cleaned triangles after canonical mapping.

The reconstruction must not rely on Component 13 halfedge pair IDs as its sole source of truth. It must build edge-use maps from public indices.

### 2.15 Internal-to-public equivalence map

The candidate must contain a complete equivalence map between cleaned internal entities and public representation.

At minimum:

- each cleaned vertex occurrence maps to one public index;
- each public index maps to one cleaned vertex occurrence;
- each cleaned triangle maps to one public facet position;
- each public facet maps to one cleaned triangle;
- each cleaned paired edge maps to the two corresponding public directed facet-edge uses;
- each cleaned connected component maps to one canonical public component range/signature; and
- every mapping has a deterministic digest.

Removed Component 12 entities remain available through Component 13 provenance reports, not as public vertices or facets.

### 2.16 Precision aggregation

The component must assemble the candidate `output_precision` from Component 03/13 ledgers without shrinking any bound.

The aggregation must conservatively include:

- operand input precision inherited by every surviving lineage;
- machine roundoff floors;
- intersection and other construction uncertainty;
- cleanup coordinate construction uncertainty;
- cumulative cleanup displacement;
- feature/component removal bounds where the output policy incorporates them into result precision;
- signed-zero or output-copy effects, if any; and
- any public-container representation effect.

The aggregation rule must be versioned and independently verifiable.

The candidate must record:

- `output_precision`;
- `maximum_authorized_tolerance`;
- `maximum_realized_displacement`;
- `maximum_cumulative_lineage_displacement`;
- `maximum_removed_feature_size`;
- remaining tolerance margin; and
- the entity/action witnesses attaining each maximum.

Ordinary success is impossible if the applicable output precision or cleanup cost exceeds tolerance. Component 14 must fail early or mark the candidate invalid; it must not defer a known budget violation to Component 15.

### 2.17 Topology report assembly

The candidate topology report must summarize and reference evidence for:

- public vertex, edge, facet, component, and shell counts;
- exactly two uses per undirected edge;
- one closed link per public vertex;
- consistent orientation per component;
- component and genus information;
- preserved topology-distinct duplicate coordinates;
- cleanup-induced component/genus/cavity changes;
- empty-result status;
- public/internal bijection; and
- structural round-trip result.

Counts must be derived from the reconstructed public topology, not copied blindly from Component 13.

Before Component 15, the report status must indicate `assembled_pending_independent_verification` or an equivalent non-success state.

### 2.18 Geometry report assembly

The candidate geometry report must include:

- finite-coordinate audit;
- triangle orientation and area-bound summaries inherited from Component 13;
- precision and tolerance ledger summaries;
- maximum displacement and feature-removal witnesses;
- no-new-intersection evidence inherited from cleanup;
- public-coordinate bit-pattern audit;
- known remaining verification obligations for Component 15; and
- a status that is not yet `tolerance_checked`.

Component 14 must not mark geometry as finally validated. Only Component 15 may promote the report to the ordinary success status after independent final checks.

### 2.19 Cleanup and topology-change report assembly

The candidate must preserve the complete Component 13 action log or a policy-approved compressed representation with independently resolvable detail.

The public-facing cleanup/topology report must summarize:

- action count by class;
- vertices moved, merged, split, duplicated, or removed;
- edges/triangles removed or retriangulated;
- components removed;
- component/genus/cavity changes;
- displacement and feature-removal bounds;
- policy authorizations used; and
- replay references for full certificates.

Compression must not prevent Component 15 or diagnostics from recovering per-action evidence.

### 2.20 Provenance report assembly

Every public vertex and facet must have recoverable provenance.

Vertex provenance must support tracing to:

- one or more Component 13 cleaned lineages;
- Component 11 output occurrence requirements;
- source vertices or canonical intersection events;
- contributing operands, shells, facets, and retained uses;
- cleanup actions that moved, merged, split, or duplicated the occurrence; and
- bounded precision/displacement ledger entries.

Facet provenance must support tracing to:

- one Component 13 triangle;
- Component 12 triangles/patches;
- Component 11 face region and cycles;
- Component 10 retained surface uses and orientation decision;
- source operand, shell, facet, and caller feature identities; and
- cleanup retriangulation actions.

The report may use shared tables and ranges, but no public entity may have missing lineage.

### 2.21 Canonical logical serialization

The component must define a versioned logical byte encoding for deterministic comparison and digesting.

The encoding must specify:

- a fixed byte order;
- explicit integer widths or a canonical variable-length representation;
- exact `T` bit-pattern encoding;
- signed-zero policy;
- sequence lengths;
- component, vertex, and facet order;
- triangle index encoding;
- policy and artifact version fields;
- empty-mesh encoding;
- report/digest domain separation; and
- collision-resistant framing between fields and tables.

The logical encoding must not serialize native struct padding, pointer values, allocator capacity, host endianness, locale-sensitive text, unordered-container iteration, or implementation-defined `typeid` strings.

A provider may stream the encoding into a digest without retaining all bytes, but normative tests must be able to materialize the exact canonical byte sequence for bounded fixtures.

### 2.22 Digest domains

At minimum, the candidate must distinguish:

- **public mesh content digest**: canonical coordinate and facet content plus public serialization version;
- **topology/geometry artifact digest**: public content plus precision, cleanup, topology-change, and relevant policy metadata;
- **provenance digest**: normalized source and cleanup lineage tables;
- **replay digest**: exact input/context lineage and stage artifacts required to reproduce the run; and
- **candidate digest**: domain-separated aggregate committed to Component 15.

Digest equality is not a substitute for structural comparison where collision-free correctness is required. Collision tests and full-byte fallback comparisons are mandatory in normative testing.

No external cryptographic library may be required. The selected in-tree deterministic digest need not serve an adversarial security purpose unless a separate policy says so, but its algorithm and version must be frozen.

### 2.23 Determinism report

The candidate must record enough information to demonstrate that canonicalization did not depend on transient state.

The determinism report should include:

- canonical-labeling provider/version;
- refinement/branching statistics;
- unresolved automorphism cell counts and resolution method;
- component signatures;
- canonical permutation digests;
- logical serialization version and byte count;
- digest versions;
- thread/execution policy metadata that must not affect result; and
- a deterministic failure key if canonicalization approached a resource limit.

The report must not expose pointer values or nondeterministic internal addresses.

### 2.24 Empty-result assembly

An empty cleaned manifold must produce a valid empty public mesh candidate.

The candidate must contain:

- zero vertices and facets;
- valid empty internal/public maps;
- zero topology counts;
- output precision no smaller than applicable inherited/context precision under the frozen empty-result rule;
- cleanup/topology-change reports distinguishing originally empty from cleanup-removed-to-empty;
- canonical empty logical bytes and digests; and
- complete Component 15 verification obligations.

The component must not add a dummy vertex, zero-area facet, or sentinel component.

### 2.25 Round-trip re-ingestion check

The component must exercise the public mesh through the same in-tree read interface that a later Boolean import would use.

The round-trip check must verify:

- coordinate bit patterns are unchanged under readback;
- facet ring lengths and index sequences match canonical output;
- reconstructed edge pairing and vertex links match the candidate topology;
- duplicate-coordinate public indices remain distinct;
- connected components match;
- output precision metadata is retained in the surrounding result wrapper; and
- the mesh is structurally admissible for Component 02 re-ingestion under the published solid contract.

Component 15 owns the final independent re-ingestion validation, including geometric and shell-semantics checks. Component 14 must nevertheless reject an assembly that fails structural round-trip.

### 2.26 Candidate handoff to Component 15

The Component 14 output is a candidate, not ordinary success.

The handoff must provide Component 15 immutable access to:

- the public mesh;
- cleaned internal manifold;
- all internal/public equivalence maps;
- all predecessor artifacts required by verification policy;
- precision, cleanup, topology-change, and provenance ledgers;
- canonical logical bytes or a reproducible streaming encoder;
- all digests and report drafts;
- structural round-trip evidence;
- resource and execution statistics; and
- replay metadata.

Only after Component 15 independently accepts the candidate may the result wrapper set `geometry.status == tolerance_checked`, finalize the success digest, and expose ordinary `bounded_boolean_success`.

If Component 15 rejects the candidate, Component 14's public mesh remains an internal diagnostic artifact according to policy and must not be returned through the ordinary success type.

### 2.27 Deterministic parallel assembly

Parallel work may compute component signatures, local refinement descriptors, provenance blocks, or report summaries. Final publication must:

- merge all records by full canonical keys;
- resolve automorphisms under the frozen canonical-labeling provider;
- assign public indices in canonical order;
- write coordinates and facets in deterministic sequence;
- combine precision and report maxima with deterministic witness tie rules;
- serialize logical bytes in prescribed order;
- choose the same primary failure under every schedule; and
- commit only after round-trip checks.

Floating reductions used for non-authoritative statistics must have deterministic order or must not affect candidate bytes, status, or digest.

### 2.28 Resource limits and canonicalization complexity

The component must account separately for:

- component reconstruction;
- refinement descriptors and partitions;
- automorphism branching states;
- canonical encodings and comparisons;
- vertex/facet permutations and reverse maps;
- public mesh arrays;
- report and provenance tables;
- logical serialization bytes or streaming work;
- digest state;
- round-trip topology maps;
- diagnostics and replay storage; and
- persistent candidate bytes.

Highly symmetric meshes may make canonical labeling expensive. The component must enforce configured work limits and fail with `resource_limit` rather than use transient IDs, source order, or a nondeterministic fallback.

### 2.29 Cancellation and transactionality

Cancellation must be polled at deterministic safe points during component reconstruction, canonical refinement, automorphism branching, permutation construction, checked allocation, coordinate/facet writing, report assembly, logical serialization, digesting, and round-trip verification.

On cancellation, all workers must join, reservations must return, and no partial public mesh or candidate report may be visible. The result is `cancelled`.

### 2.30 Independent assembly verification evidence

The component must publish enough evidence for a separately implemented verifier to check:

- connected components reconstructed from public facets;
- canonical labeling inputs and final permutations;
- internal/public vertex and facet bijections;
- exact coordinate bit copying;
- facet orientation and cyclic rotation;
- checked `I` conversions;
- edge-use pairing and vertex links reconstructed from public indices;
- duplicate-coordinate preservation;
- precision and maximum-witness aggregation;
- cleanup/topology/provenance report coverage;
- logical serialization bytes;
- all digest domains; and
- structural round-trip equivalence.

The verifier must not call the producer's canonical-labeling or map-building helper as its sole source of truth. For bounded fixtures, it must compare against exhaustive permutations or an independently implemented canonical encoding oracle where feasible.

## 3. Output contract

On success, the component must produce one immutable `assembled_output_candidate<T, I>` artifact containing or referencing:

- artifact, public-mesh, canonical-labeling, report, logical-serialization, digest, and replay versions;
- one fully populated canonical `fv_surface_mesh<T, I>`;
- canonical public vertex and facet order;
- complete cleaned-internal-to-public and public-to-cleaned maps;
- cleaned-edge-to-public-directed-use mappings;
- component ranges and canonical signatures;
- exact public coordinate bit records;
- candidate output precision and tolerance-use summaries;
- topology report with `assembled_pending_independent_verification` status;
- geometry report with nonfinal verification status;
- cleanup and topology-change reports;
- provenance report covering every public vertex and facet;
- determinism and resource reports;
- canonical logical serialization or reproducible encoder state;
- public-content, artifact, provenance, replay, and aggregate candidate digests;
- structural round-trip reconstruction and equivalence evidence; and
- complete immutable handoff references for Component 15.

The artifact must guarantee:

- public vertex count equals cleaned vertex-occurrence count;
- public facet count equals cleaned triangle count;
- every cleaned vertex occurrence maps bijectively to one public index;
- every cleaned triangle maps bijectively to one public facet;
- topology-distinct coordinate-equal vertices remain separate public indices;
- every public index is valid and representable in `I`;
- every public facet is triangular, has three distinct indices, and preserves outward orientation;
- public edge-use and vertex-link topology reconstructs exactly the cleaned manifold;
- no coordinate was moved, rounded through text, snapped, or deduplicated during assembly;
- output precision and cleanup summaries do not understate any contributor;
- canonical ordering is independent of input permutations, transient IDs, cleanup history, and worker schedule under the frozen contracts;
- logical serialization and digests are reproducible on every qualified platform;
- structural round-trip succeeds; and
- ordinary success remains unavailable until Component 15 accepts the candidate.

On failure, no assembled candidate is published. The typed error must identify the component/vertex/triangle or canonicalization cell, count/conversion, coordinate bit witness, map or topology mismatch, precision/report contributor, serialization field, resource counters, policy versions, and deterministic replay payload relevant to the failure.

## 4. Required invariants and prohibited behavior

Required invariants:

- assembly is a bijective representation change from cleaned internal vertices/triangles to public vertices/facets;
- duplicate coordinates remain duplicate public entries whenever topology distinguishes them;
- no cleaned entity is omitted or duplicated;
- all public indices and count arithmetic are overflow-checked before commit;
- triangle orientation is preserved; only cyclic rotation is permitted;
- connected components and vertex links reconstruct from public indices;
- canonical labels do not depend on transient identities or schedules;
- exact automorphisms are resolved by a complete deterministic rule;
- coordinate copying preserves the frozen `T` bit policy;
- output precision is monotonic and tolerance usage is fully reported;
- logical serialization excludes native padding and implementation-defined state;
- the candidate remains pending Component 15 verification;
- all artifacts are immutable, context-owned, transactional, deterministic, and independently verifiable; and
- canonicalization/resource difficulty causes typed failure rather than a noncanonical fallback.

Prohibited behavior:

- deduplicating public vertices by coordinate equality or tolerance;
- sorting vertices by coordinates alone;
- ordering components from source operand priority or caller indices;
- breaking canonical-label ties with transient internal IDs;
- reversing triangle orientation to obtain a lexicographically smaller triple;
- merging adjacent coplanar triangles into polygons in v1;
- skipping triangles or vertices that appear redundant;
- converting coordinates through text or lower precision;
- under-reporting precision, displacement, or removed feature size;
- treating digest equality as the only structural equivalence proof;
- serializing native structs, padding, pointers, allocator state, or unordered iteration;
- declaring `tolerance_checked` before Component 15;
- exposing a rejected candidate through ordinary success;
- assigning output based on worker completion order;
- publishing partial mesh/report data after cancellation or resource exhaustion; or
- calling an external graph-canonicalization, mesh, serialization, hashing, or geometry library.

## 5. Test and validation specification

### 5.1 Basic assembly matrix

Assemble known cleaned manifolds for:

- empty output;
- one tetrahedron;
- one triangulated box;
- several disconnected components;
- nested outer/cavity/island shells;
- point-touching components with duplicate coordinate entries;
- edge-touching components with topology-distinct edge endpoints;
- coincident but separate shell occurrences;
- meshes containing cleanup-generated vertex splits; and
- outputs whose every coordinate is duplicated by another occurrence.

Verify exact public counts, coordinate bits, facets, orientation, component reconstruction, vertex links, and maps.

### 5.2 Type matrix

Test at least:

- `float` with `std::uint32_t`;
- `float` with `std::uint64_t`;
- `double` with `std::uint32_t`; and
- `double` with `std::uint64_t`.

Include signed zero, subnormal values, adjacent floats, extreme finite exponents, large translations, and mixed magnitudes. Verify exact bit copying and logical serialization.

### 5.3 Index-capacity boundary tests

Using test adapters or constrained index policies, test:

- zero vertices/faces;
- maximum valid vertex index;
- vertex count exactly representable;
- vertex count one above representable;
- face and facet-index counts near `std::size_t` and resource limits;
- checked component offsets;
- checked `3 * face_count` multiplication;
- reserved sentinel collisions; and
- rollback after allocation succeeds but a later checked conversion fails.

No partial public mesh may be visible.

### 5.4 Duplicate-coordinate preservation tests

Include:

- two public vertices with equal coordinates in one component but separate local fans;
- two point-touching components sharing one coordinate;
- two edge-touching components sharing both endpoint coordinates;
- several coincident components with identical geometry;
- a source vertex and event with equal nominal coordinates;
- cleanup-created duplicate occurrences; and
- signed-zero variants with policy-controlled canonicalization.

Verify public indices remain distinct and reconstructed vertex links/components are correct.

### 5.5 Canonical labeling known-answer tests

Commit exact canonical public vertex/facet sequences for:

- asymmetric components;
- cyclic and dihedral symmetric components;
- regular tetrahedral and box-like symmetries;
- repeated identical disconnected components;
- components with repeated coordinates;
- components distinguished only by topology;
- components distinguished only by coordinate bits;
- automorphism cells surviving several refinement rounds; and
- hash/signature collision injection.

For bounded fixtures, compare against exhaustive permutation minimization implemented in-tree for tests.

### 5.6 Input and internal permutation tests

Permute:

- source vertices, facets, rings, shells, and components;
- source triangle order and legal source triangulation;
- internal Component 11/12/13 entity IDs;
- cleanup free-list and allocation histories;
- cleanup action discovery order where final cleaned artifact is equivalent;
- connected-component discovery starts;
- vertex-link traversal starts;
- triangle cycles; and
- report/provenance table insertion order.

For a fixed provider/policy version and equivalent cleaned artifact, public mesh bytes, canonical logical bytes, maps modulo transient source handles, reports, and digests must be byte-identical after documented remapping.

### 5.7 Triangle orientation and rotation tests

For every triangle permutation:

- verify cyclic rotations canonicalize to one triple;
- verify reversed orientation is not accepted as a canonical rotation;
- inject one reversed cleaned triangle and confirm intake failure;
- verify paired edge directions reconstruct correctly after facet sorting; and
- verify triangle provenance remains attached after rotation/order changes.

### 5.8 Public topology reconstruction tests

Independently reconstruct from public facets:

- directed edge uses;
- reciprocal pairs;
- undirected edge counts;
- vertex links;
- connected components;
- Euler/genus summaries; and
- bijection to cleaned entities.

Inject corrected-count mutations with wrong endpoints, one duplicate facet, one missing facet, a bow-tie public vertex, and a three-use edge. Reconstruction must reject them.

### 5.9 Precision and report aggregation tests

Construct ledgers where maxima arise from:

- source input precision;
- event construction uncertainty;
- cleanup coordinate construction;
- one-step displacement;
- cumulative multi-step displacement;
- removed feature size;
- component removal; and
- output signed-zero/copy policy.

Test equal maxima with deterministic witness tie rules, values just below/at/above tolerance, and deliberately missing contributors. Independent aggregation must detect under-reporting.

### 5.10 Logical serialization tests

Commit canonical bytes for representative empty and non-empty candidates.

Test:

- fixed byte order;
- integer width/varint boundaries;
- exact scalar bit encodings;
- signed-zero policy;
- sequence framing;
- domain separation;
- report inclusion/exclusion policy;
- simulated big- and little-endian hosts;
- native struct padding differences; and
- locale changes.

Qualified platforms must produce identical logical bytes and digests.

### 5.11 Digest collision and structural fallback tests

Use test-only digest truncation or injected collisions to prove:

- component ordering compares full canonical bytes on digest collision;
- candidate equality checks structural content where required;
- provenance/artifact/public-content domains remain separated;
- a forged digest with changed mesh content is rejected; and
- canonicalization never treats digest equality as entity identity.

### 5.12 Round-trip tests

For every known-answer candidate:

- construct the public mesh;
- read it back through the in-tree adapter;
- compare coordinate bits and facet index sequences;
- reconstruct topology;
- verify duplicate-coordinate indices remain distinct;
- compare connected components and vertex links;
- attach published precision metadata; and
- pass the structural portion of Component 02 input validation.

Inject adapters that reorder, deduplicate, narrow, or canonicalize coordinates unexpectedly. The assembly stage must detect and reject them.

### 5.13 Mutation tests

Corrupt valid candidates by:

- welding two duplicate-coordinate vertices;
- splitting one public vertex without updating facets;
- changing one coordinate bit;
- narrowing one coordinate through text conversion;
- changing one public index;
- reversing one facet;
- deleting or duplicating one facet;
- scrambling component order;
- breaking canonical vertex order;
- using transient IDs to resolve one automorphism;
- under-reporting output precision;
- omitting one cleanup action from reports;
- changing logical serialization framing;
- forging one digest;
- marking geometry `tolerance_checked`; and
- changing internal/public maps without changing counts.

Independent verification must reject every mutation.

### 5.14 Determinism and concurrency tests

Run with:

- thread counts 1, 2, and maximum;
- forced task delays;
- reversed component discovery;
- reversed refinement queue order;
- different allocator/freelist layouts;
- hash-collision injection;
- automorphism branch permutations;
- different report block generation order; and
- repeated execution.

Public mesh content, logical bytes, all digest domains, candidate reports, selected failure, and replay metadata must be byte-identical for a fixed qualified platform/policy version.

### 5.15 Fuzzing and shrinking

Generate valid cleaned manifolds with controlled:

- component count;
- topology/genus;
- symmetry group size;
- coordinate duplication;
- equal disconnected components;
- vertex valence;
- triangle count;
- cleanup provenance complexity;
- index capacity; and
- coordinate bit patterns.

Compare bounded canonicalization cases against exhaustive permutation oracles. Every crash, noncanonical output, accidental deduplication, topology mismatch, digest inconsistency, or verifier disagreement must serialize and shrink while preserving the failure.

### 5.16 Performance and structural gates

Measure and assert structural counters for:

- component traversal;
- refinement rounds;
- unresolved cells;
- automorphism branches and prunes;
- canonical byte comparisons;
- coordinate/facet writes;
- topology reconstruction entries;
- report/provenance bytes;
- logical serialization bytes; and
- abstract work units.

Ordinary asymmetric meshes should canonicalize near linearly or according to the selected provider's documented target. Symmetric adversarial cases may hit configured limits and fail, but must not fall back to transient order.

### 5.17 Resource and cancellation tests

For refinement states, automorphism branches, permutations, public arrays, maps, report tables, provenance bytes, logical serialization, digest work, topology reconstruction, temporary bytes, and persistent bytes, test limit-minus-one, limit, and limit-plus-one.

Cancel during intake audit, component reconstruction, refinement, automorphism resolution, map construction, public allocation, coordinate/facet writing, report assembly, serialization, digesting, and round-trip verification. Confirm all workers join, reservations return, and no partial candidate is visible.

### 5.18 Definition of done

Component 14 is complete only when:

- every cleaned vertex occurrence and triangle maps bijectively to one public vertex/facet;
- topology-distinct duplicate coordinates are preserved;
- all public indices and allocations are checked;
- public topology reconstructs exactly the cleaned manifold;
- triangle orientation is preserved;
- canonical component, vertex, and facet order is independent of transient identities and schedules;
- exact automorphisms are handled without fallback to source/internal order;
- coordinate bit copying and signed-zero policy are exact;
- output precision, cleanup, topology-change, and provenance reports are complete;
- logical serialization and digest domains are stable across qualified platforms;
- structural round-trip succeeds;
- the candidate remains pending Component 15 verification;
- independent mutation verification is effective;
- deterministic replay is byte-stable; and
- all production and normative-test code is strict portable C++17 with no external dependencies.
