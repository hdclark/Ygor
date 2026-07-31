# Component 12 implementation plan: certified output assembly and canonical publication

## 1. Scope and outcome

Implement the final transactional boundary between Component 11's certified finite-coordinate realization and the public Boolean result. The stage consumes one verified immutable `realized_boundary<T, I>`, its retained Component 10 selected-boundary provenance, and Component 1's frozen output policy. It emits one canonical `fv_surface_mesh<T, I>` without making any new geometric or Boolean decision.

Baseline policy `output_topology_policy::triangulated_v1_no_simplification` must:

- emit Component 11's certified oriented triangles exactly, with no simplification, polygon reconstruction, retriangulation, welding, cleanup, or orientation repair;
- assign exactly one public vertex index to every realization symbol used by an emitted triangle;
- omit realization symbols not used by emitted triangles;
- preserve every accepted coordinate raw bit pattern, including the sign of zero;
- preserve Component 11 triangle orientation, allowing only orientation-preserving cyclic rotation of each triangle;
- sort canonical components and faces before assigning public vertex indices;
- assign vertex indices by first canonical face use;
- emit empty `vertex_normals` and `vertex_colours`;
- emit a complete deterministic `involved_faces` index;
- emit empty mesh `metadata`;
- retain provenance, policy, diagnostics, statistics, and certificates outside the public mesh in the enclosing `boolean_result<T, I>`;
- independently parse and verify the completed public vectors before publication.

The completed component must provide:

- exact index-capacity checks with specified maximum-boundary semantics;
- owner-free and schedule-stable component, face, ring, and vertex ordering;
- a canonical public-vector encoding independent of XML/OFF/OBJ behavior;
- final structural, topological, coordinate-bit, orientation, embedding-certificate, provenance, and dependency verification;
- successful empty output for an empty realized boundary;
- precise distinction among `index_overflow`, `resource_limit`, and `internal_invariant_error`;
- atomic publication of the public result only after mandatory independent verification.

Component 12 does not evaluate exact constructions, retry realization, alter coordinate bits, merge or split symbols, select Boolean patches, classify cells, triangulate new domains, infer outward orientation, repair topology, or use tolerance-based geometry. It must never return `output_not_representable`: Component 11 either published a certified realization or failed before Component 12 began.

All implementation must be portable C++17, self-contained in Ygor, and free of external dependencies. Explicitly do not include, call, adapt, test against, or otherwise rely on `src/YgorMeshesBoolean{,2,3,4,5}.{h,cc}`.

## 2. Existing Ygor assessment and reuse boundary

### 2.1 Current production surface

The only production public mesh container is `fv_surface_mesh<T, I>` in `src/YgorMath.h` and `src/YgorMath.cc`. Its relevant public storage is:

```cpp
std::vector<vec3<T>> vertices;
std::vector<vec3<T>> vertex_normals;
std::vector<std::uint32_t> vertex_colours;
std::vector<std::vector<I>> faces;
std::vector<std::vector<I>> involved_faces;
std::map<std::string, std::string> metadata;
```

No current production type provides the planned Components 1-11 contracts, artifacts, certificates, canonical IDs, exact kernel, selected boundary, or realized boundary. Those components are prerequisites, not existing implementation that Component 12 can assume is already available.

### 2.2 Reuse and adapt

- Use `fv_surface_mesh<T, I>` only as the final public value container.
- Use `vec3<T>` only to transport the bit-preserving values already accepted by Component 11. Do not use its arithmetic, equality, ordering, normalization, or geometric helpers for canonicalization or certification.
- Adapt the first-use compaction structure from `fv_surface_mesh<T, I>::remove_disconnected_vertices`: scan faces in a fixed order, create an old-to-new mapping at first reference, append each used vertex once, and rewrite face indices. Do not call the function itself because Component 12 must perform capacity checks, preserve symbol mappings, retain certificates, and verify exact canonical order while assembling.
- Adapt the undirected-edge aggregation and opposite-directed-use checks from `src/YgorMeshesVerification.{h,cc}` as structural patterns only. The final verifier must add complete range, degeneracy, duplicate-use, vertex-link, realization mapping, and certificate checks.
- Adapt adjacency construction and deterministic connected-component traversal concepts from `src/YgorMeshesOrient.cc`. Do not use its tolerance-based representative map, bounding-box orientation heuristic, normal computations, face flipping, or repair behavior.
- Use Component 1's context, strong IDs, owner checks, checked cardinality arithmetic, resource accounting, deterministic execution facilities, canonical encoders, cancellation, diagnostics, replay, verifier registry, artifact transactions, and typed failures.
- Consume Component 10's selected patch/edge/vertex identities, result-side orientation, provenance, and selection certificate only through Component 11's retained immutable dependencies.
- Consume Component 11's realization vertices, raw coordinate bits, transport values, oriented `triangulated_v1` topology, selected/artificial edge roles, obligations, witnesses, and accepted realization certificate.

### 2.3 Non-authoritative facilities

Existing XML, OFF, and OBJ serialization is non-authoritative. These formats may omit fields, reorder data, print floating values textually, normalize values, impose format-specific conventions, or reconstruct `involved_faces`. They must not define canonical Boolean output, semantic digests, equality, replay records, or verification behavior.

Component 12 may add interoperability tests proving that supported writers can consume the public mesh, but a writer round trip is never a correctness oracle. `YGBCAN12` and `YGBOUT12` are the authoritative encodings.

### 2.4 Rejected production behavior

Do not use or invoke:

- `merge_duplicate_vertices`;
- `remove_degenerate_faces`;
- `convert_to_triangles`;
- `simplify_inner_triangles`;
- `remove_disconnected_vertices`;
- mesh remeshing or refinement facilities;
- orientation repair from `YgorMeshesOrient`;
- tolerance-based duplicate detection;
- normal-based or centroid-based orientation;
- XML/OFF/OBJ ordering or round trips as canonicalization;
- native floating-point `<`, `==`, `std::tuple<T, ...>` ordering, arithmetic, total-order extensions, or decimal formatting as a canonical key;
- epsilon, snapping, feature-size thresholds, area tolerances, randomization, or native hash iteration order;
- source insertion order, pointer values, owner tokens, worker completion order, or provenance attribution as geometric tie-breakers.

