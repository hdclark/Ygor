# Component 2 implementation plan: input topology validation and canonicalization

## 1. Scope and outcome

Implement the input-validation stage that converts both borrowed `fv_surface_mesh<T, I>` operands into one certified, immutable `validated_operands<T, I>` artifact. The stage must prove that each operand is an embedded, closed, orientable polygonal B-rep satisfying the frozen `solid_policy`, retain exact source geometry and complete provenance, and publish stable original-feature stores for every later stage.

The completed component must provide:

- checked raw-mesh structural scanning before any indexed access;
- lossless conversion of every used finite coordinate through Component 3's exact dyadic API;
- exact validation and deterministic triangulation of every polygonal facet;
- explicit directed edge uses, twins, undirected edges, facet adjacency, and ordered vertex links;
- deterministic shell decomposition, strict shell nesting, and occupied-side orientation;
- exhaustive exact self-embeddedness and inter-shell contact validation;
- canonical semantic labeling and invocation-global IDs across operands A and B;
- complete raw-to-canonical provenance, conservative exact bounds, and canonical operand digests;
- an independent mandatory verifier, validation evidence, deterministic diagnostics, and transactional publication;
- focused, permutation, property, mutation, resource, cancellation, and schedule tests.

Do not perform cross-operand intersection discovery in this component. Each operand is validated independently as a solid; interactions between A and B begin in Component 4. Do not weld, snap, perturb input geometry, delete malformed facets, reverse facets, or reinterpret an invalid shell. An empty operand is a valid empty regular closed solid and publishes empty feature stores for that role.

All implementation must be self-contained in Ygor, use strict C++17, use only Component 1 contracts and Component 3 exact-kernel services for authoritative decisions, and contain no geometric tolerance. Components 3 and the required input-artifact verifier registration from Component 13 are prerequisites for production integration.

## 2. Existing Ygor assessment and reuse boundary

### 2.1 Reuse and adapt

- Keep `fv_surface_mesh<T, I>` from `src/YgorMath.h` as the immutable public input carrier. Read only `vertices` and `faces`. Ignore caller-provided normals, colours, `involved_faces`, and metadata for topology, validation, canonicalization, and digests.
- Retain the supported specialization matrix used by existing mesh code: `float` or `double` coordinates and `std::uint32_t` or `std::uint64_t` indices.
- Reuse the canonical undirected-pair idea in `make_undirected_edge` from `src/YgorMeshesVerification.h`: normalize an edge by ordered endpoint identities. The new record must preserve both directed uses and use Component 1 strong IDs rather than expose this legacy index pair as identity.
- Use `HasOnlyFiniteVertices`, `HasValidIndices`, and `recreate_involved_face_index` only as simple scanning precedents. Reimplement their work with checked conversions, source-local diagnostics, resource accounting, and immutable output; never trust or mutate raw `involved_faces`.
- Adapt the edge-record pattern in `src/YgorMeshesHoles.cc` and `src/YgorMeshesOrient.cc`: retain source facet, ring offset, directed endpoints, normalized endpoints, and direction. Do not carry over tolerance welding, skipped collapsed edges, mutation, or heuristic orientation.
- Adapt the facet-parity BFS idea from those files as a redundant orientability check over already validated halfedges. The authoritative input orientation remains the raw ring direction; this stage rejects a contradiction rather than reversing rings.
- Adapt the exhaustive ring-edge comparison structure, boundary-case taxonomy, and deterministic decomposition control flow from `src/YgorMathMonotoneDecomposition.cc`. Replace every area, membership, intersection, and ordering decision with Component 3 exact operations. Do not reuse its input cleanup, `long double` area, tolerance checks, or automatic winding changes.
- Reuse inclusive `index_bbox` semantics from `src/YgorIndex.h` as a possible non-authoritative acceleration representation. Exact dyadic coordinate extrema in the artifact are authoritative. Any `T` interval must be proven enclosing by Component 3 and touching boxes must overlap.
- Reuse deterministic complete-key sorting and fixed-seed test-generator patterns from `src/YgorMeshesBoolean5.cc` and `tests/Test_MeshesBoolean5.cc` as reference patterns only. The legacy Boolean sources remain prohibited implementation dependencies by `broad_plan.md` and are expected to be removed.
- Component 3 may audit and wrap `YgorMeshesAdaptivePredicates` as a certified filter. Component 2 calls only semantic exact-kernel APIs and must not call the adaptive functions directly or inspect approximate determinants.
- Use Ygor logging only as the optional post-publication diagnostic adapter established by Component 1. Expected invalid-input paths return structured errors and must not call `YLOGERR`.

### 2.2 Do not reuse as validation behavior

- Do not call `merge_duplicate_vertices`, `remove_degenerate_faces`, `remove_unused_vertices`, `convert_to_triangles`, `OrientFaces`, or `EnsureConsistentFaceOrientation`. They mutate, heal, use tolerances or floating heuristics, lose provenance, or assume a weaker input contract.
- Do not use `IsClosedManifold`, `HasConsistentOrientation`, or `HasNoDegenerateFaces` as a certificate. They are triangle-oriented and do not prove opposite uses in all malformed cases, polygon validity, vertex-link cycles, embedding, or shell semantics.
- Do not use generic `vec2`/`vec3` segment-membership, segment-intersection, point-in-polygon, `plane<T>`, BSP ray classification, or constrained-Delaunay decisions where epsilon, division, rounded construction, or perturbation controls topology.
- Do not use a fan triangulation for a general polygon. A concave source facet needs a proven internal triangulation.
- Do not infer adjacency from equal coordinates. Distinct source indices remain distinct topological vertices unless one is an unused vertex removed under the fixed rule below.
- Do not serialize raw structs, native container order, `size_t`, pointer values, or generic Boost mesh serialization as canonical artifact data.

## 3. Files, namespace, and integration

Add:

- `src/YgorMeshesBooleanInputTopology.h`: immutable record types, normalized key types, provenance types, exact-bound and validation-evidence types, the `validated_operand`/`validated_operands` artifact, artifact tag/spec constants, read-only accessors, and the checked stage entry point.
- `src/YgorMeshesBooleanInputTopology.cc`: structural/facet audits, canonical semantic labeling, topology construction, exhaustive self-pair audit, shell nesting/orientation validation, canonical encoding, verifier adapter, stage coordinator, and four explicit template instantiations.
- `tests/Test_MeshesBooleanInputTopology.cc`: normative standalone unit, contract, failure, transaction, and mutation tests using the existing simple pass/fail harness style.
- `tests/Test_MeshesBooleanInputTopologyProperties.cc`: bounded deterministic permutation, generated-topology, exhaustive-pair differential, sharding, and schedule tests with a separate CTest timeout.
- `tests/MeshBooleanInputTopologyFixtures.h`: test-only mesh builders, exact bit-pattern constructors, remapping/permutation helpers, and independent expected-topology utilities. Keep this under `tests/` so it is not installed as a public header.

Use namespace `ygor::mesh_boolean`. Keep implementation-only helpers in the `.cc` file or an unnamed namespace rather than adding installed private headers. Template record accessors may remain in the public header; substantial algorithms belong in the `.cc` file with explicit instantiations.

