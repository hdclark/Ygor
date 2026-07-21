# Plan 02: Input Topology Validation and Shell Semantics

## 0. Scope and non-negotiable constraints

Implement **only Component 02** from `component_02_input_validation_shells.md`. This stage accepts one immutable Component 01 source snapshot at a time and publishes one immutable, independently verified `validated_operand<T,I>` containing canonical indexed topology, bounded facet evidence, a coherent input-realization certificate, disconnected-shell semantics, and complete diagnostic and replay evidence.

Component 02 must not:

- read the caller's mutable `fv_surface_mesh<T,I>` after Component 01 capture;
- inspect or validate `vertex_normals`, `vertex_colours`, `involved_faces`, or arbitrary `metadata`;
- mutate, repair, weld, snap, fill, split, simplify, delete, or reorient source topology;
- infer identity, adjacency, shell connectivity, or equivalence from coordinates or tolerance;
- spend the caller's cleanup tolerance during input validation;
- use operation-specific Boolean ownership or Component 07 symbolic rules to make one operand valid;
- publish source triangles, internal halfedges, collision candidates, intersection events, classifications, retained uses, output cycles, cleanup actions, public mesh output, or ordinary Boolean success;
- call `src/YgorMeshesBoolean{,2,3,4,5}*.{h,cc}` as production providers;
- use any external, vendored, downloaded, optional, runtime-invoked, or subprocess geometry dependency; or
- compile production or normative-test code outside the strict portable C++17 floating-point target established by Component 01.

Component 02 must use:

- Component 01 for immutable source snapshots, owner/type/policy validation, checked counts, strong-ID publication, typed outcomes and errors, resources, cancellation, deterministic arbitration, transactions, canonical bytes, SHA-256, replay, execution scopes, and immutable artifact publication; and
- Component 03 for source-point import, finite enclosures, bounded scalar/vector/plane operations, predicate dispositions, exact-nominal tie evidence, conservative bounds, construction conditioning, precision lineage, and verification capabilities.

No failed, cancelled, resource-exhausted, topology-only, partially checked, partially encoded, or verifier-rejected operand may publish an ordinary `validated_operand` eligible for Component 04.

`tracker.md` records completion of this planning/review step, not implementation completion. Mark Component 02 complete only after the reviewed specification and this plan are mutually consistent with Components 01, 03, 04, and 15.

## 1. Independent review conclusions and required corrections

The prior Component 02 plan had a strong topology, testing, and independent-verification foundation, but this review found integration defects that must not survive into implementation.

### 1.1 Match Component 01's exact V1 source contract

Component 01 captures only `vertices` and `faces` as semantic input. Therefore Component 02 receives no normals, colours, involved-face cache, or arbitrary metadata capability. It must not:

- reject malformed optional-array lengths;
- compare a stale `involved_faces` cache;
- preserve optional public fields as opaque Component 02 provenance;
- discover precision or shell semantics from metadata; or
- include ignored fields in source, semantic, replay, diagnostic, or canonical equality decisions.

Tests for ignored fields belong at the Component 01 public-capture boundary and as cross-stage metamorphic tests proving that Component 02 sees byte-identical source snapshots.

### 1.2 Remove the circular dependency between geometry and canonical identities

The prior ordering selected support planes and geometry witnesses before topology-dependent canonical IDs existed, while also requiring source-order-invariant proposal order and error selection. That is circular.

The corrected ordering is:

1. structural source validation and ring normalization;
2. temporary exact indexed incidence;
3. temporary vertex-link validation;
4. temporary shell discovery;
5. global topology canonicalization and strong-ID publication;
6. canonical incidence reconstruction;
7. bounded facet and shell geometry.

Before canonical IDs exist, local algorithms may use source positions only for storage access and presentation diagnostics. Semantic branch order and failure arbitration must use complete cyclic or graph encodings that exclude caller ordinals. Equal encodings represent an automorphism class and must not be broken by source order.

### 1.3 Do not overclaim a unique presentation mapping in symmetric topology

Canonical semantic bytes and canonical IDs must be invariant under presentation-only permutations. A unique caller-to-canonical bijection is not mathematically available for every exact automorphism class.

The implementation must distinguish:

- **semantic canonical labeling**, which is unique up to identical complete canonical encoding and determines artifact bytes and IDs; from
- **presentation correspondence**, which may choose any verified bijection within one exact automorphism class and is excluded from semantic bytes.

A presentation map must remain source-correct and total, but changing only the representative within an indistinguishable automorphism class must not change semantic content or primary failure bytes.

### 1.4 Require one coherent epsilon-validity certificate

Independent facet residual checks do not prove the broad plan's definition that one perturbation of each shared source vertex makes the entire mesh embedded and semantically consistent.

V1 must publish one of these explicit certificate dispositions:

- `nominal_embedded`: the exact nominal source coordinates themselves satisfy every required facet, local wedge, shell, and forbidden-intersection check within inherited uncertainty;
- `constructed_coherent_realization`: Component 02 constructs one deterministic realized point per referenced source vertex, each within its source precision envelope, and verifies all facets, edges, vertex neighborhoods, shells, and pair relations against that single shared realization; or
- `topology_only_nonpublishable`: explicitly diagnostic and ineligible for ordinary Component 04 processing.

Per-facet independent projections or per-facet independently adjusted vertices are not a coherent certificate.

### 1.5 Certify shell-boundary compatibility before final parentage

One interior probe can lie inside another shell even when the two boundaries cross. Therefore containment probes alone cannot establish nesting.

The implementation must first classify every potentially interacting shell pair as:

- definitely disjoint;
- strict containment candidate with boundary separation;
- authorized zero-volume point contact;
- authorized zero-volume edge contact;
- conditionally authorized face contact with unique occupancy;
- forbidden transverse intersection;
- forbidden coplanar positive-area overlap;
- unsupported whole-shell coincidence;
- positive-volume overlap; or
- ambiguous.

Only pair records proven compatible may participate in parent selection.

### 1.6 Correct occupied-side semantics

Under the orientation convention used throughout the broad plan:

- an outward-oriented exterior boundary has empty space on its positive/outward side and occupied material on its negative/inward side;
- a correctly reversed cavity boundary also has occupied material on its negative side and empty space on its positive side.

Thus every accepted oriented boundary stores `occupied_side == negative` and `empty_side == positive`. Nesting parity determines the required geometric orientation of the shell; it does not reverse this side-label convention.

### 1.7 Define a Component 02 validation relation kernel

Component 03 supplies bounded arithmetic and primitive predicates. Component 07 later supplies operation-specific relation and symbolic ownership semantics. Component 02 still needs a narrow, versioned validation relation layer for:

- projected segment/segment relations;
- triangle/triangle relations, including coplanar cases;
- point-against-shell winding or parity classification;
- local edge and vertex wedge compatibility; and
- shell-pair boundary compatibility.

This kernel may share pure formula dispatch and bounded predicate adapters with Component 04, but it must not use Component 07 Boolean operation ownership, create intersection events, or publish downstream relation IDs.

### 1.8 Keep isolated vertices outside semantic topology