All existing tolerance cleanup, remeshing, simplification, and orientation-repair paths are prohibited. A failed invariant is an error, not an opportunity to modify the mesh.

## 3. Files, namespace, and integration

Add:

- `src/YgorMeshesBooleanOutput.h`: output-policy schemas, public-result schemas, canonical ordering descriptors, output mapping records, assembly certificate, immutable internal output artifact, and stage entry point.
- `src/YgorMeshesBooleanOutput.cc`: dependency audit, capacity checks, canonical component/face ordering, first-use reindexing, public mesh assembly, canonical encoding, coordinator, verifier adapter, and four `<T, I>` explicit instantiations.
- `tests/Test_MeshesBooleanOutput.cc`: focused assembly, indexing, public-field policy, failure, mutation, encoding, rollback, and re-ingestion tests.
- `tests/Test_MeshesBooleanOutputProperties.cc`: permutation, component-order, schedule, compiler, provenance-independence, and end-to-end property tests.
- `tests/MeshBooleanOutputFixtures.h`: synthetic realized boundaries, raw-bit coordinate builders, capacity-model fixtures, independent public-vector parser, independent topology verifier, mutation helpers, deterministic PRNG, and replay records.

Modify only as required to reconcile prerequisites:

- `src/YgorMeshesBooleanContract.{h,cc}`: add output policy/version fields, output IDs, limits, accounting categories, invariant codes, artifact/result framing, and typed public result support.
- `src/YgorMeshesBooleanRealization.h`: expose complete read-only realization vertex/triangle mappings, raw bits, edge roles, certificate bindings, and owner-free semantic keys. Do not add output indices to Component 11.
- `src/YgorMeshesBooleanSelection.h`: only reconcile retained read-only provenance needed for the final handoff.
- `src/CMakeLists.txt`: add the output source and strict compilation settings.
- `tests/CMakeLists.txt` and `tests/compile.sh`: add the two test targets.

Use namespace `ygor::mesh_boolean`. Keep provisional component records, face keys, symbol-to-output maps, verifier topology, and scratch encodings private to the `.cc` or tests.

Explicitly instantiate `float` and `double` with `std::uint32_t` and `std::uint64_t`. Register CTest targets `MeshBooleanOutput.Unit` and `MeshBooleanOutput.Properties`, labeled `mesh_boolean;component12`. Add authoritative GCC/Clang Debug/Release, ASan/UBSan malformed-vector/rollback, and TSan verification/publication runs without ignored failures.

## 4. Prerequisites and interface reconciliation

Production integration must not begin until Components 1-11 exist and their required contracts and verifiers pass. In particular:

1. Component 1 must provide `output_policy_v1`, output-stage accounting, checked cardinalities wider than `I`, canonical raw-bit encoders, typed failure precedence, artifact transactions, and a public `boolean_result<T, I>`.
2. Component 10 must provide stable selected entities and complete provenance without requiring source mesh pointers.
3. Component 11 must publish `realization_policy_v1` with `triangulated_v1`, one realization vertex per used symbolic identity, complete oriented triangles, raw coordinate bits, transport `vec3<T>`, selected/artificial edge roles, and a passed realization certificate.
4. Component 11 must expose an owner-free semantic vertex key. It must identify the canonical mathematical symbol and be stable across equivalent execution schedules. It must exclude owner tokens, pointers, provisional IDs, worker IDs, source insertion order, and representative-only provenance.
5. Component 11 must expose an owner-free semantic triangle identity or enough immutable fields to derive it from the three semantic vertex keys, selected patch semantics, and certified triangulation role.
6. Component 13 must register a Component 12 verifier that receives the candidate output artifact, immutable Component 11 dependency, frozen policy, accounting, and cancellation without globals.
7. Resource policy must bound public vertices, faces, face-index entries, involved-face entries, component/adjacency records, mapping records, certificate facts, canonical bytes, verifier scratch, diagnostics, and replay data.
8. The public result wrapper must keep the internal output certificate and dependency digests distinct from `fv_surface_mesh::metadata`.

If the planned upstream interfaces do not provide owner-free semantic keys, improve those interfaces before implementing Component 12. Do not derive stable identity from serialized owner-bound handles.

## 5. Stage, ownership, and publication contract

Expose a coordinator equivalent to:

```cpp
template<class T, class I>
status_or<boolean_result<T, I>>
assemble_boolean_output(boolean_context<T, I>& context);
```

Internally, construct and verify an immutable artifact equivalent to:

```cpp
template<class T, class I>
struct assembled_output {
    fv_surface_mesh<T, I> mesh;
    std::vector<output_vertex_record<I>> vertices;
    std::vector<output_face_record<I>> faces;
    std::vector<output_component_record<I>> components;
    output_assembly_certificate certificate;
};
```

The coordinator must:

1. Require the exact published `realized_boundary<T, I>` from `artifact_slot::realized_boundary`, its retained selected-boundary dependency, the frozen output policy, and the registered verifier.
2. Reject stale, copied, replacement, wrong-generation, wrong-coordinate-type, wrong-index-type, wrong-triangulation-policy, owner-mismatched, or digest-mismatched dependencies.
3. Validate dense realization IDs, complete raw-bit/transport mappings, oriented triangle records, certificate status, and all prerequisite ranges before allocation.
4. Open one `output_assembly` transaction. No public mesh, output index, diagnostic, certificate, or resource charge becomes observable before verification succeeds.
5. Compute logical output cardinalities and classify index capacity before constructing public vectors.
6. Derive canonical component and face order without output indices.
7. Assign one output index per first-used realization symbol.
8. Assemble all public fields according to the exact baseline policy.
9. Encode the candidate, invoke the independent verifier, compare report and certificate bindings, check cancellation, and publish atomically.
10. Return the mesh by value or immutable ownership inside `boolean_result<T, I>`. The published result must not expose mutable references to the internal certified artifact.