Update the Component 1 build integration:

- Amend `src/YgorMeshesBooleanContract.h/.cc` before Component 2 integration to add the process-only `verification_environment_view` and transaction-staged replay update described in Sections 6.1 and 8.8. Update `plan_01_contract_context.md`'s concrete callback/publication description when implementing this prerequisite so the checked-in architecture and code do not disagree.
- Add `YgorMeshesBooleanInputTopology.cc` to the named strict-arithmetic source list in `src/CMakeLists.txt`, after repository-wide flags. Effective fast-math must be disabled and the compile-time fast-math guard must apply to this translation unit.
- Add both test executables and `MeshBooleanInputTopology.Unit` and `MeshBooleanInputTopology.Properties` registrations to `tests/CMakeLists.txt`, link the in-tree Ygor target and `Threads::Threads`, and label them `mesh_boolean;component2`.
- Add both standalone test files to `tests/compile.sh`; keep `set -eu` effective and wait for every background compile.
- Run the named CTests authoritatively in `.gitlab-ci.yml` without `|| true`, in GCC/Clang and Debug/Release jobs. Add ASan/UBSan coverage for malformed indices, verifier mutation, and rollback, and TSan coverage for sharding, cancellation, and atomic publication. Do not introduce a network-fetched test dependency.

## 4. Public stage and dependency contract

Expose a stage coordinator with the conceptual form:

```cpp
template<class T, class I>
status_or<std::shared_ptr<const published_artifact<validated_operands<T, I>>>>
validate_operands(boolean_context<T, I>& context);
```

The concrete return aliases may follow Component 1, but these rules are mandatory:

1. Require a context whose exact-kernel service is the checked Component 3 implementation and whose verifier registry contains the exact Component 2 artifact tag, verification-spec version, and mandatory invariant set.
2. Open one `input_validation` transaction for artifact slot `validated_operands`; both A and B and all original-feature stores are constituents of that one draft.
3. Read each borrowed raw mesh only while the context lifetime precondition holds. Never cache mutable raw references in published records.
4. Charge exact coordinates, records, key material, triangulations, evidence, exhaustive pair work, sort scratch, verifier reconstruction, and canonical encodings through Component 1 accounting.
5. Check cancellation before each audit phase, at bounded intervals in all facet/edge/pair loops, before exact fallback, before verification, and immediately before publication.
6. Build private provisional records for both roles. Final IDs are never allocated by workers or independently per operand.
7. Freeze the candidate bundle, compute its digest, invoke the registered read-only verifier, obtain the matching certificate, and publish atomically. Failure of either operand exposes neither operand and no constituent store.
8. Convert public-boundary exceptions as Component 1 specifies. Geometry contract violations return `input_contract_error` at `input_validation`; configured work/memory/exact-number exhaustion returns `resource_limit`; malformed draft state or producer/verifier disagreement returns `internal_invariant_error`.

Component 2 may call only exact-kernel semantic operations, including exact input decoding, scalar/vector comparison, `orient2d`/`orient3d`, support-plane construction, point/plane and point/polygon classification, segment relations, polygon relations, exact signed area, exact affine constructions, ray/facet crossing, and symbolic point-location decisions. If Component 3 does not expose a required rich relation, extend Component 3's API rather than reconstructing the decision with rounded arithmetic in Component 2.

## 5. Fixed schema-v1 input policy

### 5.1 Empty operands and unused vertices

- A mesh with no faces represents the empty regular closed solid. It may contain raw vertices; all are unused and handled by the unused-vertex rule. It publishes no original features or shells for that role.
- Accept unused raw vertices if their coordinates are finite and supported. Remove them from the semantic operand before IDs freeze and retain a deterministic raw provenance disposition of `unused_removed`; do not assign them `original_vertex_id`.
- All raw vertices, including unused ones, participate in the Component 1 input digest and finite/encoding audit. Only used vertices participate in the canonicalized semantic operand digest.
- This is a fixed schema-v1 rule, not an option. Adding reject/preserve alternatives later requires an explicit policy and replay-schema extension.

### 5.2 Rings and source vertices

- A face is a cyclic ring with no repeated closing index. Require at least three entries.
- Reject consecutive equal indices, including the last/first pair; reject every repeated nonconsecutive index in the same ring. Do not erase repetitions.
- Preserve collinear boundary vertices when the ring is otherwise simple and has nonzero exact area. They are source vertices and edges, not cleanup opportunities. Triangulation must accommodate them without emitting zero-area triangles.
- Distinct used indices with equal exact coordinates remain distinct topology while structural records are built. The facet and embeddedness audits will reject any resulting zero-length edge, self-contact, undeclared contact, or shell contact. They are never silently merged.
- Reject duplicate facets regardless of equal or opposite ring direction. Detect topological duplicate cyclic rings before edge publication and geometric positive-area coincidence during embeddedness validation.

### 5.3 Shell contact and orientation

For the only schema-v1 `solid_policy`, `outward_oriented_nested_shells`:

- Distinct shells of one operand must have disjoint boundaries. Reject vertex, edge, tangent, crossing, coplanar, or positive-area boundary contact, including coordinate-coincident duplicate shells.
- Disjoint shells may be strict siblings or strictly nested. Build a strict containment forest; no ambiguous containment is accepted.
- Nesting depth determines occupancy polarity: even-depth shells add occupied material and odd-depth shells remove it as cavities. Deeper shells alternate in the same manner.
- Input ring orientation must already agree with this polarity. A geometrically outward-oriented shell is required at even depth and a geometrically inward-oriented shell at odd depth. Reject a globally reversed operand, a wrongly oriented cavity, or a local orientation contradiction. Do not normalize by reversing facets because no such policy is frozen in Component 1.
- Empty space outside all depth-zero shells is unoccupied. The resulting nested boundary therefore defines one unambiguous regular closed solid.

Document these decisions in public comments and canonical policy/version metadata. Later normalization APIs may deliberately weld, split, or reorient meshes, but Boolean validation does not.

## 6. Artifact and record model

### 6.1 Artifact ownership

Define `validated_operands<T, I>` as one immutable bundle containing:

- owner token, setup digest, artifact type/version, and exact-kernel arithmetic-policy digest;
- exactly two `validated_operand` views in role order A then B;
- invocation-global `published_store`s for original vertices, facets, edge uses, undirected edges, and shells;
- per-role raw provenance/disposition tables and canonicalized operand digests;
- validation evidence and exact conservative bounds;
- no pointer or reference into the raw input mesh.

Component 1's two-argument verifier callback is insufficient for an independent input checker because `artifact_view` contains neither authoritative borrowed inputs nor kernel/accounting services. Before implementing this component, extend the callback to `verifier_callback(const artifact_view&, const verification_spec&, const verification_environment_view&)`. The process-only, nonserializable environment contains the owner token; checked immutable kernel service; resource-accounting and cancellation facades; coordinate/index type tags; and type-erased const views of both context-borrowed raw operands. The Component 2 typed adapter validates owner/type bindings, casts the two operands to the supported `fv_surface_mesh<T,I>` specialization, and rescans them independently. The environment exists only for the synchronous verifier invocation, is never retained by the candidate or published artifact, and is excluded from canonical bytes/digests. Do not use globals, copy unbudgeted input archives, or instantiate replacement services. This is a required Component 1 API correction, not behavior Component 2 may add unilaterally.