Finite isolated source vertices remain in Component 01 exact source snapshots and replay. They do not receive Component 02 canonical topology IDs, shell membership, or Component 04 records. This avoids making semantically empty geometry depend on unused presentation data while retaining exact reproducibility.

### 1.9 Clarify duplicate-coordinate acceptance

Distinct source indices with identical coordinate bits remain distinct identities. This rule prohibits welding; it does not force acceptance of collapsed geometry. A zero-extent source edge, non-adjacent coordinate-coincident corner, zero-area local wedge, or incompatible overlapping sheet must receive a bounded geometric disposition and may fail.

### 1.10 Reuse existing Ygor narrowly and deliberately

Existing Ygor facilities are useful but not authoritative Component 02 providers:

- `fv_surface_mesh<T,I>` and `vec3<T>` remain public or nominal carriers only;
- `YgorMeshesAdaptivePredicates` may be improved and consumed only through Component 03's strict bounded interface;
- `YgorMeshesVerification` supplies useful regression concepts for finite checks, index checks, undirected edge counts, and orientation examples, but its triangular-only boolean/exception APIs do not validate polygon rings, reciprocal uses, vertex links, canonical identities, precision, shell semantics, or reconstructible evidence;
- generic contour, plane, mesh cleanup, triangulation, BSP, and Boolean helpers are not production providers because they use incompatible tolerance, identity, mutation, or error contracts.

Do not duplicate generally useful index-only algorithms unnecessarily. A small bounded-subsystem incidence utility may be factored for Components 02, 05, and 15 only after its strong-ID, resource, determinism, and verifier contracts are explicit.

## 2. Existing Ygor assessment and mandatory reuse decisions

### 2.1 Reuse unchanged as narrow carriers

Reuse:

- `fv_surface_mesh<T,I>` only at the Component 01 public boundary and Component 14 output boundary;
- `vec3<T>` only as a nominal three-coordinate carrier at audited interfaces;
- fixed-width integer types and C++17 standard-library containers and algorithms under Component 01 resource and ordering contracts; and
- Component 01/03 bounded-subsystem infrastructure.

Never use `vec3::operator==`, `operator<`, `Dot`, `Cross`, `unit`, length, distance, angle, or text conversion for authoritative identity, ordering, or acceptance.

### 2.2 Improve adaptive predicates through Component 03

Component 03's plan refactors the in-tree expansion-arithmetic approach into a strict-target, enclosure-aware provider. Component 02 may request exact-nominal sign or tie evidence only through Component 03. It must also use the full bounded predicate disposition; exact nominal sign alone cannot authorize a decision whose inherited uncertainty crosses zero.

### 2.3 Use `YgorMeshesVerification` only as a differential reference

Do not call `ValidateClosedTriangularMesh`, `IsClosedManifold`, or `HasConsistentOrientation` as an acceptance gate. They are insufficient because they:

- are triangular-oriented;
- expose booleans or exceptions rather than typed evidence;
- count undirected uses without proving reciprocal directed pairing;
- do not validate one cyclic vertex link;
- do not support polygon planarity or simplicity;
- do not carry input precision or coherent realization evidence;
- do not establish shell nesting or occupied sides; and
- do not publish canonical immutable artifacts.

For triangular fixtures, tests may compare their coarse answer with Component 02 after constraining expectations. Any disagreement must be explained by the stronger Component 02 contract, not silently normalized away.

### 2.4 Do not reuse mutating or unbounded geometry helpers

Production Component 02 must not call:

- `fv_surface_mesh::merge_duplicate_vertices`;
- `convert_to_triangles`;
- `remove_degenerate_faces`;
- `remove_disconnected_vertices`;
- `simplify_inner_triangles`;
- `compute_vertex_normals`;
- `slice_with_planes`;
- `recreate_involved_face_index` or `apply_involved_face_index_diff`;
- contour duplicate/extraneous-point removal;
- legacy point-in-polygon tolerance helpers;
- raw plane/line intersection helpers;
- random surface sampling; or
- any legacy mesh Boolean implementation.

### 2.5 New code versus shared bounded utilities

Implement Component 02 as bounded-subsystem code under `src/YgorMeshesBooleanBounded/`. Shared code is permitted only for pure, versioned, owner-checked functionality whose contract is common:

- checked integer incidence grouping;
- bounded projected polygon primitive relations;
- conservative closed-AABB primitives supplied by Component 03; and
- canonical byte/key helpers supplied by Component 01.

Do not share producer orchestration, shell parent selection, artifact schemas, or verifier control flow.

## 3. Exact file and target layout

Add or complete these files under `src/YgorMeshesBooleanBounded/`:

- `ValidatedOperand.h` — immutable artifact schema and checked read-only views;
- `InputValidationTypes.h` — stable enums, certificate dispositions, witness records, temporary handles, and capability declarations;
- `InputValidation.h/.cc` — top-level typed entrypoint and fixed phase orchestration;
- `InputRingNormalization.h/.cc` — source-ring structural normalization and reversible source-position records;
- `SourceIncidence.h/.cc` — temporary and canonical directed/undirected source-edge records;
- `VertexLinkValidation.h/.cc` — producer corner-link traversal and one-cycle validation;
- `CanonicalSourceTopology.h/.cc` — colored-graph encoding, refinement, automorphism handling, canonical IDs, and presentation correspondences;
- `InputFacetGeometry.h/.cc` — support planes, fixed projection records, simplicity, orientation, and private facet decomposition;
- `InputRealization.h/.cc` — coherent realization certificate construction and validation;
- `InputGeometryRelations.h/.cc` — versioned validation-only segment, triangle, wedge, point/shell, and shell-pair relation logic;
- `ShellSemantics.h/.cc` — shell discovery records, intrinsic orientation, compatibility graph, nesting, parity, occupied sides, and winding;
- `InputGeometryValidation.h/.cc` — scalable whole-operand forbidden-interaction and local compatibility search;
- `ValidatedOperandCodec.h/.cc` — canonical semantic encoding, source linkage, and digest calculation; and
- `ValidatedOperandVerifier.h/.cc` — independent reconstruction and mutation rejection.

Where Components 02 and 04 need identical primitive projected predicates, add:

- `BoundedSourcePolygonKernel.h/.cc` — pure formula-dispatched Component 03 adapters for orientation, segment relation, local cone, and point-in-triangle/polygon predicates.

This shared kernel must not choose canonical IDs, ears, decompositions, parents, shells, errors, or artifact ownership. Component 02 and Component 04 must retain independent orchestration and verification.

Extend existing bounded-subsystem registries rather than creating parallel registries:

- `ContractVersions.h`;
- Component 01 stage/checkpoint registry;
- Component 01 error category/subcode registry;
- Component 01 resource-kind registry where a genuinely distinct counted entity is required; and
- Component 01 canonical encoding and replay schema registries.

Add normative tests under `tests/mesh_boolean_bounded/`:

- `TestInputValidationUnit.cc`;
- `TestInputSourceContract.cc`;
- `TestInputTopology.cc`;
- `TestInputCanonicalization.cc`;
- `TestInputFacetGeometry.cc`;
- `TestInputCoherentRealization.cc`;
- `TestInputGeometryRelations.cc`;
- `TestInputShellSemantics.cc`;
- `TestInputEpsilonValidity.cc`;
- `TestInputValidationMutation.cc`;
- `TestInputValidationProperties.cc`;
- `TestInputValidationAdversarial.cc`;
- `TestInputValidationFuzz.cc`;
- `InputValidationFixtures.h/.cc`;
- `InputValidationExactOracle.h/.cc`; and
- `InputValidationTestProvider03.h/.cc` only until the production Component 03 capabilities are available.

Register focused CTest cases for source contract, structural topology, canonicalization, facet geometry, coherent realization, relation kernel, shell semantics, epsilon validity, mutation, properties/fuzz, and adversarial/resource behavior. Apply Component 01's strict floating-point helper to every production and normative-test translation unit.

## 4. Stable versions, stages, checkpoints, and failure subcodes

### 4.1 Version registry

Add explicit nonzero V1 values for:

- input-validation provider;
- normalized-ring schema;
- source-incidence schema;
- vertex-link schema;
- source-topology canonicalizer;
- presentation-correspondence schema;
- input-facet geometry provider;
- coherent-realization certificate schema/provider;
- validation relation-kernel formula set;
- shell-pair relation schema/provider;
- shell-semantics schema/provider;
- input-geometry assessment schema/provider;
- validated-operand artifact;
- validated-operand canonical encoding; and
- validated-operand verifier.

Zero is invalid/uninitialized. Unknown required versions, unsupported enum values, nonzero reserved fields, or predecessor version mismatches are typed failures. Include all relevant versions in artifact bytes, diagnostic evidence, replay, and verifier checks.

### 4.2 Fixed logical checkpoints

Use the Component 02 operand-A and operand-B stages reserved by Component 01. Define stable checkpoints in this order:

1. capability, owner, operand, type, policy, and predecessor-version validation;
2. source count, byte, ID-capacity, resource, and work preflight;
3. source scalar finiteness and index-range validation;
4. index-only ring normalization;
5. temporary directed/undirected edge-incidence construction;
6. temporary vertex-link validation;
7. temporary topological shell discovery;
8. global canonical source-topology labeling and automorphism record construction;
9. canonical incidence, link, and shell reconstruction;
10. Component 03 source-point import;
11. canonical facet support-plane and projection selection;
12. coherent realization certificate construction or nominal certificate selection;
13. facet simplicity, nonzero orientation, and validation-only decomposition;
14. intrinsic shell orientation evidence;
15. shell-pair boundary relation and compatibility prepass;
16. shell containment, parent forest, depth, orientation parity, occupied sides, and winding;
17. whole-operand epsilon-validity and local-wedge assessment;
18. proposed immutable artifact construction;
19. independent validated-operand verification;
20. canonical encoding, digest, replay, and exact resource finalization; and
21. pre-publication cancellation poll and atomic transaction commit.

Do not renumber released checkpoints. Future optional work requires reserved checkpoint values or a schema/provider version change.

### 4.3 Required Component 02 subcodes

Allocate a disjoint Component 02 range and define explicit stable subcodes for at least:

**Capability/source failures**

- wrong or stale owner/operand;
- unsupported type/source layout;
- predecessor version mismatch;
- source count/range/byte overflow;
- source resource/work preflight exceeded;
- non-finite source coordinate;
- out-of-range source index;
- topology-only mode not permitted for ordinary publication.

**Ring/topology failures**

- empty or undersized normalized ring;
- repeated non-consecutive source identity;
- topological self-edge;
- duplicate directed edge use;
- open boundary edge;
- non-manifold edge-use count;
- same-direction paired uses;
- corrupt reciprocal source location;
- vertex-link open chain;
- vertex-link multiple cycles;
- vertex-link repeated incidence;
- vertex-link degree mismatch;
- shell membership contradiction.

**Canonicalization failures**

- canonical labeling work exhausted;
- canonical key collision with unequal full bytes;
- invalid automorphism-class record;
- non-dense/wrong-domain canonical ID;
- presentation correspondence not bijective within allowed class;
- semantic encoding depends on source order.

**Facet/realization failures**

- source bounded point unavailable;
- support plane unavailable;
- planarity definitely exceeded;
- planarity uncertainty unresolved;
- coherent realization infeasible;
- coherent realization resource/work exhausted;
- realization displacement exceeds input precision;
- realization shared-vertex inconsistency;
- stable projection unavailable;
- projected proper crossing;
- projected non-adjacent touch;
- projected collinear overlap;
- projected relation uncertain;
- facet orientation zero, reversed, or uncertain;
- private decomposition failed;
- private decomposition coverage contradiction.

**Shell/relation failures**

- shell intrinsic orientation unavailable;
- shell-pair transverse intersection;
- shell-pair unsupported coplanar overlap;
- shell-pair positive-volume overlap;
- unsupported whole-shell coincidence;
- shell contact semantics ambiguous;
- shell parent ambiguous;
- shell containment cycle;
- shell orientation conflicts with depth parity;
- occupied-side convention mismatch;
- total winding outside zero/one model.

**Whole-geometry failures**

- forbidden non-adjacent interaction;
- authorized adjacency exceeded shared feature;
- local edge wedge incompatible;
- local vertex star incompatible;
- collapsed or inverted source geometry;
- uncertainty spans incompatible topology;
- declared input precision contradicted;
- validation spatial-index false-negative detected by verifier/test mode.

**Artifact failures**

- validated-operand verifier rejection;
- canonical bytes/digest mismatch;
- source/presentation map inconsistency;
- underreported precision/resource evidence.

Map malformed indexed topology and strict-policy orientation mismatch to `input_contract_error`; definite or unresolved geometric incompatibility to `input_geometry_not_epsilon_valid`; non-unique nesting/contact/occupancy to `ambiguous_shell_semantics`; capacity to `index_overflow`; accounting/work limits to `resource_limit`; cancellation to `cancelled`; and producer/verifier contradictions to `internal_invariant_error`.

Expected geometric uncertainty must never become `internal_invariant_error` merely because one producer path expected a definite answer.

## 5. Top-level API and capability contract

Implement an internal entrypoint conceptually equivalent to:

```cpp
template<class T, class I>
boolean_outcome<artifact_handle<const validated_operand<T,I>>>
validate_operand(
    operand_id operand,
    immutable_source_mesh_view<T,I> source,
    boolean_context_view<T,I> context,
    precision_context_view<T> precision,
    input_validation_capabilities<T,I> capabilities);
```

`input_validation_capabilities` must contain narrow owner-checked views for:

- checked counts and index conversion;
- resource reservations, slices, and final leases;
- cancellation and deterministic progress;
- diagnostic finding collection and primary-error arbitration;
- stage transaction and immutable publication;
- canonical ID proposal/publication;
- canonical bytes, SHA-256, and replay accumulation;
- deterministic serial or Component 17 execution scopes; and
- Component 03 source import, bounded operations, predicates, conditioning, bounds, precision lineage, and verifier APIs.

The validator must not receive the complete mutable top-level invocation object.

Before source access, validate:

- context/source/precision owner equality;
- correct operand role;
- supported V1 `T` and `I` profile;
- all source, context, precision, and provider versions;
- strict floating environment already established;
- frozen input precision and verification policy;
- transaction open for the correct operand stage;
- ordinary-success eligibility versus topology-only mode; and
- resource/cancellation capability ownership.

Return exactly one verified immutable artifact or one typed error. Do not log, throw an expected error, partially publish, or mutate predecessor artifacts.

## 6. Immutable artifact schema

### 6.1 Referenced source vertex record

Publish one record only for a vertex referenced by a retained normalized ring. Store:

- canonical `source_vertex_id`;
- owner and operand;
- exact x/y/z source bits;
- Component 03 source bounded-point and precision-lineage references;
- coherent-realization point/reference and displacement certificate when applicable;
- canonical incident corner, directed-use, facet, edge, and shell ranges;
- presentation correspondence class/reference; and
- canonical record encoding contribution.

Do not publish isolated source vertices as semantic records.

### 6.2 Facet/ring/corner records

For each accepted source facet, store:

- canonical `source_facet_id`, `source_ring_id`, and corner range;
- orientation-preserving canonical ring sequence;
- total source-position normalization actions in the presentation section;
- support-plane construction/formula/evidence;
- fixed projection frame and orientation mapping;
- planarity, simplicity, and nonzero-area witnesses;
- validation-only decomposition and boundary/coverage evidence;
- shell membership; and
- canonical bytes.

V1 supports one simple contour per source facet. Ring reversal is prohibited under strict policy.

### 6.3 Directed/undirected source-edge records

Each directed use stores origin, destination, facet, ring, corner, previous/next use, reciprocal use, undirected edge, shell, and source-presentation references.

Each undirected edge stores ordered endpoint IDs and exactly two opposite uses. Endpoint order is by strong identity, not coordinates.

### 6.4 Vertex-link records

For every referenced source vertex, store:

- canonical incident-corner set;
- producer cyclic order and convention;
- predecessor/successor evidence;
- degree and count evidence;
- representative automorphism class when symmetric; and
- verifier reconstruction references.

### 6.5 Coherent realization certificate

Store one operand-level certificate with:

- disposition (`nominal_embedded`, `constructed_coherent_realization`, or `topology_only_nonpublishable`);
- certificate provider/formula versions;
- one realized point reference for every referenced source vertex;
- proof that each realization enclosure lies inside the source input-precision envelope;
- shared-use proof that every incident facet consumes the same vertex realization;
- all facet plane/orientation, edge-wedge, vertex-star, shell, and forbidden-interaction checks evaluated against the certificate;
- maximum realized input adjustment and canonical witness set;
- no cleanup-budget charge; and
- verifier reconstruction data.

A constructed realization is validation evidence, not a source edit. Components 04 and later continue to preserve source coordinate/precision lineage according to their contracts; they may consume the realized certificate only where explicitly specified.

### 6.6 Shell records

Each shell record contains:

- canonical `source_shell_id`;
- canonical member ranges for vertices, edges, corners, facets, and rings;
- complete structural encoding and representatives;
- conservative bounds;
- intrinsic orientation evidence;
- canonical interior/exterior side probes;
- pairwise shell relation references;
- parent or exterior-root status;
- depth;
- required geometric orientation parity;
- `occupied_side == negative` and `empty_side == positive`;
- winding evidence;
- geometry-assessment summary; and
- resource statistics.

### 6.7 Operand-level record

`validated_operand<T,I>` contains:

- owner, operand, type descriptors, artifact/provider versions;
- Component 01 source snapshot digest and Component 03 precision context link;
- all canonical semantic records above;
- dense-ID range descriptors;
- shell compatibility graph and nesting forest;
- coherent-realization certificate;
- geometry-verification disposition;
- semantic canonical bytes/digest;
- presentation correspondence and source-location maps in a separate non-semantic section;
- complete precision, diagnostic, replay, and resource references; and
- local verifier disposition.

Semantic bytes exclude caller ordinals and presentation-choice representatives. Source/replay data remain in Component 01 and linked presentation sections.

## 7. Transactional validation workflow

Execute one Component 02 stage transaction per operand with private mutable workspace.

1. Validate capabilities and versions.
2. Preflight all source/derived counts, bytes, ID capacity, candidate upper bounds, diagnostic capacity, replay contribution, and work.
3. Reserve mandatory resources before allocations.
4. Validate finite source coordinate bits and all indices.
5. Normalize rings using indices only.
6. Build temporary directed/undirected incidence.
7. Validate temporary vertex links.
8. Discover temporary topological shells.
9. Build the topology-only colored graph, canonicalize it, and publish dense canonical IDs.
10. Reconstruct all incidence, links, and shell membership in canonical order; verify temporary-to-canonical correspondence.
11. Import referenced source points through Component 03.
12. Select support planes and projection frames in canonical order.
13. Establish the coherent realization certificate.
14. Validate facet simplicity, orientation, and private decomposition against the coherent certificate.
15. Establish intrinsic shell orientation.
16. Classify shell-pair boundaries and build the compatibility graph.
17. Establish strict containment, parent forest, depth, orientation parity, occupied sides, and winding.
18. Run whole-operand forbidden-interaction and local-wedge validation.
19. Construct a proposed immutable artifact.
20. Independently verify the proposal.
21. Re-encode canonical bytes, compute digests, reconcile exact resources, and promote persistent leases.
22. Poll cancellation immediately before commit.
23. Atomically publish or roll back all private state.

A phase may not continue when its prerequisites are invalid. Within one phase, collect a bounded set of independent failures and select Component 01's canonical minimum. Long loops poll cancellation and charge work at deterministic range boundaries.

## 8. Source preflight and index-only normalization

### 8.1 Count and resource preflight

Checked preflight must include:

- source vertex and facet counts;
- total original and maximum normalized ring positions;
- directed use, undirected edge, corner, and link-arc maxima;
- shell maxima;
- topology canonicalization graph nodes/edges/refinement records;
- Component 03 source imports;
- support-plane proposals and fallback work;
- private decomposition triangles, at most `sum(max(0,n-2))`;
- realization variables/constraints/work;
- facet-segment and triangle spatial-index nodes/candidates;
- shell-pair candidates and relation findings;
- verifier-owned reconstruction/index work;
- diagnostic/replay bytes; and
- persistent and peak temporary bytes.

Preflight arithmetic must detect overflow before host allocation. A correctly reserved `std::bad_alloc`, `length_error`, or container-capacity rejection is translated through Component 01's reviewed boundaries.

### 8.2 Empty operands and isolated vertices

An empty facet sequence is the empty regular solid. Referenced topology arrays are empty. Finite isolated source points remain only in the Component 01 snapshot/replay and do not affect Component 02 semantic bytes.

A non-empty facet list containing an empty or malformed ring is not equivalent to an empty solid and must fail.

### 8.3 Ring normalization

For each facet:

1. checked-copy source indices;
2. remove consecutive identical source indices;
3. remove one duplicate closing source index;
4. require at least three retained positions and distinct identities;
5. reject any remaining repeated identity;
6. create temporary corners and directed uses;
7. preserve direction; and
8. create total source-position action records.