The output artifact retains strong typed dependencies on Component 11, Component 10 provenance, frozen policy, and the accepted verification report. It contains no borrowed input mesh pointer, hash-table iteration state, provisional ordering, native floating comparison result, or mutable repair state.

## 6. Baseline public mesh policy

Freeze `output_policy_v1` with baseline mode `triangulated_v1_no_simplification`.

### 6.1 Faces

- Emit exactly one public face for every Component 11 realization triangle.
- Every face has exactly three indices.
- No realization triangle may be omitted, duplicated, split, merged, or replaced.
- The public triangle must use the same three realization symbols and the same orientation as Component 11.
- Canonical ring rotation may choose among `(a,b,c)`, `(b,c,a)`, and `(c,a,b)` only.
- `(a,c,b)` and its rotations are forbidden because they reverse orientation.
- Artificial triangulation edges remain represented by adjacent public triangles; they are not public edge records and are not removed.
- Empty realization emits no faces.

### 6.2 Vertices

- A realization symbol is used if and only if it appears in an emitted Component 11 triangle.
- Emit exactly one public vertex for each used realization symbol.
- Distinct realization symbols always receive distinct output indices.
- Equal coordinate bits must not cause a merge. Preserve distinct realization symbols as distinct output vertices; if Component 11's frozen schema instead prohibits equal accepted triples, diagnose such an artifact as an upstream invariant failure rather than relying on that prohibition for indexing.
- A realization symbol referenced by several triangles receives one shared output index.
- Unused realization records are not emitted. Their presence in a baseline Component 11 artifact should normally be an upstream invariant error; omission remains specified defensively.
- Copy each coordinate through bit-preserving construction and verify copied-back X/Y/Z bits against Component 11.
- Preserve `+0` and `-0` exactly. Component 12 has no signed-zero normalization mode in schema v1.

### 6.3 Normals and colours

`mesh.vertex_normals` and `mesh.vertex_colours` must both be empty.

Component 12 must not compute normals because:

- normals are not required to represent certified topology;
- native normalized arithmetic would not be authoritative;
- introducing them would create another deterministic floating-output policy;
- an empty vector is explicitly valid for `fv_surface_mesh`.

No default colours are emitted.

### 6.4 `involved_faces`

`mesh.involved_faces` must be complete, not empty:

- its size equals `mesh.vertices.size()`;
- entry `v` contains every public face index whose ring references `v`;
- each face appears exactly once in each incident vertex entry;
- because baseline faces have three distinct vertices, every face contributes exactly three entries;
- each entry is strictly increasing by public face index;
- no duplicate face index is permitted;
- no entry is empty because first-use assembly emits only used vertices.

Build it by scanning already canonical public faces from face index zero upward and appending the current checked face index to each of the three incident entries. This yields deterministic ascending order without a later sort.

Do not call `recreate_involved_face_index()` as the authoritative implementation. Its loop is a useful structural reference, but Component 12 must perform checked conversions and retain assembly mappings. The independent verifier must reconstruct this index from `vertices` and `faces` without trusting the stored vector.

### 6.5 Metadata and provenance

`mesh.metadata` must be empty in `triangulated_v1_no_simplification`.

Operation names, input digests, replay tokens, certificates, schema versions, source feature IDs, and provenance must not be inserted into the mesh metadata map. Text metadata is not a canonical typed schema and could make equivalent schedules differ through incidental attribution or formatting.

The enclosing `boolean_result<T, I>` carries:

- operation and policy versions;
- input and dependency semantic digests;
- canonical output digest;
- output assembly certificate or immutable reference to it;
- deterministic statistics;
- compact typed provenance mappings when requested by the frozen result policy;
- diagnostics and replay data.

Provenance does not participate in geometric component, face, ring, or vertex ordering. Representative-only provenance from coincident source sheets is informational and must never change public vectors or `YGBCAN12`.

## 7. Exact index-capacity semantics

### 7.1 Addressable cardinality

For unsigned index type `I`, let:

```text
Imax = numeric_limits<I>::max()
capacity(I) = Imax + 1
```

`capacity(I)` is a mathematical checked cardinality, not an `I` value. A nonempty collection of `N` indexed entities is representable exactly when its greatest required ordinal `N - 1` is no greater than `Imax`. Therefore:

- `N = 0` is representable;
- `N = Imax + 1` is index-representable because the greatest index is `Imax`;
- `N = Imax + 2` is not representable and returns `index_overflow`.

Never compute `Imax + 1` in type `I`. Use Component 1's checked cardinality type or compare `N - 1` after proving `N != 0`.

### 7.2 Required index domains

Baseline output uses `I` for:

- vertex indices stored in each face;
- face indices stored in `involved_faces`.

Consequently both used-vertex count and public-face count must satisfy the addressable-cardinality rule.

A ring length is represented by `std::vector<I>::size_type`, not by `I`. Baseline ring length is exactly three and requires no `I` count conversion. Every ring element must still be a checked `I` vertex index.

Component, mapping, certificate, and provenance counts are not public mesh indices unless their schema explicitly uses `I`; they use checked sizes or strong internal IDs and are governed by resource/schema limits.

### 7.3 Host-container and resource limits

Index representability is distinct from construction feasibility:

- Return `index_overflow` when a logically known required vertex or face ordinal exceeds `Imax`, even if resource limits would also prevent allocation.
- Return `resource_limit` when counts fit `I` but exceed caller-declared entity/byte/work limits, `size_t`, allocator, vector `max_size`, verifier budget, cancellation, or available allocation.
- Check logical index capacity before reserving public storage so a small `I` overflow is not obscured by allocation failure.
- For a theoretical `uint64_t` cardinality of `2^64`, indexing is mathematically valid but ordinary 64-bit `size_t` vectors cannot contain it; if such an upstream artifact could be described without materialization, assembly returns `resource_limit`, not `index_overflow`.
- Allocation failure after capacity acceptance is `resource_limit`.
- No limit failure may cause truncation, partial output, omission of `involved_faces`, or a smaller approximate result.