Each `validated_operand` stores role, ordered ranges/lists into the global stores, root shells, raw and semantic counts, canonical digest, and its occupied-side convention. Empty ranges are valid. Accessors validate owner, role, ID domain, and record membership.

### 6.2 Vertex and provenance records

Define records equivalent to:

```cpp
enum class raw_vertex_disposition : std::uint8_t {
    retained,
    unused_removed
};

struct source_vertex_provenance {
    operand_id operand;
    std::uint64_t raw_vertex_ordinal;
    raw_vertex_disposition disposition;
    optional<original_vertex_id> canonical_vertex;
    std::array<coordinate_bits<T>, 3> raw_bits;
    optional<exact_point3> exact_coordinate;
};

template<class T>
struct validated_vertex {
    operand_id operand;
    original_vertex_id id;
    std::array<T, 3> raw_coordinate;
    std::array<coordinate_bits<T>, 3> raw_bits;
    exact_point3 exact_coordinate;
    shell_id shell;
    std::vector<edge_use_id> ordered_outgoing_link;
    std::vector<source_vertex_provenance> sources;
};
```

Use the concrete Component 3 exact point handle/value and Component 1 accounting containers. Preserve signed-zero bits for retained and removed-unused vertices even though exact `+0` and `-0` compare mathematically equal. Every raw source has exactly one disposition record. Every retained source maps to exactly one canonical vertex and every canonical vertex maps to exactly one retained source; no record collects all members of an automorphism orbit or implies welding. An unused source has no canonical ID but retains its raw bits and successfully decoded exact coordinate so the verifier can audit finite conversion and removal without borrowing the input.

Define analogous source-facet provenance `(operand, raw_face_ordinal)` and source-edge-use provenance `(operand, raw_face_ordinal, raw_ring_offset)`. Raw ordinals are diagnostic/provenance values, not semantic IDs or semantic canonical-order tie breakers.

### 6.3 Facet records

Each `validated_facet` must contain:

- role and `facet_id`;
- a canonical cyclic vector of oriented `original_vertex_id`s;
- a same-length vector of directed `edge_use_id`s;
- exact support plane in normalized Component 3 form and the source IDs of its deterministic establishing triple;
- deterministic projection axis and exact projected ring;
- exact nonzero signed projected area and orientation relation to the 3D ring;
- a deterministic triangulation as oriented triples of source vertex IDs;
- explicit private-diagonal records tagged `internal_nonsemantic`, with exact inside/visibility evidence;
- edge-neighbor facets in ring order and complete source provenance;
- exact coordinate extrema and optional proven enclosing `T` bounds.

Internal triangles and diagonals are acceleration/predicate data only. They receive no `facet_id`, `edge_use_id`, or `undirected_edge_id`, never appear as public B-rep boundaries, and must always reconcile to the source polygon in pair relations.

### 6.4 Edge-use, edge, and link records

Each `validated_edge_use` stores ID, role, incident facet, local ring offset, directed origin/destination, previous and next use in the facet ring, twin, owning undirected edge, shell, and source provenance. Each `validated_undirected_edge` stores ID, role, ordered endpoint IDs, exactly two opposite directed uses, the two incident facets, shell, and exact endpoint bounds.

Do not create a separate mutable adjacency table. Derive facet adjacency from twins and expose a canonical cached view only if the verifier reconstructs and compares it.

For each vertex, `ordered_outgoing_link` is the cyclic radial/topological order induced combinatorially by facet rings and twins. Define successor for outgoing use `h=(v,w)` as the outgoing use at `v` reached through the twin of the previous ring edge of `h`'s facet; use one documented equivalent convention everywhere. The sequence begins at the minimum canonical edge-use key and follows that successor. It must visit every incident outgoing use exactly once and return to its start.

### 6.5 Shell and nesting records

Each `validated_shell` stores:

- role and `shell_id`;
- canonically sorted facet, edge, edge-use, and vertex IDs;
- exact component bounds;
- exact nonzero oriented volume numerator/rational computed from the certified facet triangulations;
- geometric orientation (`outward` or `inward`), containment parent if any, sorted children, and depth;
- semantic contribution (`material_boundary` at even depth or `cavity_boundary` at odd depth);
- one canonical containment witness source vertex and exact point-location evidence against its parent and relevant competing shells;
- connectivity, orientability, embeddedness, nesting, and orientation evidence references.

The shell record must not claim that Euler characteristic or volume sign alone proves manifoldness or embedding. Those are consequences/supporting checks after edge, link, and intersection validation.

### 6.6 Validation evidence

Use compact, versioned evidence records for successful mandatory checks. Each record names an invariant code, sorted feature IDs or pre-ID source locators, exact predicate/relation result, exact operand/construction references, and evidence digest. At minimum retain evidence summaries for:

- coordinate decoding and facet planarity/area/simplicity;
- triangulation partition and diagonal legality;
- edge pairing, ring cycles, vertex-link cycles, and shell connectivity;
- self-pair and shell-pair embeddedness coverage;
- containment relations and shell orientation/polarity.

Do not retain an unbounded predicate transcript by default. Store aggregate coverage plus the witnesses needed to replay boundary decisions; full predicate traces are governed by Component 1 trace policy. Certificates and verification reports are separate from the artifact digest as established by Component 1.

## 7. Deterministic canonicalization and IDs

### 7.1 Semantic labeling before strong IDs

Raw indices and traversal order cannot define stable semantic order. Build a colored incidence graph per operand after topology, shell decomposition, and embeddedness are proven, but before containment queries and final IDs:

- vertex-node initial color bytes are `(node_type, exact_point3, incident_edge_count, incident_facet_count)`;
- facet-node initial color bytes are `(node_type, normalized_oriented_exact_plane, ring_length, cyclic multiset of exact ring-point/edge-vector encodings)` and exclude triangulation and raw ordinal;
- directed-incidence-node initial color bytes are `(node_type, exact origin point, exact destination point, projection-axis tag)`;
- typed graph arcs encode vertex-to-incidence origin/destination, incidence previous/next, incidence-to-facet, incidence twin, and facet-to-shell-component relations; reverse arcs have distinct explicit labels where direction matters;
- shell-component nodes are colored by exact bounds, exact oriented-volume sign/value, member counts, and their component incidence relation, but not containment parent or depth;
- operand role is always the first color/key field.