Do not rotate to a source-order-selected minimum at this phase. Build an orientation-preserving cyclic semantic encoding for canonicalization. After global IDs exist, rotate to the least full canonical corner-key sequence. If several rotations are exactly identical, record the automorphism class rather than selecting semantic content from caller order.

## 9. Exact topology validation

### 9.1 Temporary edge incidence

Group directed-use proposals by unordered temporary vertex identity. Temporary identities may reference source positions internally, but canonical equality and failure keys must include complete local semantic structure rather than raw ordinals.

Require exactly two opposite uses per undirected edge. Validate ring location, previous/next connectivity, and reciprocal endpoint reversal.

### 9.2 Vertex-link validation

For a corner at source vertex `v` with incoming edge `u->v` and outgoing edge `v->w`:

- cross the reciprocal outgoing edge to locate the adjacent facet corner at `v` as successor;
- cross the reciprocal incoming edge for predecessor;
- verify exactly one predecessor and successor per incident corner; and
- prove one cycle covers all incident corners.

Reject open chains, repeated incidence, early return, unvisited corners, multiple cycles, or degree mismatch.

The independent verifier must reconstruct an undirected link graph from published ring/edge records, prove every node degree two, and prove connectedness, rather than calling the producer successor traversal.

### 9.3 Topological shell discovery

Discover connected components through paired source-edge adjacency only. Coordinate contact never joins shells.

For every temporary shell, collect full member sets and a topology-only structural encoding. Every referenced feature must belong to exactly one shell. Isolated source points belong to none.

## 10. Canonical source topology and automorphisms

### 10.1 Colored incidence graph

Construct a typed colored graph with node domains for:

- referenced source vertices;
- facets/rings;
- corners;
- directed edge uses;
- undirected edges; and
- temporary shells.

Initial semantic colors contain:

- domain and operand role;
- exact coordinate bits for referenced vertices;
- ring length and orientation-preserving role;
- typed incidence roles; and
- no caller ordinal, pointer, allocation position, task identity, or hash bucket state.

### 10.2 Refinement and individualization

Perform deterministic partition refinement over full canonical encodings of prior color and sorted typed neighbor-color multisets. Hashes may accelerate but never establish equality.

For unresolved cells, run bounded individualization/refinement:

1. choose a cell by complete stable cell key;
2. explore members by complete structural branch key;
3. refine after each choice;
4. encode the complete labeled graph;
5. retain the lexicographically least complete encoding; and
6. charge work/resources before each branch block.

If branches are indistinguishable and produce identical complete encodings, retain one semantic labeling plus an automorphism-class record. Do not use caller order to choose semantic bytes.

### 10.3 Dense IDs and presentation correspondence

Publish dense Component 01 strong IDs from the winning semantic labeling. Rebuild all records using only canonical IDs.

For each exact automorphism class, presentation correspondence may choose any verified bijection that:

- maps every retained caller source position to one compatible canonical record;
- preserves coordinates, facet rings, incidence, and typed roles;
- has an inverse over retained records; and
- is excluded from semantic canonical bytes.

Diagnostics may display caller positions, but primary semantic error arbitration must use canonical class evidence. Symmetric equivalent failures are coalesced into one class witness.

## 11. Bounded facet geometry and coherent realization

### 11.1 Canonical source-point import

After canonical IDs exist, import every referenced source point through Component 03. Verify nominal bits, inherited input precision, source provenance, owner, and finite enclosure. No source point may be imported twice under conflicting precision.

### 11.2 Support-plane proposals

For each canonical ring, evaluate a fixed provider-versioned proposal family:

- canonical consecutive triples;
- triples maximizing certified separation and area lower bounds;
- deterministic Component 03 fallback formulations; and
- bounded all-triples fallback in canonical combination order when required.

Rank certifiable proposals by:

1. greatest certified normal-magnitude lower bound;
2. smallest plane-uncertainty upper bound; and
3. least complete canonical feature key.

A work-limit failure is `resource_limit`, not planarity invalidity.

### 11.3 Coherent realization strategies

Attempt strategies in a fixed versioned order.

#### Strategy A: nominal embedded certificate

Use one nominal source point per referenced canonical vertex. Certify:

- every facet against its support plane and projection;
- every retained ring simple and nonzero;
- every edge wedge and vertex star compatible;
- every shell intrinsically oriented;
- every shell pair compatible;
- no forbidden non-adjacent interaction; and
- all decisions conservative under inherited source uncertainty.

If all pass, publish `nominal_embedded`.

#### Strategy B: constructed coherent realization

When permitted by the V1 certificate policy, construct one realized point per referenced vertex under explicit Component 03 bounded constraints. A conforming V1 implementation may use a deterministic bounded active-set or projection scheme, but it must specify:

- variables: three coordinates per referenced source vertex;
- source-box/radial constraints: every realized point remains inside its input precision envelope;
- facet constraints: all vertices of one facet satisfy one selected realized support plane within the certificate's exact acceptance rule;
- nondegeneracy constraints: accepted facet orientations, source-edge extents, edge wedges, and vertex stars stay definite;
- shared-variable constraint: one source vertex has exactly one realization across all incident facets;
- deterministic constraint and pivot order by canonical IDs;
- bounded iteration/work limits;
- conservative residual and displacement verification through Component 03; and
- fail-closed handling when feasibility or conditioning cannot be certified.

Do not solve each facet independently and average results. Do not use an unconstrained least-squares fit whose residual is merely below user tolerance. Do not claim a complete arbitrary perturbation decision procedure.

After construction, independently re-evaluate all required topology-compatible geometry on the same realized points. Publish only if every realized point is within input precision and all global checks pass.

If V1 elects not to implement Strategy B initially, it may conservatively accept only Strategy A and reject other potentially valid inputs. The artifact/provider version must state that reduced sufficient domain explicitly; it must not claim general epsilon-validity.

### 11.4 Fixed projection and ring simplicity

Choose a coordinate-drop projection from the selected bounded plane normal by greatest certified absolute-component lower bound, then distortion upper bound, then fixed X/Y/Z order.

Using the coherent certificate points, classify ring-segment pairs:

- consecutive segments may meet only at their shared source identity;
- first/last are consecutive;
- non-consecutive proper crossing is invalid;
- non-consecutive point touch is invalid;
- collinear overlap is invalid;
- adjacent overlap beyond the shared endpoint is invalid; and
- unresolved relation uncertainty fails.

Build a deterministic segment spatial index for scalable enumeration and gate it against exhaustive pairs in tests.

### 11.5 Facet orientation and private decomposition

Compute bounded projected area with prescribed translation reduction and fixed pairwise reduction. Require a definite nonzero sign consistent with source orientation.

Construct a private no-Steiner decomposition only for validation queries. Preserve all source boundary uses, choose definite ears in canonical key order, and verify:

- triangle orientation;
- reciprocal internal diagonal uses;
- boundary cancellation;
- connected triangle dual;
- no forbidden triangle overlap; and
- source-region coverage.