### 7.4 Conversion rules

Before each cast to `I`, prove the source ordinal is no greater than `Imax`. This applies independently to:

- newly assigned public vertex indices;
- public face indices appended to `involved_faces`;
- any `I` field in public mapping records.

Use checked conversion helpers. Raw `static_cast<I>` is permitted only inside a helper whose precondition has just been verified and tested.

## 8. Canonical ordering without circular keys

Canonicalization must sort faces before first-use vertex reindexing. Sorting faces by final public indices would be circular because those indices depend on face order. Therefore all pre-index ordering uses owner-free semantic tokens supplied or derivable from Component 11.

### 8.1 Owner-free semantic vertex token

For each used realization vertex, derive `vertex_token_v1` from:

- Component 11's owner-free canonical symbolic-vertex key;
- accepted X/Y/Z raw coordinate bits in fixed axis order;
- coordinate type/schema version.

Exclude:

- output index;
- realization store offset unless that offset is itself the canonical semantic rank committed by `YGBCAN11`;
- owner/setup tokens;
- pointers;
- source insertion order;
- worker or shard IDs;
- hash values without canonical collision resolution;
- representative-only or alternate provenance;
- native `T` comparisons;
- decimal coordinate text.

The symbolic key is the primary identity. Raw bits make the realized representation explicit but do not merge symbols. If two distinct symbols somehow produce identical complete tokens, fail as an upstream/internal invariant defect rather than add source provenance as an accidental tie-breaker.

All token bytes use Component 1's canonical integer and length encoding. Coordinate bits are compared as unsigned raw bit strings only after semantic key fields; this is a deterministic bit ordering, not native floating numeric ordering.

### 8.2 Oriented triangle ring key

For Component 11 triangle `(a,b,c)`, form the three orientation-preserving rotations:

```text
K0 = (token(a), token(b), token(c))
K1 = (token(b), token(c), token(a))
K2 = (token(c), token(a), token(b))
```

Choose the lexicographically least encoded tuple. Record the corresponding rotation. Do not consider reversed tuples.

Append owner-free semantic tie fields only after the oriented ring tuple:

- selected exact patch semantic key;
- Component 11 triangulation-role key;
- boundary/artificial edge-role tuple aligned with the chosen rotation.

These fields distinguish distinct certified triangle records only if their oriented vertex tuples are equal. Two triangles with the same oriented symbols and same semantic role are duplicates and must fail; source provenance is not a tie-breaker.

### 8.3 Face adjacency

Build a temporary map from owner-free undirected semantic edge key:

```text
(min(vertex_token(u), vertex_token(v)),
 max(vertex_token(u), vertex_token(v)))
```

to directed triangle uses. Require exactly two uses with opposite directions. This is a verification and component-discovery map, not a new topology decision.

Hash tables may accelerate lookup, but canonical output and first failure are determined by sorted semantic edge keys.

### 8.4 Connected-component key

Traverse triangle adjacency without changing orientation:

1. Select the least unvisited triangle ring key as a seed.
2. Visit adjacent triangles using neighbors sorted by `(shared semantic edge key, neighbor triangle ring key)`.
3. Collect the component's triangle keys.
4. Sort the collected triangle keys independently of traversal.
5. Define the component key as:
   - schema version;
   - triangle count;
   - sorted triangle-key sequence;
   - sorted semantic vertex-token sequence;
   - sorted semantic edge-key sequence.

This key is owner-free and does not depend on output indices, BFS queue timing, provenance, or storage order.

Sort components lexicographically by component key. Equal complete component keys indicate duplicate realized components and are an invariant failure.

### 8.5 Final face order

Within each component, sort faces by oriented triangle ring key and semantic tie fields. Concatenate components in component-key order.

This ordering occurs before public vertex assignment. It guarantees that disconnected components cannot interleave according to provisional realization storage and that first-use compaction is deterministic.

### 8.6 First-use vertex assignment

Scan canonical faces in final order. For each face, scan its chosen orientation-preserving ring rotation from position zero to two:

1. If the realization symbol has no public index, assign the next ordinal.
2. Append that symbol's bit-preserved `vec3<T>` to `mesh.vertices`.
3. Record the realization-symbol-to-output-index mapping.
4. Append the existing or newly assigned index to the face ring.

The mapping is by realization symbol identity, never by coordinate bits. At completion:

- every used symbol has exactly one index;
- every output vertex is used;
- index order is exactly first canonical face use;
- no later vertex sort is allowed.

## 9. Optional proof-backed simplification architecture

No simplification is enabled in baseline schema v1. `triangulated_v1_no_simplification` is mandatory for the initial implementation and release gate.

Future simplification must be introduced as a separately versioned output policy and artifact stage, not as hidden behavior in baseline assembly. The architecture may reserve:

```cpp
enum class output_topology_policy {
    triangulated_v1_no_simplification,
    proof_simplified_v2
};
```

Unknown or unimplemented policies fail at context setup or decode.

A future simplification proposal must be transactional and produce:

- the exact selected domains being replaced;
- the Component 11 triangles and obligations being retired;
- the proposed polygonal or simplified topology;
- an exact proof that the union of replacement domains equals the retired exact domains;
- proof that occupied side and orientation are unchanged;
- proof that every emitted polygon is exactly planar, simple, non-zero-area, and correctly oriented after realized coordinates are considered;
- proof that edge incidence, vertex links, connected components, genus, cavities, and embedding are preserved;
- a replacement realization-obligation set covering all new facets, edges, non-incidences, links, and radial orders;
- an independent verifier result for those replacement obligations;
- a versioned simplification certificate and canonical encoding.

Potential operations such as removing artificial degree-two subdivisions or merging coplanar adjacent triangles are permitted only through that proof protocol. “Coplanar” must be exact both for selected geometry and for the emitted `T` bit patterns where polygonal output requires it.