Encode every initial color through `canonical_encoder`; sort distinct byte strings lexicographically and assign dense refinement colors by that rank. At each refinement round, encode `(old_color, sorted vector of (arc_type, target_old_color))`, rank all distinct signatures lexicographically, and stop only when the complete partition is unchanged. For a nonsingleton result, choose the nonsingleton cell with least color, individualize each member in turn with one new distinguished color, refine recursively, and serialize each discrete leaf as `(ordered node initial colors, sorted typed adjacency tuples under leaf labels)`. The canonical form is the lexicographically least complete leaf serialization. Explore every branch in the initial implementation; do not use prefix pruning until a separately specified lower-bound proof and differential oracle tests exist. Branch iteration may use raw storage order because every branch is explored, but raw order never enters a leaf or chooses among unequal leaves. Account every branch/work byte and return `resource_limit`, with no artifact, rather than falling back to raw order when a configured limit is reached.

Symmetric automorphisms can yield several source-to-label mappings with identical minimum semantic encoding. Select one equal minimum leaf by lexicographically comparing its complete vector of raw source locators only after semantic leaf equality is established. This gives every retained raw source exactly one ID for diagnostics, but that raw-source-to-ID bijection may vary within an automorphism orbit after input permutation. Raw locators and the equal-leaf tie choice are excluded from semantic store ordering and canonicalized operand digests. Tests require invariant abstract canonical store bytes/IDs and separately validate the one-to-one provenance mapping; they must not demand an impossible permutation-equivariant source-to-ID mapping for indistinguishable automorphic features.

This exact canonical-label path is required for correctness and deterministic replay. A faster canonical-label implementation may replace it only when differential tests prove byte-identical labels against the exhaustive individualization/refinement oracle.

### 7.2 Ring, edge, facet, and shell normalization

- Preserve facet winding. Rotate each cyclic ring to its lexicographically least rotation under canonical vertex labels; use a linear least-rotation algorithm or compare all rotations for the initial implementation. Never reverse a ring during rotation normalization.
- For pre-label duplicate detection, compare exact cyclic source-index sequences under all rotations in both directions. This check is local and must not become the final identity key.
- Normalize an undirected edge by `(operand, min(vertex), max(vertex))` and preserve direction only in edge-use records.
- Define an edge-use key as `(operand, facet semantic key, local position in its normalized oriented ring, directed endpoint keys)`.
- Define a facet key from operand, oriented exact plane, normalized oriented ring semantic encoding, and geometry. Internal triangulation choices and raw face ordinal are not identity fields.
- Define a shell key from operand and canonical colored component-subgraph encoding, including exact bounds and oriented volume but excluding containment parent, depth, selected witness, and BFS discovery order. Nesting enriches the already identified shell record and does not redefine original-feature identity.

### 7.3 Joint ID assignment

After both operands have passed embeddedness and are fully semantically labeled, but before symbolic shell point location, use Component 1's `canonical_id_factory` separately for each strong-ID domain and feed records from both roles into each factory. Keys begin with operand role, so IDs are dense and invocation-global while A sorts before B. Assign in dependency order:

1. original vertices;
2. facets after remapping rings to vertex IDs;
3. edge uses after remapping facet/ring incidence;
4. undirected edges after remapping their two uses;
5. shells after remapping complete component contents, before adding nesting fields.

If Component 1's concrete key factory requires all references to have IDs first, use immutable temporary semantic labels and perform checked remapping between passes. No provisional label escapes the transaction. Verify equal keys by full exact semantic comparison; a conflicting duplicate is an internal producer defect, not an arbitrary tie.

### 7.4 Canonical operand encoding

Define a versioned top-level `YGBOPD02` canonical record. Its payload order is:

1. schema version, operand role, coordinate/index type tags, and solid policy;
2. semantic used-vertex/facet/edge-use/edge/shell counts;
3. vertices in canonical ID order: exact coordinate canonical encoding and semantic incidence only;
4. facets in canonical ID order: ring, plane, projection, exact area, triangulation, and semantic adjacency;
5. edge uses and undirected edges in canonical ID order;
6. shells in canonical ID order: members, bounds, exact volume, containment, depth, and orientation role;
7. provenance-free semantic validation-evidence digest.

Frame the record using Component 1's top-level grammar and define `canonicalized_operand_digest = MD5("YGBOPD02" || framed operand payload)`. Exclude raw source ordinals, source input order, unused vertices, process owner token, diagnostics, reports, certificates, raw-locator evidence, and optional acceleration layout. Consequently, vertex/index/facet/ring rotations that describe the same oriented B-rep produce the same canonicalized operand digest, while Component 1's original input digest remains order- and bit-pattern-sensitive.

Define the complete candidate artifact payload as framed `YGBVAT02` fields in this order: artifact schema and type tag; kernel arithmetic-policy digest; length-prefixed A and B `YGBOPD02` payloads; each invocation-global store in ID order with count and complete semantic records; `YGBPRV02` provenance payload containing all raw vertex/facet/ring locators, raw coordinate bits/exact values, dispositions, and their mapped IDs; provenance-free semantic evidence payload; raw-locator validation-evidence payload; and exact authoritative bounds. Empty stores/vectors encode a zero `u64` count and optional fields use Component 1 presence booleans. Exclude owner token, process-only verification environment, diagnostics, reports, certificates, and replaceable acceleration layout. Compute the invocation-bound artifact digest exactly as Component 1 requires: `MD5("YGBART01" || setup_digest || artifact_slot || YGBVAT02 framed payload)`. Therefore input permutations may preserve both operand semantic digests while changing setup, full provenance payload, artifact digest, report digest, and replay descriptor.

## 8. Validation pipeline

### 8.1 Phase A: raw structural scan

For A then B in role order, with deterministic sharding permitted only inside a role:

1. Validate raw vertex and face counts with Component 1 checked `size_t`/`uint64_t`/`I` conversion helpers before allocating maps or forming IDs. A face count need not fit `I` merely because indices use `I`; every internal conversion must still be checked and final strong-ID capacity enforced.
2. Scan every raw coordinate, including unused vertices, in raw ordinal/component order. Reject NaN, infinity, unsupported encoding, or failed exact conversion with operand, raw vertex ordinal, component, coordinate bits, and kernel evidence.
3. Scan each face length and every index before dereference. Report the lexicographically first `(operand, raw face ordinal, ring offset)` violation. Reject rings shorter than three, repeated closing index, consecutive duplicate, or repeated nonconsecutive index.
4. Build checked used-vertex flags and raw source incidence. Apply the fixed unused-removal disposition without mutating the source mesh.
5. Detect exact-coordinate-equal endpoints for every topological edge and reject them as zero-length even when indices differ.
6. Construct provisional source locators and reserve worst-case edge-use counts using checked sums. No strong feature ID is available yet; errors use tagged operand/raw-ordinal locators in deterministic fields and never fabricate IDs.

When work is parallel, worker failures are merged by canonical source locator and stable subcode. Completion timing must not choose the reported error.

### 8.2 Phase B: exact facet audit

For every raw facet in canonical source-work order:

1. Find the lexicographically first source triple with nonzero exact `orient2d` in at least one coordinate projection, using a deterministic bounded search. Collinear leading vertices are allowed. If no triple exists, report exact zero-area/collinearity evidence.
2. Construct a normalized exact support plane through that triple. Classify every ring vertex against it using exact point-plane/orientation predicates; reject the first non-coplanar vertex. Do not use `plane<T>` normalization.
3. Select the projection axis by exactly comparing absolute plane-normal components and dropping the largest; break exact ties by fixed axis order X, then Y, then Z. Record the orientation parity between projected and 3D rings.
4. Project all vertices exactly. Require every boundary segment to have distinct endpoints. Compare all non-adjacent segment pairs in lexicographic ring-position order with the rich exact segment relation. Adjacent segments may meet only at their declared shared endpoint; the first and last segments are adjacent. Reject proper crossing, collinear overlap, undeclared endpoint touch, or any distinct-index equal-coordinate contact.
5. Compute exact signed shoelace area. Reject zero; retain sign and normalized value. Do not automatically reverse a ring based on sign because support-plane orientation and shell policy are checked later.
6. Detect topologically duplicate facets by canonical cyclic source-index keys, treating reversal as duplicate for rejection. Retain exact geometric duplicate/overlap detection for Phase E because distinct source indices can describe the same region.
7. Produce the deterministic exact triangulation in Section 9 and validate its certificate before accepting the facet.
8. Compute exact coordinate extrema directly from exact source coordinates. If a `T` interval form is stored, require Component 3 to prove containment; otherwise omit it and let later acceleration use exact extrema or exhaustive fallback.

Facet failures report the smallest source facet and ring offsets known, exact relation enum/sign, predicate operand references, original coordinate bits, and replay metadata.

### 8.3 Phase C: halfedge topology construction

1. Emit one provisional directed edge use per oriented facet-ring edge, preserving facet and local offset.
2. Sort uses by exact topological undirected endpoint key. For each group, reject a self-edge, one use, more than two uses, two uses from the same facet/local edge, or two uses with the same direction. Require exactly two opposite directed uses and assign twins.
3. Reject duplicate directed uses explicitly even when another failure such as nonmanifold edge also applies; deterministic subcode precedence is specified in Section 13.
4. Build previous/next ring links and verify each facet cycle closes after exactly its ring length without repeated use.
5. Derive facet adjacency from twins in ring order. Do not infer adjacency from shared vertices or coordinates.
6. For each used vertex, construct the link-successor permutation from ring and twin relations. Require every successor/predecessor to be unique, traverse from the least provisional semantic edge-use key, and require one cycle containing every incident outgoing use. More than one cycle is a pinched/bow-tie vertex and is rejected even if every edge has two uses.
7. Run the independent facet-parity propagation check: crossing every shared edge must impose a consistent orientation parity. A contradiction is invalid orientability/orientation. Because opposite edge uses are required, the accepted raw rings already form one consistent orientation and are not modified.

### 8.4 Phase D: shell decomposition

1. Build connected components of facets through twin-edge adjacency. Vertex-only or coordinate-only contact never joins components.
2. Assign every edge use, edge, and vertex to exactly one component. A vertex appearing in multiple edge-connected components would have a multi-cycle link and must already have failed Phase C; check this invariant again.
3. Verify each component is nonempty, connected, closed, and orientable from its constituent records.
4. Compute component exact bounds and exact oriented volume by summing signed tetrahedral determinants from every certified source-facet triangle against the exact origin. Normalize consistently by six. Translation independence follows from closure and is tested; a zero volume for an embedded closed shell is an internal contradiction or invalid geometry and prevents publication.
5. Retain provisional shell semantic encodings for pair audits and canonical labeling. Do not assign final shell IDs in discovery order.

### 8.5 Phase E: exact embeddedness and contact audit

Initially enumerate every unordered facet pair within each operand in canonical provisional work-key order. Do not depend on Component 4 and do not issue `candidate_id`s. A safe private bounds rejection may skip exact narrow phase only if exact extrema prove a strict gap; exhaustive mode runs/compares the unpruned relation.

For each pair:

1. Determine the complete declared topological intersection from shared source vertex and undirected-edge incidence. Internal triangulation diagonals are never declared intersections.
2. Evaluate exact source-polygon/polygon relation. Triangles may accelerate the relation, but merge triangle results and prove them against source polygon boundaries so artificial diagonals cannot create contacts or identities.
3. Facets sharing an edge may intersect exactly in the full declared shared edge and its endpoints, including when coplanar. Reject positive-area overlap, extension beyond the edge, a second contact component, or any other intersection.
4. Facets sharing only one or more declared vertices may intersect exactly at those declared vertices. Reject a curve, area, undeclared point, or crossing. Multiple shared vertices without a shared edge are accepted only if the full vertex-link/topological and geometric tests prove no extra contact; otherwise reject at the first causal condition.
5. Facets with no declared common feature must be geometrically disjoint. Reject tangency, vertex-on-facet, edge-edge/edge-facet contact, proper crossing, coplanar overlap, or geometric coincidence. Thus equal-coordinate distinct topological vertices are preserved as distinct until this exact audit and then rejected when they create undeclared contact.
6. Pairs in different provisional shells must be completely boundary-disjoint. Any relation other than disjoint is a shell-contact policy violation, with duplicate/coincident positive-area shells diagnosed explicitly.

Record coverage counts/digests for all pairs and relation categories. In mandatory production mode, exhaustive enumeration is authoritative for the initial implementation. After Component 4 exists, an optional private self-query adapter may prune pairs only when its candidate set is sorted/deduplicated and exhaustive differential tests prove identical accepted artifacts and first failures. Preserve the exhaustive path permanently for verification and small inputs.

### 8.6 Phase F: canonical labels and stable original-feature IDs

After both operands pass Phase E, run Section 7 canonical labeling jointly and assign final original vertex, facet, edge-use, undirected-edge, and shell IDs across A and B. Remap all incidence and sort one-to-one provenance now. Shell identity is based on its embedded connected component and exact oriented volume, not on nesting fields that are still unknown. These IDs remain transaction-private but final and stable, and Component 3 may use them for symbolic point-location ties in the next phase. Failure or cancellation still rolls back every ID and exposes no store.

### 8.7 Phase G: shell containment and orientation

After boundary disjointness is proven:

1. For each ordered pair of distinct shells whose exact bounds do not prove separation, choose the query shell vertex with least final `original_vertex_id`. Exact-classify that point against the other shell. Boundary is an internal contradiction because Phase E proved disjoint boundaries; inside/outside is constant over the connected query shell.
2. Implement point location through Component 3 exact ray/facet crossing and stable-ID symbolic perturbation. Record unperturbed boundary relations separately; perturbation only chooses a generic ray/probe and must not move the solid or erase a real contact. Retry/order rules derive exclusively from final original-feature IDs, never raw ordinals, provisional keys, or numeric offsets.
3. Cross-check pair classifications for antisymmetry: two disjoint connected shells cannot each strictly contain the other. Detect duplicate/equal shells before this phase.
4. For each shell, choose as parent the containing shell with no other containing shell strictly between them. Resolve this from the complete containment relation, not volume magnitude or first hit. Multiple incomparable candidate parents are an invariant failure.
5. Sort roots and children by semantic shell key, assign depth from roots, and prove the forest is acyclic and complete.
6. Derive geometric orientation from the sign of the exact oriented volume under one documented right-handed determinant convention. Positive means geometrically outward and negative inward; verify this convention with analytic tetrahedron tests and an independent local side query.
7. Require outward orientation at even depth and inward orientation at odd depth. Reject mismatch with shell source features, volume sign, depth, parent, and point-location evidence.
8. Establish occupied-side metadata: for every oriented facet, the occupied material side is the negative side of its right-hand-rule normal under the accepted nesting orientation. For the independent side check, choose the least-ID nonzero triangle of the facet triangulation, construct its exact barycenter as a facet-interior witness, and ask Component 3 for the two symbolic open-side classifications `p +/- epsilon*n`. `epsilon` is a lexicographic infinitesimal keyed by facet/triangle IDs, not a represented coordinate or numeric tolerance. Preserve the unperturbed point-on-facet relation and require the two symbolic sides to agree with nesting occupancy. If the chosen barycenter lies on another source edge because of triangulation structure, advance by triangle ID; inability to obtain a source-facet-interior witness is an invariant failure with exact evidence.