These private triangles are not Component 04 source triangles and receive no source-triangle IDs.

## 12. Validation relation kernel

### 12.1 Scope

`InputGeometryRelations` is a Component 02 semantic layer over Component 03 primitives. It returns immutable validation findings, not Component 07 canonical Boolean relations.

It must define complete result enums and evidence for:

- projected segment/segment relation;
- bounded triangle/triangle relation;
- point/boundary and point/shell classification;
- incident edge wedge relation;
- incident vertex-star relation; and
- shell-pair boundary relation.

Each relation is computed once per canonical Component 02 key and reused by all Component 02 consumers.

### 12.2 Segment relations

Distinguish at least:

- disjoint;
- authorized shared endpoint;
- proper crossing;
- non-adjacent touch;
- collinear disjoint;
- collinear endpoint touch;
- collinear positive-length overlap; and
- uncertain.

Topology determines whether a shared endpoint is authorized. Coordinate equality does not.

### 12.3 Triangle relations

For each candidate pair, first classify plane-side intervals, then use the required noncoplanar or coplanar formulation. Distinguish:

- definitely disjoint;
- authorized shared source feature only;
- zero-volume point contact;
- zero-volume segment contact;
- proper transverse intersection;
- coplanar positive-area overlap;
- whole-patch coincidence;
- containment/overlap relevant to shell semantics; and
- uncertain.

For coplanar pairs, use a canonical common projection and bounded 2D segment/containment tests. Do not use centroid-only tests or nominal epsilon.

### 12.4 Point-against-shell classification

Use a fixed versioned sequence of rational ray directions, deterministic conservative candidate enumeration, bounded ray/triangle predicates, and a fixed half-open rule. Reuse each ray/triangle relation within the classification record.

Return definite winding/parity, boundary overlap, ambiguous, resource failure, or cancellation. Never guess. A probe enclosure intersecting a boundary is not definitely inside or outside.

### 12.5 Local wedge/star compatibility

For each source edge, verify its two incident realized facet sides form a locally embedded two-manifold wedge and meet only on the authorized source edge.

For each source vertex, verify incident realized triangles in cyclic link order form one locally embedded disk neighborhood and do not cross or invert outside authorized shared edges/vertex.

These checks complement indexed manifoldness; they must detect a geometrically folded or self-overlapping star that retains valid edge counts and link topology.

## 13. Shell orientation, compatibility, nesting, and occupied sides

### 13.1 Intrinsic orientation

For each shell, compute independent bounded evidence:

- a translated, fixed-tree signed-volume enclosure over private decomposition triangles; and
- one or more deterministic two-sided facet probes classified against the same shell.

Positive definite signed volume denotes outward orientation under the repository's right-handed convention. If both evidence paths are definite, require agreement. If neither certifies orientation, fail. Store intrinsic orientation without yet deciding whether it is correct for nesting depth.

### 13.2 Shell-pair candidate enumeration

Build a deterministic conservative spatial index over shell and private-triangle bounds. Bounds may prune definitely separated pairs but never prove containment.

For every potentially interacting shell pair, create one canonical pair record and classify all boundary interactions. Verify exhaustive equality for bounded tests.

### 13.3 Shell-pair compatibility result

Publish exactly one relation per pair:

- `definitely_disjoint`;
- `strict_containment_possible_a_in_b` with boundary separation;
- `strict_containment_possible_b_in_a` with boundary separation;
- `authorized_point_contact`;
- `authorized_edge_contact`;
- `authorized_face_contact_unique_occupancy`;
- `forbidden_transverse_intersection`;
- `forbidden_coplanar_positive_area_overlap`;
- `positive_volume_overlap`;
- `unsupported_whole_shell_coincidence`; or
- `ambiguous`.

A face-contact acceptance requires proof that no positive-volume overlap exists and that occupied-side semantics are unique. Whole-shell coincidence is rejected in V1.

### 13.4 Interior probes and parent candidates

Generate a canonical certified interior probe for every shell, preferably from an accepted inward-side facet probe. If its enclosure intersects any shell boundary, try the next canonical candidate.

A potential parent must satisfy both:

- a compatible shell-pair boundary relation with strict separation; and
- a definite inside classification of the child probe.

Point, edge, or face contact never establishes parentage by itself.

Select the unique immediate containing shell using complete pairwise containment records. Reject incomparable parents, missing required comparisons, cycles, or ambiguity.

### 13.5 Depth, parity, and side convention

Assign exterior roots depth zero and propagate depth through the acyclic parent forest.

Under strict V1:

- even-depth shells must be intrinsically outward oriented;
- odd-depth shells must be intrinsically inward/reversed relative to their parent solid;
- no shell is silently reversed;
- every accepted oriented boundary stores occupied material on its negative side; and
- every accepted boundary stores empty space on its positive side.

Verify representative cells and pair relations so total oriented winding away from boundaries is exactly zero or one. Reject overlapping additive siblings, contradictory cavities, or winding two.

## 14. Whole-operand epsilon-validity assessment

### 14.1 Verification dispositions

Support:

- `required_scalable`;
- `required_scalable_with_diagnostics`;
- `exhaustive_test_only`; and
- internal `topology_only_nonpublishable`.

There is no ordinary-success `none` mode.

### 14.2 Deterministic validation spatial index

Build a private deterministic index over private validation triangles and shell bounds using closed Component 03 bounds inflated by inherited precision/certificate uncertainty. Specify axis choice, full sort key, split rule, node numbering, and closed-overlap behavior.

The index may emit false positives but no false negatives. Exhaustive test mode compares every pair.

### 14.3 Topological exclusions

Exclude or narrow pairs only from source identity:

- triangles of one private facet decomposition are governed by that facet's coverage proof;
- adjacent facets may meet on their exact shared source edge only;
- facets sharing one source vertex may meet on that source occurrence only;
- non-adjacent facets have no authorized contact; and
- separate shells gain no authorization from coordinate coincidence.

### 14.4 Mandatory certificates

Ordinary acceptance requires proof that, under the one coherent realization:

1. every facet is simple, nonzero, and orientation-compatible;
2. every source edge has positive certified extent or an explicitly supported noncollapsed representation;
3. every edge wedge and vertex star is locally embedded;
4. no non-adjacent source facets intersect transversely;
5. no unsupported coplanar positive-area overlap exists;
6. authorized adjacent contact is confined to the exact shared source feature;
7. shell-pair relations match the compatibility graph;
8. nesting and orientation parity produce unique occupied sides;
9. no uncertainty spans incompatible topology or occupancy; and
10. every realization displacement remains within input precision without cleanup-tolerance expenditure.

This is a conservative sufficient certificate, not a complete arbitrary perturbation-existence algorithm. Difficult valid geometry may fail. Known contradictions may not pass.

## 15. Canonical encoding and digests

Use Component 01 canonical bytes. Never encode object memory, padding, pointers, `size_t`, native enum layout, locale text, container capacity, compiler strings, timestamps, or presentation-selected automorphism representatives.

Encode semantic content in this order:

1. magic, artifact schema, provider/formula versions;
2. context owner digest, operand, and type profile;
3. source snapshot digest and precision-context digest/reference;
4. geometry-verification and coherent-certificate dispositions;
5. canonical referenced source vertices;
6. canonical facets, rings, and corners;
7. canonical directed/undirected edge records;
8. canonical vertex links;
9. canonical shell membership and intrinsic orientation;
10. shell-pair compatibility graph;
11. nesting forest, depth, parity, occupied sides, and winding;
12. coherent-realization evidence and bounded geometry findings;
13. resource/statistics summary; and
14. verifier disposition.

Encode source positions, normalization actions tied to source order, caller diagnostics, and presentation correspondence in a separate source/presentation section. Semantic digest equality is based on full bytes; SHA-256 is an accelerator and integrity check only.

## 16. Independent validated-operand verifier

The verifier receives the proposed immutable records, Component 01 source snapshot, Component 03 verifier capabilities, and immutable context. It must not call producer orchestration, canonical-label search, successor traversal, support-plane proposal selector, realization constructor, shell parent selector, or producer spatial-index builder as its sole check.

### 16.1 Source and topology reconstruction

Independently:

- validate owner, versions, type profile, ranges, and references;
- reread only vertices/faces from the Component 01 source snapshot;
- repeat finite and index checks;
- reconstruct ring normalization with a straightforward alternate algorithm;
- reconstruct directed uses from normalized rings;
- group full undirected keys and prove exactly two opposite uses;
- reconstruct each vertex link as a connected degree-two graph;
- rediscover shell components; and
- compare complete member sets, not producer counts/digests alone.

### 16.2 Canonical and automorphism checks

Independently verify:

- semantic graph encoding excludes source positions;
- dense IDs match the documented canonical labeling result for bounded fixtures or a separately organized verifier labeling path;
- every presentation correspondence is a graph/coordinate/ring isomorphism into its allowed class;
- semantic bytes are unchanged under presentation representative changes; and
- forced digest collisions cannot merge unequal full records.

For large canonical-label searches, the verifier may validate a proof/certificate emitted by the producer plus sampled alternate branches, but bounded exhaustive tests must independently recompute the minimum canonical encoding.

### 16.3 Geometry and coherent-certificate reconstruction

Independently:

- recheck source-point and realization envelopes;
- prove one realization per referenced source identity;
- recompute plane residuals with alternate grouping/formula IDs;
- enumerate ring segment pairs in a different order;
- verify private decomposition boundary cancellation and coverage;
- rebuild a verifier-owned triangle interaction index;
- rerun forbidden interactions with an independently organized relation path;
- recompute shell volume with a different fixed reduction grouping;
- seek or verify probes with an alternate prescribed direction order;
- reconstruct shell-pair compatibility and parent/depth relations; and
- verify occupied-negative/empty-positive and zero/one winding.

For bounded small fixtures, switch to exhaustive all-pairs and exact integer/rational oracles.

### 16.4 Encoding, resource, and publication checks

Re-encode semantic and presentation sections, recompute digests, verify precision monotonicity, verify persistent resource leases against actual immutable storage, and prove no temporary ID or caller pointer escaped.

Any committed-producer contradiction is `internal_invariant_error::validated_operand_verifier_rejection`. An expected inability to certify source geometry remains the corresponding input geometry or ambiguity failure.

Provide test-only corrupt constructors for every mutation category; exclude them from production builds.

## 17. Determinism and primary-error arbitration

Before canonical IDs exist, semantic failure keys use:

- operand role;
- failure kind;
- exact coordinate/index values involved;
- orientation-preserving normalized local cyclic encodings;
- complete typed neighborhood encodings; and
- source location only as a final non-authoritative presentation field.

After canonical IDs exist, use Component 01 entity order and full evidence keys.

For equal semantic failures in one automorphism class:

- coalesce into one finding with multiplicity/equivalence-class digest; or
- retain all bounded findings but select a primary class key independent of source order.

Parallel task schedule, range partition, diagnostic truncation, hash collision, allocation order, and thread count must not alter primary error bytes.

## 18. Resource accounting, cancellation, and rollback

Charge at least:

- referenced source vertices, facets, rings, corners, directed uses, undirected edges, and links;
- temporary and canonical shells;
- canonical graph nodes/edges/refinement/branch work;
- Component 03 source and realized points;
- plane proposals and predicate work;
- private decomposition triangles/diagonals;
- segment/triangle spatial-index nodes and candidate pairs;
- realization constraints/iterations/work;
- shell-pair records, probes, and ray relations;
- verifier reconstruction records;
- diagnostic findings and replay bytes;
- persistent and temporary bytes; and
- abstract work units.

Reserve before allocation. Parallel execution uses immutable inputs, task-private output, deterministic merge, and preallocated resource slices. Races must not decide which shell, relation, or error receives resources.

Poll cancellation:

- after preflight;
- between deterministic source/facet/edge/link/shell ranges;
- at canonical-label branch blocks;
- at realization iteration blocks;
- between plane proposal/fallback blocks;
- between segment/triangle candidate blocks;
- between shell-pair/probe/ray blocks;
- before verifier entry;
- after verifier completion; and
- immediately before publication.

On cancellation or error, stop admitting commit-producing work, join all execution scopes, roll back private IDs/storage/reservations, preserve predecessor artifacts, finalize one deterministic failure/replay record, and publish nothing.

## 19. Required tests and qualification

### 19.1 Source-contract tests

Prove:

- only source vertices/faces reach Component 02;
- arbitrary normals, colours, involved-face caches, and metadata do not change source snapshot, Component 02 artifact/error, or replay digest;
- finite isolated vertices remain replay-visible but semantic-topology-invisible; and
- topology-only mode cannot masquerade as ordinary input acceptance.

### 19.2 Structural topology tests

Cover empty, tetrahedral, box, concave polygonal, genus, disconnected, and high-valence valid meshes plus every malformed ring, index, edge-use, reciprocal-pair, bow-tie, multiple-link-cycle, and owner/version failure.

Require exact categories/subcodes and independent reconstruction.

### 19.3 Canonicalization tests

Exhaust all small presentation permutations and thousands of deterministic larger permutations. Include symmetric tetrahedra/cubes, repeated isomorphic shells, duplicate-coordinate distinct occurrences, symmetric invalid defects, and forced hash collisions.

Require:

- byte-identical semantic artifacts;
- stable canonical IDs;
- source-correct presentation correspondence;
- allowed correspondence variation only within an exact automorphism class; and
- source-order-independent primary errors.

### 19.4 Facet and coherent-realization tests

Cover:

- exact planar and bounded near-planar polygons;
- shared vertices across differently oriented facet planes;
- feasible coherent realization;
- individually acceptable facet residuals with globally infeasible shared realization;
- long thin and nearly collinear geometry;
- zero-extent coordinate-coincident source edges;
- self-crossings, touches, and collinear overlap;
- support-plane and projection ties;
- displacement/input-precision boundaries; and
- all realization work/resource/cancellation exits.

Use a test-only exact rational feasibility oracle for low-complexity fixtures and prove every published realized point lies in its source envelope.

### 19.5 Relation and shell tests