A non-applicable simplification candidate leaves topology unchanged. A malformed proof is `internal_invariant_error`; inability to complete required proof work is `resource_limit`. Simplification must never trigger a new coordinate search or return `output_not_representable`. If changed topology cannot be certified with Component 11's fixed coordinates, the candidate is not admissible.

## 10. Assembly algorithm

### 10.1 Dependency audit and cardinality preparation

1. Check cancellation and validate context, artifact slot, owner, generation, schema, coordinate type, index type, policy, dependency digests, verifier registration, and certificate bindings.
2. Require Component 11 policy `triangulated_v1` and output policy `triangulated_v1_no_simplification`.
3. Validate every realization vertex's finite raw bits and bit-identical transport `vec3<T>`.
4. Validate every realization triangle has three distinct realization symbols, a valid selected patch mapping, preserved orientation, valid edge roles, and a passing realization-certificate reference.
5. Count realization triangles and collect used realization symbols with checked arithmetic.
6. Check vertex and face index capacity according to Section 7 before public allocations.
7. Compute conservative resource envelopes for tokens, rotations, adjacency, components, mappings, public vectors, `involved_faces`, certificates, canonical bytes, and independent verifier scratch.
8. Open private accounting-backed storage only after all count checks pass.

An empty realized boundary must have no triangles and no used realization symbols. It proceeds to an empty successful mesh after dependency verification.

### 10.2 Canonical pre-index ordering

1. Construct one owner-free vertex token for each used realization symbol.
2. Require complete token uniqueness across distinct symbols.
3. Derive each oriented triangle's canonical cyclic rotation without reversal.
4. Build sorted semantic edge uses and require exactly two opposite uses per undirected edge.
5. Build triangle adjacency from those edge uses.
6. Discover connected components using deterministic sorted adjacency.
7. Compute each owner-free component descriptor.
8. Sort components by descriptor and faces within components by oriented ring key.
9. Require complete uniqueness of canonical triangle records.
10. Freeze the final face sequence before assigning any output index.

No output index may appear in a pre-index key or comparator.

### 10.3 Public-vector construction

1. Reserve exact checked sizes for `mesh.faces`, mapping records, and component ranges.
2. Scan final faces and assign first-use public vertex indices.
3. For each newly used symbol, append its bit-preserved coordinate and verify copied-back bits immediately.
4. Append each triangle as a three-element `std::vector<I>` using the chosen cyclic rotation.
5. Set `vertex_normals`, `vertex_colours`, and `metadata` to empty.
6. Allocate `involved_faces` with exactly one entry per output vertex.
7. Scan public faces in order and append the checked current face index to each referenced vertex entry.
8. Require each `involved_faces` entry to be nonempty and strictly increasing.
9. Freeze output vertex, face, component, and realization-mapping records.

Do not mutate the mesh after canonical encoding begins.

### 10.4 Certificate and publication

1. Recompute counts, component ranges, first-use positions, edge-use totals, and involved-face totals.
2. Bind every public vertex to exactly one Component 11 realization symbol and every public face to exactly one Component 11 realization triangle.
3. Record coordinate-bit equality, orientation-preserving rotation, component membership, edge incidence, and first-use facts.
4. Encode canonical semantic bytes and invocation-bound artifact bytes.
5. Invoke the mandatory independent verifier, which parses public vectors from scratch.
6. Compare verifier report, certificate, policy, and dependency bindings.
7. Perform a final cancellation check.
8. Commit accounting and atomically publish `boolean_result<T, I>`.

No public result is returned if any step fails.

## 11. Invariants and failure handling

Before publication, prove:

1. The realized dependency and every retained prerequisite match the frozen context.
2. Baseline output contains exactly the complete Component 11 triangle multiset, with no simplification or retriangulation.
3. Every public face has exactly three distinct in-range indices.
4. Every Component 11 triangle is represented by an orientation-preserving cyclic rotation only.
5. Every used realization symbol maps to exactly one public index and every public vertex maps to exactly one used realization symbol.
6. No output vertex is unused.
7. Every public coordinate has exactly the Component 11 X/Y/Z raw bits, including signed zero.
8. `vertex_normals`, `vertex_colours`, and `metadata` are empty.
9. `involved_faces` is complete, ascending, duplicate-free, and exactly reconstructible from faces.
10. Every undirected public edge has exactly two opposite directed face uses.
11. Every vertex link is one cycle per surface sheet and agrees with Component 11's certified link.
12. Public connected components and face adjacency are isomorphic to Component 11's triangulated complex.
13. No duplicate public face or duplicate realized component exists.
14. Component 11 orientation, permitted incidence, nonintersection, and embedding obligations remain applicable because coordinates and triangle connectivity are unchanged.
15. Component and face sorting is owner-free and precedes first-use vertex assignment.
16. Provenance and representative attribution do not affect public vectors.
17. Empty realization produces exactly six empty mesh fields and a passed empty certificate.
18. Canonical bytes and equivalent failures are independent of input storage permutation, hash behavior, allocation address, thread count, worker timing, native floating environment, and compiler optimization.

Failure classification:

- Return `result_topology_not_supported` during preflight when a valid selected exact boundary does not satisfy `result_topology_policy::closed_embedded_two_manifold`. Bind canonical topology obstructions and do not invoke Component 11.
- Return `index_overflow` only when a logically required public vertex or face ordinal cannot be represented by `I`.
- Return `resource_limit` for declared count/byte/work/verifier limits, `size_t` or container limits, cancellation, allocation failure, or inability to complete mandatory verification.
- Return `internal_invariant_error` for malformed/stale dependencies, failed Component 11 certificate binding, duplicate semantic tokens, invalid triangles, inconsistent orientation, open/non-manifold incidence, mapping disagreement, coordinate-bit changes, invalid public fields, verifier disagreement, or encoding corruption.
- Component 12 does not return `input_contract_error`; Component 2 owns input rejection.
- Component 12 does not return `output_not_representable`; Component 11 owns finite-coordinate representability.
- Component 12 must not convert an upstream failure into a different geometric result.