### 8.8 Phase H: encoding, verification, and publication

1. Complete nesting/orientation fields on the already identified records, check no invalid sentinel or cross-role reference remains, and build all immutable stores with accounting allocators. Recheck every count and ID conversion before allocation and publication.
2. Canonically encode each semantic operand and the complete artifact. Stage the canonicalized A/B replay-digest updates inside the same transaction; they are not visible yet.
3. Run cheap producer-side invariants, freeze the candidate, and invoke the independent Component 2 verifier with the mandatory invariant set. Exhaustive verification adds alternate point-location paths and canonical-label oracle comparison.
4. Use the required Component 1 transaction extension: `stage_replay_update` prebuilds the replacement replay descriptor and descriptor digest in private storage, validates that only this slot's canonicalized A/B operand fields and artifact entry change, and reserves all bytes before publication. Mint and validate the certificate, check cancellation and diagnostics completion, then atomically install the artifact slot, committed accounting, and prevalidated replay descriptor pointer at Component 1's nonthrowing publication linearization point. Update Component 1's publication contract/tests to include this pointer swap. Any prior error destroys the transaction, discards the staged descriptor, and releases private reservations.

## 9. Deterministic exact facet triangulation

Implement an exact ear-clipping triangulation over the projected simple ring unless Component 3 supplies a simpler already-certified polygon triangulator. It must preserve every source boundary vertex, including collinear vertices, while producing only nonzero-area triangles.

1. Normalize only the working ring's start position by the lexicographically least rotation of `(exact projected point, incoming exact edge vector, outgoing exact edge vector)` tuples; accepted facet points are distinct, and full cyclic comparison resolves repeated tuple prefixes. Preserve source winding and retain the raw-position mapping only as provenance.
2. Classify a candidate diagonal between non-adjacent current vertices against the immutable source boundary, every source vertex, and all previously accepted internal diagonals/triangle boundaries. Its open segment must lie in the source polygon interior, meet the immutable boundary only at its endpoints, contain no non-endpoint source vertex, and neither cross nor overlap prior internal edges.
3. A candidate ear must have nonzero orientation matching the polygon area sign, a legal internal diagonal, and contain no other source vertex or current-cycle vertex in its closed triangle except the ear's own three vertices. Boundary classifications are explicit so collinear chains do not create zero-area ears or T-junctions.
4. Among all valid ears, choose the least tuple of the three exact projected-point encodings, immutable normalized ring positions, and candidate-diagonal encoding. This total key uses only Phase B data, not later incidence, IDs, or raw ordinals. Remove only the ear from the private working cycle; do not alter the source facet record.
5. Use deterministic backtracking over ears in that key order if a greedy branch cannot complete while preserving all source vertices. Define a complete triangulation encoding as the lexicographically sorted vector of individually normalized oriented triangle point/position tuples, independent of ear emission order. Memoize `canonical current cycle -> lexicographically least valid suffix triangle set or no-solution`; when combining a prefix ear with a cached suffix, sort and compare the complete set. Do not use a visited-cycle cache that discards a different prefix. Account every search node and memoized suffix. A proven simple nonzero-area polygon must have at least one complete branch; exhausting a configured search budget returns `resource_limit`, while no complete branch in unlimited mode is `internal_invariant_error` with the explored-cycle evidence.
6. Finish with one nonzero triangle and orient every triangle consistently with the source projected area.
7. Verify exactly that triangle interiors are pairwise disjoint, their boundary incidence consists of the source ring plus paired internal diagonals, every source boundary segment occurs once, each internal diagonal occurs twice oppositely, and the exact signed triangle-area sum equals the polygon area.
8. Require exactly `n-2` triangles for a ring of `n` vertices. If preservation of a collinear vertex makes naive ear choice fail, valid ears on adjacent non-collinear triples still exist; tests must include long collinear chains. Never drop the vertex or emit a degenerate triangle as a workaround.

Triangulation is deterministic but is excluded from facet mathematical identity where two valid triangulations could differ. It remains in canonical artifact bytes so a changed implementation is replay-visible, and its full partition certificate is mandatory.

## 10. Mandatory verifier specification

Register a stable Component 2 artifact type tag, checker version, and named invariant set. The verifier receives Component 1's frozen `artifact_view`, verification spec, and owner-bound process-only `verification_environment_view` described in Section 6.1. It is read-only and must not call producer-private derived-state helpers where an independent reconstruction is practical.

Mandatory checks must:

1. Validate owner, slot, type tag, schema/kernel digest, role partition, dense store IDs, sorted order, and every cross-reference.
2. Rescan both authoritative borrowed raw operands from the verification environment: compare every vertex count/bit pattern, face count, face length, and index against candidate provenance before trusting any disposition or mapping. Recompute used/unused incidence from these raw faces; ensure every retained source maps once, every unused source has no facet incidence or ID, and every ID belongs to the recorded role/shell.
3. Re-decode every authoritative retained and unused raw coordinate bit pattern exactly, compare it to the candidate provenance and stored exact point, reject non-finite/unsupported patterns, and check exact bounds over retained points. Input digests are integrity bindings, not substitutes for these comparisons.
4. Reconstruct every facet ring from edge uses, recompute plane incidence, projection, exact area, simplicity evidence, and triangulation partition.
5. Regroup edge uses independently by endpoints; require exactly two opposite uses, recompute twins/edges/facet adjacency, and compare all cached fields.
6. Reconstruct each vertex-link successor graph and require one cycle containing all incident uses. Reconstruct shell connected components independently and compare memberships.
7. Independently point-locate the canonical witness for every required ordered shell pair, reconstruct the complete strict containment relation and parent forest, derive depths and orientation polarity, and verify every local symbolic open-side query and occupied-side convention. Do not accept internally consistent stored parent/depth fields without recomputing containment.
8. Validate embeddedness coverage evidence and rerun a deterministic mandatory subset sufficient to bind evidence to actual records. For the initial implementation, rerun all facet pairs; a later bounded mandatory checker may use independently proven certificates only after Component 13 defines that proof contract.
9. Re-encode canonical semantic operands and artifact and compare all digests. Verify canonical ordering without trusting stored sort keys.
10. Return the first causal failure in fixed invariant-code order with sorted evidence. Any discrepancy in a candidate built from accepted user input is `internal_invariant_error`; resource exhaustion remains `resource_limit` and prevents certification.