Cover the complete segment and triangle relation matrices, local wedges/stars, point/shell classifications, shell pair relations, disjoint roots, strict nesting, cavities, islands, six-level depth, point/edge/face contact, crossing boundaries with misleading probes, positive-volume overlap, coincidence, ambiguous parents, orientation reversal, and winding two.

For every accepted shell, assert occupied-negative and empty-positive.

### 19.6 Mutation tests

Mutate independently:

- normalized rings and maps;
- edge uses/pairs;
- vertex links;
- canonical IDs and automorphism classes;
- presentation correspondence;
- source point/realization/precision bounds;
- plane/projection/simplicity/orientation/decomposition evidence;
- shell membership/intrinsic orientation/pair relation;
- parent/depth/parity/occupied side/winding;
- omitted forbidden interaction;
- topology-only flag;
- canonical bytes/digests; and
- resource leases.

Every required mutation must be rejected for reconstructed content, not only a stale producer count.

### 19.7 Property, fuzz, replay, and structural performance tests

Use a fixed deterministic PRNG. Generate valid closed polygonal manifolds, nested shell trees, duplicate-coordinate separated topology, coherent near-planar realizations, and one controlled invalid mutation at a time.

Minimize and permanently retain every crash, hang, nondeterministic result, unexpected internal error, producer/verifier disagreement, index false negative, or exact-oracle mismatch.

Gate structural counters for:

- segment candidates versus exhaustive pairs;
- canonical refinement rounds/branches;
- coherent-realization variables/constraints/iterations;
- shell-pair candidates;
- triangle candidates versus exhaustive pairs;
- bounded/exact predicate calls; and
- peak resources.

Large disjoint workloads must not unconditionally degrade to all pairs. Time alone is not a correctness gate.

### 19.8 Build and sanitizer matrix

Run supported:

- GCC and Clang Debug/Release;
- ASan/UBSan;
- TSan when parallel execution is enabled;
- `float` and `double`;
- `std::uint32_t` and `std::uint64_t`; and
- Component 17 thread counts 1, 2, and maximum after concurrency integration.

Prove every Component 02 translation unit remains outside inherited fast-math and unsafe LTO behavior.

### 19.9 Contract traceability

Maintain a manifest mapping every normative clause in `component_02_input_validation_shells.md` and every Section 21 completion item below to named executable evidence. CI fails if a clause or required mutation lacks a test.

## 20. Implementation sequence and gates

1. **Schemas, versions, subcodes, source capability, and test provider.** Gate: headers compile in strict C++17; ignored fields are impossible to access through the Component 02 capability.
2. **Count/index checks and ring normalization.** Gate: overflow boundaries, source-position actions, idempotence, and empty/isolated semantics pass.
3. **Temporary edge incidence and vertex links.** Gate: every edge and bow-tie/link category is precisely distinguished.
4. **Topological shell discovery and canonical source topology.** Gate: permutations, automorphisms, collision forcing, and presentation correspondence pass.
5. **Canonical incidence reconstruction and source-point import.** Gate: no temporary/source-order identity escapes and all precision links verify.
6. **Facet plane/projection and nominal coherent certificate.** Gate: exact planar corpus and bounded predicate known answers pass.
7. **Constructed coherent-realization provider, if included in V1.** Gate: exact small feasibility oracle, displacement bounds, shared-variable invariants, resource/cancellation behavior, and independent recheck pass. If omitted, version and accepted-domain tests explicitly document nominal-only sufficiency.
8. **Facet simplicity, orientation, and private decomposition.** Gate: complete projected relation and coverage corpus passes.
9. **Validation relation kernel and local wedge/star checks.** Gate: exact small relation matrices and mutation tests pass.
10. **Intrinsic shell orientation and shell-pair compatibility.** Gate: misleading-probe crossing fixtures and complete contact/overlap matrix pass.
11. **Nesting, parity, occupied sides, and winding.** Gate: deep shell trees and all ambiguity/orientation cases pass with occupied-negative semantics.
12. **Scalable whole-operand geometry assessment.** Gate: exhaustive small comparison finds no omitted interaction.
13. **Artifact codec and independent verifier.** Gate: every mutation, digest collision, and resource reconstruction is rejected correctly.
14. **Transactions, cancellation, replay, and deterministic parallel-range behavior.** Gate: every checkpoint rolls back and produces stable errors.
15. **Full sanitizer/type/compiler/performance/traceability qualification.** Gate: Section 21 is complete before implementation is declared done.

Do not integrate Component 04 against an unverified or topology-only artifact. Component 04 must consume frozen plane/projection/orientation records and must not repair a Component 02 failure.

## 21. Definition of done

Component 02 implementation is complete only when all of the following are true:

- the input capability exposes exactly Component 01's vertices/faces source contract and no ignored public field;
- every source count, byte calculation, coordinate, and index is checked before dereference/allocation;
- ring normalization is index-only, reversible, orientation-preserving, and idempotent;
- every accepted source edge has exactly two reciprocal opposite directed uses;
- every referenced source vertex has one independently verified closed cyclic fan;
- disconnected shells are discovered only through paired indexed topology;
- topology canonicalization precedes geometry-dependent proposal order and no caller-order tie affects semantic bytes;
- canonical semantic IDs/bytes are invariant under presentation permutations, including exact automorphisms;
- presentation correspondence is source-correct without falsely claiming a unique bijection inside indistinguishable automorphism classes;
- finite isolated vertices remain outside semantic topology while remaining replayable through Component 01;
- every referenced source point has a Component 03 enclosure and monotonic precision lineage;
- every accepted facet has a bounded support plane, fixed projection, simple ring, definite nonzero orientation, and verified private coverage evidence;
- every ordinary accepted operand has one shared coherent realization certificate, or the provider explicitly and conservatively limits V1 to nominal embedded inputs;
- duplicate-coordinate identities never merge and collapsed geometry receives an explicit bounded disposition;
- local edge wedges and vertex stars are geometrically compatible with indexed topology;
- shell-pair boundary compatibility is certified before parentage;
- intrinsic shell orientation, parent, depth, parity, occupied-negative side, empty-positive side, and zero/one winding are deterministic and independently reconstructed;
- unsupported coincidence, positive-volume overlap, forbidden intersection, and incompatible uncertainty fail closed;
- topology-only artifacts are unmistakably nonpublishable and rejected by Component 04 ordinary entry;
- all expected failures use stable Component 01 categories/subcodes/stages/entities/numeric evidence/replay and deterministic arbitration;
- every allocation and work unit is preflighted, resource-accounted, cancellation-safe, joined, rollback-complete, and transactionally published;
- the independent verifier uses materially different higher-level control flow and rejects every required mutation;
- no caller pointer, mutable source storage, temporary ID, source-order semantic dependency, random ray, hash-only equality, locale, wall clock, external dependency, legacy Boolean provider, or unsuitable mutating helper enters production behavior;
- all unit, known-answer, property, metamorphic, adversarial, mutation, fuzz/shrink/replay, structural-performance, sanitizer, compiler, and type-matrix tests pass;
- every normative clause maps to executable evidence; and
- production and normative-test code remain strict portable C++17 and standard-library-only.