If both a known index overflow and a later potential resource failure exist, report `index_overflow` because logical capacity is checked first. If resource exhaustion prevents determining malformed semantics that require traversal, report `resource_limit`; do not speculate about an invariant result.

Check cancellation before dependency audit, cardinality analysis, token generation, edge grouping, each component frontier, sorting, first-use assembly, `involved_faces` construction, encoding, independent verification, and publication.

Diagnostics include policy/dependency digests, realization and selected IDs, semantic token digests, component/face/vertex/output indices, raw coordinate bits, expected/actual incidence, capacity facts, resource facts, provenance references, and replay token. Decimal floating text is supplementary only.

## 12. Canonical encodings and deterministic execution

### 12.1 Semantic encoding

Define `YGBCAN12` as the canonical public-output semantics for one Component 11 realization and output policy. Encode:

- schema, coordinate type, index type, topology policy, and ordering versions;
- Component 11 semantic digest and realization-policy digest;
- component descriptors in canonical order;
- public vertices in first-use order as X/Y/Z raw bits;
- public faces in canonical order as fixed-length three-index rings;
- realization-symbol-to-output-index semantic mappings;
- realization-triangle-to-public-face semantic mappings and cyclic-rotation values;
- complete `involved_faces`;
- explicit empty normals, colours, and metadata markers;
- deterministic certificate facts and semantic counts;
- combined semantic digest.

`YGBCAN12` excludes:

- pointers and owner tokens;
- setup or invocation ordinal;
- source mesh storage order;
- representative-only and alternate source provenance;
- diagnostics, traces, timings, allocator state, hash layout, worker IDs, and caches;
- native `vec3<T>` object bytes or padding;
- XML/OFF/OBJ serialization;
- decimal floating text.

Raw coordinate bits are authoritative. Signed zeros remain distinct.

### 12.2 Invocation-bound encoding

Define `YGBOUT12` with:

- schema/type versions;
- setup and output-policy digest;
- exact Component 10/11 dependency identities, generations, and digests;
- deterministic statistics;
- length-prefixed `YGBCAN12`;
- output certificate and verifier-report bindings;
- optional typed compact provenance payload under a separately versioned result policy.

Wrap the internal artifact with Component 1's `YGBART01` framing. The returned `boolean_result<T, I>` uses its own Component 1 framing and includes either exactly one success payload or exactly one typed failure payload.

Decode rejects unknown versions/enums, wrong types, bad lengths, trailing bytes, noncanonical booleans, malformed raw bits, non-dense mappings, invalid rotations, reversed faces, unsorted components/faces, non-first-use vertex order, out-of-range indices, malformed `involved_faces`, nonempty baseline normals/colours/metadata, inconsistent counts/digests, and dependency mismatches.

### 12.3 Deterministic execution

- Fallible semantic preparation occurs before standard sorting.
- Standard comparators inspect immutable canonical byte keys, IDs, integer ranks, and raw unsigned bits and are `noexcept`.
- No comparator performs exact arithmetic or native floating comparison.
- Hash tables are lookup accelerators only.
- Component and face ordering remains serial in schema v1 unless deterministic partitioning is proven.
- Verification may use private deterministic shards, merged by canonical component/face range.
- Canonical first failure follows Component 1's versioned failure-order policy, not task completion order.

Equivalent schedules over the same canonical Component 11 artifact must produce byte-identical `YGBCAN12`, public vectors, mappings, and certificate facts.

## 13. Mandatory independent verifier

Register a stable Component 12 artifact tag, schema/checker version, and invariant set with Components 1 and 13.

The verifier receives:

- the completed candidate `fv_surface_mesh<T, I>`;
- candidate output mappings and certificate;
- immutable Component 11 realized boundary;
- retained Component 10 selected boundary;
- frozen output policy;
- resource accounting and cancellation.

It must not call producer helpers for token construction, ring rotation, component traversal, face sorting, first-use assignment, `involved_faces` construction, certificate generation, or encoding.

The verifier must parse the public vectors from scratch:

1. Read `vertices`, `faces`, `vertex_normals`, `vertex_colours`, `involved_faces`, and `metadata` directly.
2. Require empty normals, colours, and metadata under baseline policy.
3. Validate every face length and every index before dereference.
4. Rebuild directed and undirected edge uses independently from public faces.
5. Require exactly two opposite uses for every undirected edge.
6. Rebuild face adjacency and every vertex link independently.
7. Recompute connected components without trusting producer ranges.
8. Rebuild `involved_faces` from faces and compare exact entry sizes, values, and ascending order.
9. Determine first occurrence of every public vertex in public face order and require output index `k` to be first introduced immediately after indices `0` through `k-1`.
10. Copy every public coordinate to raw X/Y/Z bits without arithmetic and compare through the claimed Component 11 vertex mapping.
11. Require a bijection between public vertices and used Component 11 realization symbols.
12. Require a bijection between public faces and Component 11 triangles.
13. For each mapped triangle, accept only its three orientation-preserving rotations and reject all reversed permutations.
14. Independently derive owner-free vertex tokens, triangle keys, component descriptors, component order, and face order.
15. Compare the independently expected public first-use indexing and all vectors with the candidate.
16. Replay Component 11 certificate applicability: unchanged coordinate bits and unchanged oriented triangle connectivity must preserve every realization obligation.
17. Recompute certificate counts and digests.
18. Independently encode `YGBCAN12`, `YGBOUT12`, and `YGBART01`.

The verifier may use a structurally different simple implementation even when less efficient. For bounded tests, exhaustive pairwise face and adjacency scans are preferred over producer-like maps.

Verifier resource exhaustion prevents publication with `resource_limit`. Any semantic disagreement is `internal_invariant_error`.

Mutation tests must alter every owner/dependency binding, policy/version, coordinate bit, signed-zero bit, vertex order, face order, ring rotation, ring reversal, index, face length, mapping, component range, edge use, `involved_faces` entry/order, normal, colour, metadata item, certificate fact, digest, and serialization field. Every mutation must fail in Release/NDEBUG.