Exhaustive verification additionally:

- compares canonical labeling with a slower exhaustive oracle on bounded artifacts;
- reevaluates every source facet pair without private acceleration and compares relation categories;
- repeats shell point location with alternate symbolic rays/orders and requires the same unperturbed result;
- repeats local outward/inward symbolic side queries with alternate valid facet-interior witnesses and checks translation-invariant shell volume;
- validates provenance minimization hooks by tracing each record back to raw faces/ring positions.

Mutation tests must corrupt each major field, including a coordinate, ring entry, plane, projected point, triangle, diagonal tag, twin, previous/next link, vertex-link order, shell member, parent/depth, orientation role, bound, provenance, owner token, digest, and evidence relation. The appropriate invariant must reject every mutation before publication.

## 11. Error taxonomy, deterministic precedence, and diagnostics

Add stable input-validation subcodes for at least:

- unsupported/non-finite coordinate encoding;
- raw count or index conversion overflow and out-of-range index;
- short ring, repeated closing/consecutive/nonconsecutive vertex, and zero-length geometric edge;
- collinear/zero-area facet, non-planar facet, self-crossing/self-touching ring, and duplicate facet;
- invalid triangulation precondition or producer triangulation invariant;
- boundary edge, nonmanifold edge, same-direction twin uses, duplicate directed use, and duplicate edge use;
- open/broken facet cycle, disconnected vertex link, pinched/bow-tie vertex, and orientability contradiction;
- undeclared self-contact, proper self-intersection, adjacent-facet excess intersection, coplanar positive-area overlap, duplicate shell, and forbidden shell contact;
- containment contradiction and shell orientation/depth mismatch.

For failures found in one scan/group, use a fixed precedence from earliest validation phase, then subcode numeric order, then canonical source locator. Do not let a later derived symptom replace the first causal violation. Within edge grouping, prefer zero/self edge, duplicate use, cardinality, then same-direction orientation. Within pair auditing, prefer positive-area coincidence/overlap, proper crossing, curve contact, then point contact when several relations are present. Document and test the precise enum order.

Before IDs exist, identify features with structured `(operand role, raw face/vertex/ring ordinal)` locators in deterministic fields. After IDs exist, include strong feature refs and retain source locators. Evidence includes exact signs/relation enums, canonical exact values or limb encodings, and original floating bit patterns; approximate decimal text is display-only.

Emit stage-local diagnostics for aggregate counts, removed unused vertices, shell/nesting summary, exact fallback/resource observations, and validation failure. Sort and publish diagnostics through Component 1. Never log one message per exhaustive pair by default. Full pair/predicate detail belongs in a charged full trace.

## 12. Resource, concurrency, and lifetime requirements

- Use checked arithmetic for total ring entries, edge uses, pair counts `n*(n-1)/2`, triangulation counts, encoding sizes, and all byte reservations. Never form an overflowing product before checking it.
- Charge used and unused source dispositions, exact coordinate storage, canonical-label search, temporary pair work, triangulation, bounds, evidence, and verifier reconstruction to the appropriate private or committed resource kind.
- Charge each exact predicate/construction through Component 3's exact-number/work accounting. Unlimited correctness mode may be expensive but has no arbitrary geometry-size cutoff.
- Partition independent facet audits and facet-pair evaluations by canonical source work keys. Workers write only private shards. Merge records, evidence, failures, and additional resource requests in key order.
- Canonical-label branching and shell containment may be parallelized only if branch/result selection remains lexicographic and resource grants follow Component 1's deterministic round protocol. A serial initial implementation is preferred over schedule-sensitive complexity.
- Bound cancellation checks by a documented maximum number of ring edges, pair tests, exact fallbacks, or canonical-label nodes between observations.
- Published exact values and records own their storage. No pointer into raw vectors, worker scratch, kernel cache entries with shorter lifetime, or transaction shards may escape.
- A caller mutation of borrowed raw meshes during validation violates Component 1's API precondition. Debug lifetime/digest guards may diagnose it but cannot make concurrent mutation supported.

## 13. Implementation sequence

Implement and gate the work in this order:

1. Add public record/provenance/evidence types, schema/tag constants, exact-kernel dependency checks, and canonical encoders with compile-time API/immutability tests.
2. Implement raw count/index/finite/exact-conversion scans and the fixed unused-vertex policy for all four type pairs.
3. Implement exact support-plane/projection/ring simplicity/area validation and deterministic facet failure evidence.
4. Implement exact ear clipping and independent triangulation partition verification, including collinear boundary chains.
5. Implement provisional directed edge uses, grouping/twins, facet cycles/adjacency, vertex-link cycles, and orientability checks.
6. Implement shell connected components, exact bounds, and exact oriented volume.
7. Implement exhaustive source-facet pair relations with declared-feature reconciliation and all adjacent/non-adjacent/inter-shell rules.
8. Implement canonical incidence-graph labeling, ring normalization, and joint A/B original-feature ID assignment after embeddedness and before symbolic queries.
9. Implement exact shell point location, strict containment forest, depth polarity, orientation rejection, occupied-side metadata, provenance remapping, and canonicalized operand/artifact encodings.
10. Implement the independent verifier adapter, mutation tests, transaction/fault/cancellation behavior, and atomic publication.
11. Add deterministic generated/permutation/exhaustive-differential tests, then integrate strict CMake flags, standalone scripts, CTest, CI, release, and sanitizer jobs.

Do not begin Component 4 production integration until the Component 2 artifact contract tests and mandatory verifier pass. Component 4 may be developed against hand-built verified fixtures once its own prerequisites permit.

## 14. Validation and test matrix

### 14.1 Structural and coordinate tests

Run all applicable cases for all four `<T, I>` pairs:

- empty meshes with zero vertices and with finite unused vertices;
- mixed used/unused vertices and deterministic removed-source provenance;
- `+0`, `-0`, subnormal, minimum normal, maximum finite, and one-ULP-separated coordinates;
- every NaN/infinity sign/payload category relevant to supported encodings;
- out-of-range indices and injected checked-conversion/count/ID-capacity failures;
- zero-, one-, and two-entry facets;
- repeated closing index, consecutive duplicate including wraparound, repeated nonconsecutive index, and equal-coordinate distinct-index zero-length edge;
- deterministic first-error selection under worker schedules for a fixed raw input. For a permuted malformed input with one unique semantic defect, compare normalized subcode and geometric witness; raw locators, setup/replay digests, and the selected defect among several independent errors may legitimately change.

### 14.2 Exact facet and triangulation tests