## 14. Public result and handoff contract

On success, return a `boolean_result<T, I>` equivalent to:

```cpp
template<class T, class I>
struct boolean_success {
    fv_surface_mesh<T, I> mesh;
    output_summary summary;
    artifact_digest input_a_digest;
    artifact_digest input_b_digest;
    artifact_digest selected_boundary_digest;
    artifact_digest realized_boundary_digest;
    artifact_digest canonical_output_digest;
    std::shared_ptr<const output_assembly_certificate> certificate;
    std::optional<compact_output_provenance> provenance;
};

template<class T, class I>
using boolean_result =
    status_or<boolean_success<T, I>>;
```

The exact concrete Component 1 result representation may differ, but its semantics must be:

- success contains exactly one fully verified canonical mesh;
- failure contains no public mesh;
- the mesh is safe to copy independently of internal artifact lifetime;
- the certificate and provenance are immutable;
- optional provenance policy affects only the wrapper payload, not public mesh vectors or `YGBCAN12`;
- empty Boolean results return success with an entirely empty mesh and valid certificate;
- all output diagnostics identify the frozen operation and input digests;
- callers do not need Components 10 or 11 to consume the public mesh;
- replay and auditing code can retain dependency bindings when requested.

The public mesh contract for baseline output is:

```text
vertices          canonical first-use realized coordinates
vertex_normals    empty
vertex_colours    empty
faces             canonical oriented triangles
involved_faces    complete ascending reverse index
metadata          empty
```

## 15. Test plan

### 15.1 Baseline assembly and field policy

- Empty realization produces empty vertices, normals, colours, faces, `involved_faces`, and metadata.
- A tetrahedral shell emits four certified triangles with complete reverse incidence.
- Multiple disconnected shells, nested cavity boundaries, genus-bearing components, and high-valence vertices preserve topology and orientation.
- Every output face is exactly a Component 11 triangle under one of three cyclic rotations.
- Reversed triangle permutations fail.
- Artificial triangulation edges remain present as ordinary shared public edges.
- Unused realization symbols are omitted; every emitted symbol is indexed exactly once.
- Repeated use of one symbol across many triangles always shares one output index.
- Distinct symbols are never merged by coordinates.
- Normals, colours, and metadata remain empty even when inputs contained such fields.
- `involved_faces` contains exactly the ascending public face indices for each vertex.

### 15.2 Raw bits and ordering

- Exhaust `+0`, `-0`, minimum/maximum subnormals, minimum normals, powers of two, largest finite values, and positive/negative coordinates for float and double.
- Verify all X/Y/Z raw bits survive Component 11-to-public copying exactly.
- Use coordinates where native numeric comparison treats signed zeros as equal; canonical output must still preserve and encode distinct bits.
- Use negative values and NaN-like raw-order boundaries to prove no native float ordering is called. NaN/infinity must already be impossible and malformed injected values fail.
- Rotate every input realization triangle cyclically; canonical public ring remains identical.
- Reverse a realization triangle and require invariant failure rather than canonical reversal.
- Permute realization vertex, triangle, selected-patch, provenance, and component storage; canonical public vectors remain identical.
- Change only representative or alternate provenance attribution while preserving semantic Component 11 geometry; public vectors and `YGBCAN12` remain identical.
- Deliberately collide incomplete keys to prove complete owner-free tokens resolve valid distinctions or fail instead of using provenance.

### 15.3 Face-before-vertex canonicalization

- Construct fixtures where realization vertex order disagrees with canonical face order and require first canonical use.
- Construct disconnected components whose source and realization storage orders are reversed.
- Construct faces whose final public-index order would differ if vertices were sorted first; require the specified pre-index face order.
- Verify no pre-index key contains an output index.
- Compare producer output with an independent oracle that sorts owner-free triangle keys first and compacts second.
- Randomly permute all internal stores before canonicalization and require byte-identical output.

### 15.4 Index capacity and resource boundaries

Use a non-allocating checked-cardinality test seam plus feasible small-index test types where appropriate.

For both vertex and face domains, test:

- zero;
- one;
- `Imax`;
- `Imax + 1`, accepted as an addressable count when host/resource policy permits;
- `Imax + 2`, rejected as `index_overflow`;
- greatest emitted ordinal exactly `Imax`;
- one conversion beyond `Imax`;
- faces fitting while vertices overflow;
- vertices fitting while faces overflow because face IDs are stored in `involved_faces`;
- logical index overflow combined with a low allocation budget, requiring `index_overflow`;
- index-representable counts exceeding `size_t`, vector `max_size`, or configured bytes, requiring `resource_limit`;
- exact-at-limit and one-over mapping, adjacency, canonical-byte, verifier-scratch, and work limits;
- allocation failure and cancellation at every construction phase.

No test may require actually allocating billions of records merely to exercise arithmetic.

### 15.5 Independent public-vector verification

- Reorder vertices without rewriting faces.
- Rewrite faces but not `involved_faces`.
- Remove, duplicate, reorder, or add an `involved_faces` entry.
- Add an unused vertex.
- Duplicate a face.
- Remove one triangle.
- Introduce an out-of-range index.
- Collapse a triangle index.
- Create a boundary edge, three-use edge, or same-direction two-use edge.
- Break a vertex link while retaining edge counts.
- Swap disconnected component order.
- Swap faces within a component.
- Normalize one signed zero.
- Add one normal, colour, or metadata key.
- Corrupt every mapping and certificate field.
- Require the independent verifier to reject each mutation in Release builds.

### 15.6 Re-ingestion through Component 2

Every successful end-to-end baseline output must be re-ingested as a fresh operand through Component 2:

- empty output follows Component 2's explicitly supported empty-result ingestion path or a dedicated empty-mesh contract path;
- nonempty output passes finite-coordinate checks;
- every triangle has three distinct vertices and exact non-zero area;
- edge manifoldness and opposite orientation pass;
- every vertex link is valid;
- shells and cavities retain outward/occupied-side semantics;
- non-adjacent facets have only permitted shared-feature incidence;
- no tolerance cleanup or orientation repair is enabled during re-ingestion.