- triangles, convex polygons, concave polygons, and polygons with long collinear boundary chains;
- first several vertices collinear but a later triple establishing a valid plane;
- exactly collinear/zero-area rings and a one-ULP non-planar vertex;
- self-crossing bow ties, endpoint self-touch, non-adjacent collinear overlap, and repeated-coordinate distinct-index contacts;
- dominant-axis ties for XY/XZ/YZ choices and both projected orientation parities;
- cyclic ring rotation invariance without winding reversal;
- ear choices with points on candidate-ear boundaries and narrow exact features;
- exact `n-2` triangle count, nonzero consistent triangles, legal diagonals, boundary coverage, paired internal edges, and exact area sum;
- triangulation producer faults and verifier detection.

### 14.3 Topology tests

- valid outward tetrahedron, triangular cube, six-polygon cube, octahedron, high-valence shell, and disconnected shells;
- one-use boundary edge, three-or-more-use edge, same-direction two-use edge, duplicate directed edge use, and duplicate/reversed facet;
- pinched vertex and bow-tie/two-cycle vertex links where every undirected edge still has exactly two uses;
- components touching only through equal coordinates, a vertex, or an edge, all rejected under shell policy rather than merged;
- facet adjacency and link reconstruction after arbitrary facet/ring/vertex permutation;
- supporting Euler-characteristic checks without treating them as manifold proofs.

### 14.4 Embeddedness, nesting, and orientation tests

- non-adjacent proper facet crossing; vertex-on-facet, edge-edge, and edge-facet tangency/contact;
- coplanar disjoint facets, legal coplanar shared edge, partial positive-area overlap, exact duplicate area, and adjacent facets intersecting beyond their shared feature;
- strict sibling shells, one cavity, nested outer/cavity/island chains, and multiple roots/children;
- distinct shells touching at a vertex, edge, or facet and coordinate-coincident duplicate shells, all rejected;
- one reversed facet, globally reversed root shell, outward-oriented cavity, and a correctly alternating nested orientation chain;
- containment witnesses on difficult projected alignments, symbolic-ray vertex/edge hits, and alternate symbolic ray invariance;
- exact oriented volume under translation and coordinate-axis permutation with the documented sign convention.

### 14.5 Canonicalization and determinism tests

For valid fixtures and generated solids:

- independently rotate each facet ring;
- permute raw vertex indices and remap faces;
- permute facets, shells, and disconnected component order;
- shuffle provisional records and worker shards; run thread budgets 1, 2, and a larger available count;
- require identical semantic canonical bytes, abstract feature-store order/IDs, canonicalized operand digests, and accepted/failure outcome;
- require each invocation's artifact/report/replay digests to bind correctly to its own order-sensitive `setup_digest`; do not require those invocation-bound digests or raw-locator diagnostics to match across input permutations;
- compare provenance as a one-to-one raw-source mapping, permit source-to-ID changes only within proven automorphism orbits, and verify provenance does not alter semantic operand digests;
- exercise highly symmetric tetrahedron/cube/polyhedron automorphisms and compare the canonical-label result with exhaustive labeling on bounded cases;
- verify signed-zero raw/input digests remain distinct while mathematically equivalent semantic exact coordinates canonicalize as documented;
- prove no hash iteration, allocation address, traversal order, or triangulation work order appears in bytes.

### 14.6 Property and differential tests

- Generate closed oriented polygonal shells from a fixed in-tree PRNG with serialized seed/type/operation-independent fixture metadata; do not use implementation-defined random distributions.
- Subdivide a planar facet or source edge without changing the occupied solid and verify validation succeeds with expected changed source topology and equivalent shell occupancy/orientation semantics; do not incorrectly require identical facet IDs across genuinely changed B-reps.
- Compare private self-query acceleration, once introduced, against exhaustive facet pairs and require no relation or first-failure difference.
- Compare producer adjacency, links, components, bounds, and canonical labeling against independent test or verifier reconstructions.
- Force Component 3 filter acceptance and exact fallback for the same fixtures and require byte-identical artifacts/evidence semantics.
- Serialize every generated failure by original coordinate bits, rings, seed, permutation, schedule, and replay descriptor.

### 14.7 Failure, transaction, and verifier tests

- A valid/B invalid and A invalid/B valid publish no slot and no constituent store.
- Inject memory, work, entity, exact-number, canonical-label, pair-count, evidence, diagnostic, and verifier limits at exact-bound and one-over-bound cases.
- Cancel before validation, between operands, during facet and pair loops, during canonical labeling, before verification, and before publication.
- Exercise every Component 1 fault point and require rollback of private records and reservations; only separately committed deterministic diagnostics/traces may remain.
- Reject wrong owner, role, slot, type tag, kernel digest, schema, stale report, changed invariant set, changed artifact digest, invalid ID, cross-role reference, and duplicate publication.
- Apply every mutation listed in Section 10 and require release/NDEBUG verification to reject it without relying on assertions.
- Verify successful stores remain readable and immutable after transaction destruction and carry no raw-mesh or scratch lifetime dependency.

### 14.8 Build and performance checks

- Build in strict C++17 with GCC and Clang, warnings enabled, and inspect effective flags to prove Component 2 is not compiled under fast-math.
- Run unit and bounded property tests in Debug and Release/NDEBUG.
- Run ASan/UBSan for malformed indexing, exact-value ownership, rollback, and mutation tests; run TSan for executor sharding, cancellation, diagnostics, and publication.
- Benchmark structural scan, exact facet audit, triangulation, exhaustive pair audit, point location, canonical labeling, and verifier separately. Report source sizes, pair counts, exact fallback counts, and peak accounted bytes.
- Performance optimization may add safe bounds or Component 4 self-query only after exhaustive differential equivalence. Never weaken exact predicates, endpoint rules, canonical labeling, evidence, or mandatory publication checks.

## 15. Completion criteria

Component 2 is complete only when all of the following hold:

- every raw index is checked before dereference and every used/unused finite coordinate has the documented exact/provenance treatment;
- every accepted source facet is exactly planar, simple, nonzero-area, and has a certified deterministic internal triangulation that preserves its public identity;
- every edge has exactly two opposite uses, every facet ring closes, every vertex link is one cycle, and every shell is connected, closed, and orientable;
- exhaustive exact source-level pair validation proves each shell embedded and all distinct shell boundaries disjoint under the fixed policy;
- the strict containment forest, nesting depth, exact orientation, and occupied side are unambiguous and agree with `outward_oriented_nested_shells` without mutating input winding;
- abstract semantic store bytes, canonicalized operand digests, and dense invocation-global ID structure are independent of input ordering, allocation, hash iteration, task partition, and thread schedule; on symmetric inputs the raw-source-to-ID bijection may vary only within a proven automorphism orbit as documented;
- the artifact owns exact/raw geometry, topology, shells, bounds, provenance, evidence, and canonical digests and exposes no mutable or raw-input-backed state;
- the independent mandatory verifier reconstructs all major incidence and geometry obligations, and every tested corruption prevents publication in release builds;
- every invalid input/resource/cancellation path returns deterministic structured evidence and leaves no partial operand or constituent store visible;
- all four supported template combinations, strict builds, Debug/Release, authoritative CTest/CI, and required sanitizer/property suites pass without external dependencies;
- public headers document enough of the immutable artifact, side convention, ID ownership, provenance, and failure contract for Components 4-13 to consume it without consulting legacy Boolean code.