Compare the re-ingested topology with the Component 11 triangulated complex through canonical mappings. Do not compare decimal coordinates or file-format output.

Include all operations and representative cases: disjoint, overlapping, nested, equal, touching, tangent, coplanar overlap, cavities, multiple components, and empty results.

### 15.7 End-to-end and metamorphic tests

- Union, intersection, both differences, and symmetric difference over analytic fixtures.
- Operand swap with operation remapping.
- Source vertex/facet/shell permutations.
- Equivalent candidate schedules and Component 11 worker arrangements.
- Exactly representable translations, axis permutations, orientation-corrected sign flips, and positive power-of-two scales.
- Input subdivision changes may legitimately change baseline triangulation; require topological/set equivalence rather than identical bytes unless Component 11 semantic artifacts are identical.
- For one identical Component 11 semantic artifact, require byte-identical public vectors and `YGBCAN12`.
- Vary thread counts, worker delays, hash collision modes, allocation addresses, ambient rounding modes, and exact-filter settings.
- Compare GCC/Clang Debug/Release canonical bytes.
- ASan/UBSan cover malformed vectors, checked conversions, and rollback.
- TSan covers independent verification, cancellation, diagnostics, and publication.
- Fault injection before every publication boundary leaves no public result or committed output charge.

### 15.8 Optional simplification qualification

Before any future `proof_simplified_v2` policy ships:

- compare simplified and baseline outputs against the same exact selected boundary;
- verify exact domain-union equality;
- re-ingest both through Component 2;
- compare connected components, links, genus, cavities, orientation, and occupied side;
- independently replay replacement realization obligations;
- test non-coplanar rounded polygon candidates, narrow features, collinear subdivisions, coincidence seams, and high-valence vertices;
- prove a rejected simplification leaves baseline output unchanged;
- prove simplification never changes coordinate bits or returns `output_not_representable`.

These tests do not enable simplification in baseline v1.

## 16. Implementation sequence

Implement in this order:

1. Reconcile Components 1, 10, 11, and 13 output-policy, owner-free key, dependency, capacity, resource, verifier-environment, and public-result interfaces.
2. Freeze `triangulated_v1_no_simplification`, public-field policy, failure semantics, invariant codes, and `YGBCAN12`/`YGBOUT12` schemas.
3. Implement checked cardinality and index-conversion helpers with exhaustive maximum-boundary tests.
4. Implement owner-free realization vertex tokens and orientation-preserving triangle rotation.
5. Implement semantic edge grouping, adjacency validation, deterministic connected components, component descriptors, and pre-index face sorting.
6. Implement first-use realization-symbol indexing and bit-preserving public vertex/face assembly.
7. Implement exact baseline `involved_faces`, empty normals/colours/metadata policy, and output mapping records.
8. Implement assembly certificate, canonical encodings, typed diagnostics, accounting, cancellation, rollback, and atomic publication.
9. Implement the independent public-vector parser and verifier without producer canonicalization helpers.
10. Add mutation, re-ingestion, permutation, provenance-independence, maximum-`I`, resource, replay, compiler, sanitizer, and schedule-determinism suites.
11. Qualify full end-to-end Boolean results only after Components 1-11 and their mandatory verifiers pass.
12. Keep optional simplification disabled until a separately reviewed proof schema, replacement-obligation protocol, verifier, and test suite exist.

## 17. Definition of done

Component 12 is complete only when:

- Component 11 `triangulated_v1` triangles are emitted exactly once with no simplification, cleanup, retriangulation, welding, or orientation repair;
- canonical components and faces are ordered before first-use public vertex assignment;
- all ordering keys are complete, owner-free, schedule-stable, non-circular, and independent of representative provenance;
- no native floating ordering, equality, arithmetic, or decimal formatting controls canonicalization;
- every used realization symbol receives exactly one output index and every output index denotes one used symbol;
- triangle cyclic canonicalization never reverses orientation;
- every accepted coordinate bit pattern, including signed zero, is preserved exactly;
- vertex and face maximum-index semantics accept `Imax + 1` addressable entities when resources permit and reject `Imax + 2`;
- `index_overflow` and `resource_limit` are distinguished exactly as specified;
- Component 12 has no path returning `output_not_representable`;
- baseline normals, colours, and metadata are empty;
- baseline `involved_faces` is complete, ascending, duplicate-free, and independently reconstructed;
- public vectors are closed, manifold, consistently oriented, embedded by the still-applicable Component 11 certificate, and subdivision-equivalent to Component 10's selected exact boundary;
- the mandatory verifier parses all public vectors from scratch and does not trust producer mappings, adjacency, ordering, or certificates;
- successful output re-ingests through Component 2 without normalization, cleanup, or repair;
- empty output is a successful canonical result;
- existing XML/OFF/OBJ, remeshing, cleanup, and orientation routines are neither authoritative nor invoked;
- all malformed-artifact, mutation, capacity, cancellation, allocation, rollback, replay, compiler, sanitizer, and schedule tests pass in Release as well as Debug;
- equivalent execution schedules over one Component 11 semantic artifact produce byte-identical public vectors and canonical encodings;
- failure returns no public mesh and successful publication is atomic.

## 18. Plan-gap amendment: result-topology preflight

Component 12 owns `authorize_result_topology(...)` before Component 11 and repeats its binding check during final assembly. The authorization binds owner, selected-boundary identity/digest, selected topology class and certificate, and output-topology policy digest. Empty and `closed_embedded_two_manifold` are authorized by the initial policy. `closed_stratified_nonmanifold` returns `result_topology_not_supported` with sorted exact obstruction records and no realization attempt.

The preflight independently verifies Component 10's topology classification from its occurrence/link records. It never rounds coordinates, welds occurrences, drops contact strata, or reclassifies valid stratified topology as malformed. This establishes topology-failure precedence over coordinate representability. G1 vertex- and edge-touching union/xor cases must reach this typed failure; their intersection and difference cases remain successful.
